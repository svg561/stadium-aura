#include "MicCharacterProcessor.h"

void MicCharacterProcessor::prepare (double sampleRate, int channels)
{
    currentSampleRate = sampleRate;
    states.assign (static_cast<size_t> (channels), {});
    profileSmoothing = 1.0f - std::exp (-1.0f / (0.035f * static_cast<float> (sampleRate)));
    reset();
}

void MicCharacterProcessor::reset() noexcept
{
    std::fill (states.begin(), states.end(), State {});
}

float MicCharacterProcessor::processSample (int channel, float input, Mode mode) noexcept
{
    const auto target = profileFor (mode);
    auto smooth = [this] (float& current, float next) { current += (next - current) * profileSmoothing; };
    smooth (currentProfile.highPassHz, target.highPassHz);
    smooth (currentProfile.bodyDb, target.bodyDb);
    smooth (currentProfile.presenceDb, target.presenceDb);
    smooth (currentProfile.airDb, target.airDb);
    smooth (currentProfile.harmonicAmount, target.harmonicAmount);
    smooth (currentProfile.harmonicBias, target.harmonicBias);
    smooth (currentProfile.outputDb, target.outputDb);

    auto& state = states[static_cast<size_t> (channel)];
    const auto dcCoeff = onePoleCoefficient (currentProfile.highPassHz, currentSampleRate);
    const auto bodyCoeff = onePoleCoefficient (320.0f, currentSampleRate);
    const auto presenceCoeff = onePoleCoefficient (4200.0f, currentSampleRate);
    const auto airCoeff = onePoleCoefficient (11000.0f, currentSampleRate);

    state.dc += dcCoeff * (input - state.dc);
    const auto highPassed = input - state.dc;
    state.low += bodyCoeff * (highPassed - state.low);
    state.presenceLow += presenceCoeff * (highPassed - state.presenceLow);
    state.airLow += airCoeff * (highPassed - state.airLow);

    const auto body = state.low;
    const auto presence = state.presenceLow - state.low;
    const auto air = highPassed - state.airLow;
    auto shaped = highPassed
                + body * dbToDelta (currentProfile.bodyDb)
                + presence * dbToDelta (currentProfile.presenceDb)
                + air * dbToDelta (currentProfile.airDb);

    const auto biased = shaped + currentProfile.harmonicBias * 0.035f;
    const auto saturated = std::tanh (biased * (1.0f + currentProfile.harmonicAmount * 1.8f))
                         - std::tanh (currentProfile.harmonicBias * 0.035f);
    shaped = juce::jmap (currentProfile.harmonicAmount, shaped, saturated);
    return shaped * juce::Decibels::decibelsToGain (currentProfile.outputDb);
}

float MicCharacterProcessor::processRelativeSample (int channel, float input, SourceMode source, Mode target,
                                                     float correctionAmount, float targetAmount,
                                                     float badFrequencyTamer, float airProtection,
                                                     float bodyProtection, bool hardwareSafe) noexcept
{
    auto& state = states[static_cast<size_t> (channel)];
    const auto safety = hardwareSafe ? 0.62f : 1.0f;
    const auto corrected = processProfile (state, input, sourceProfileFor (source),
                                           correctionAmount * safety, airProtection, bodyProtection, true);

    const auto presenceCoeff = onePoleCoefficient (3600.0f, currentSampleRate);
    const auto airCoeff = onePoleCoefficient (8200.0f, currentSampleRate);
    state.presenceLow += presenceCoeff * (corrected - state.presenceLow);
    state.airLow += airCoeff * (corrected - state.airLow);
    const auto harshBand = state.airLow - state.presenceLow;
    const auto energy = std::abs (harshBand);
    const auto envelopeCoeff = energy > state.problemEnvelope ? 0.08f : 0.003f;
    state.problemEnvelope += (energy - state.problemEnvelope) * envelopeCoeff;
    const auto dynamicCut = juce::jlimit (0.0f, 0.42f * safety,
                                         (state.problemEnvelope - 0.035f) * badFrequencyTamer * 4.0f);
    const auto cleaned = corrected - harshBand * dynamicCut;
    return processProfile (state, cleaned, profileFor (target), targetAmount * safety,
                           airProtection, bodyProtection, false);
}

float MicCharacterProcessor::processProfile (State& state, float input, const Profile& profile, float amount,
                                              float airProtection, float bodyProtection, bool invert) noexcept
{
    const auto bodyCoeff = onePoleCoefficient (320.0f, currentSampleRate);
    const auto presenceCoeff = onePoleCoefficient (4200.0f, currentSampleRate);
    const auto airCoeff = onePoleCoefficient (11000.0f, currentSampleRate);
    state.low += bodyCoeff * (input - state.low);
    state.presenceLow += presenceCoeff * (input - state.presenceLow);
    state.airLow += airCoeff * (input - state.airLow);
    const auto direction = invert ? -1.0f : 1.0f;
    const auto bodyScale = 1.0f - juce::jlimit (0.0f, 1.0f, bodyProtection) * 0.72f;
    const auto airScale = 1.0f - juce::jlimit (0.0f, 1.0f, airProtection) * 0.72f;
    const auto body = state.low;
    const auto presence = state.presenceLow - state.low;
    const auto air = input - state.airLow;
    auto shaped = input
        + body * dbToDelta (profile.bodyDb * direction * amount * bodyScale)
        + presence * dbToDelta (profile.presenceDb * direction * amount)
        + air * dbToDelta (profile.airDb * direction * amount * airScale);
    if (! invert)
    {
        const auto harmonicAmount = profile.harmonicAmount * amount;
        const auto dc = profile.harmonicBias * amount * 0.025f;
        const auto saturated = std::tanh (shaped * (1.0f + harmonicAmount * 1.5f) + dc) - std::tanh (dc);
        shaped = juce::jmap (harmonicAmount, shaped, saturated);
    }
    return shaped * juce::Decibels::decibelsToGain (profile.outputDb * amount * direction);
}

MicCharacterProcessor::Profile MicCharacterProcessor::profileFor (Mode mode) noexcept
{
    switch (mode)
    {
        case Mode::u67:        return { 55.0f,  0.8f, -0.8f,  0.2f, 0.16f,  0.18f, -0.35f };
        case Mode::c12:        return { 72.0f, -0.7f,  1.0f,  1.8f, 0.10f,  0.10f, -0.70f };
        case Mode::classic251: return { 65.0f, -0.5f,  0.7f,  1.5f, 0.12f,  0.14f, -0.60f };
        case Mode::u87:        return { 78.0f, -0.6f,  0.8f,  0.6f, 0.035f, 0.00f, -0.20f };
        case Mode::u47:        return { 45.0f,  1.3f, -0.7f, -0.3f, 0.22f,  0.24f, -0.55f };
        case Mode::elaM251:    return { 68.0f, -0.6f,  0.9f,  2.0f, 0.14f,  0.18f, -0.75f };
        case Mode::c800g:      return { 82.0f, -1.0f,  1.5f,  2.3f, 0.07f,  0.05f, -0.95f };
    }
    return { 78.0f, -0.6f, 0.8f, 0.6f, 0.035f, 0.0f, -0.2f };
}

MicCharacterProcessor::Profile MicCharacterProcessor::sourceProfileFor (SourceMode mode) noexcept
{
    switch (mode)
    {
        case SourceMode::unknown:          return { 55.0f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f, 0.0f };
        case SourceMode::dynamicGeneral:   return { 65.0f,  1.0f, -0.5f, -1.8f, 0.0f, 0.0f, 0.0f };
        case SourceMode::condenserGeneral: return { 72.0f, -0.3f,  0.5f,  0.8f, 0.0f, 0.0f, 0.0f };
        case SourceMode::ribbonGeneral:    return { 45.0f,  1.1f, -1.0f, -2.5f, 0.0f, 0.0f, 0.0f };
        case SourceMode::sm57Style:        return { 85.0f, -0.4f,  1.7f, -2.8f, 0.0f, 0.0f, 0.0f };
        case SourceMode::sm7Style:         return { 55.0f,  1.5f, -0.3f, -2.2f, 0.0f, 0.0f, 0.0f };
        case SourceMode::c80Style:         return { 75.0f, -0.5f,  0.9f,  1.0f, 0.0f, 0.0f, 0.0f };
        case SourceMode::brightCondenser:  return { 80.0f, -0.8f,  1.2f,  2.0f, 0.0f, 0.0f, 0.0f };
        case SourceMode::darkCondenser:    return { 60.0f,  1.0f, -0.7f, -1.5f, 0.0f, 0.0f, 0.0f };
        case SourceMode::warmTubeMic:      return { 48.0f,  1.4f, -0.4f, -0.4f, 0.0f, 0.0f, 0.0f };
        case SourceMode::flatMeasurement:  return { 30.0f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f, 0.0f };
    }
    return { 55.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
}

float MicCharacterProcessor::onePoleCoefficient (float frequency, double sampleRate) noexcept
{
    return 1.0f - std::exp (-juce::MathConstants<float>::twoPi * frequency
                            / static_cast<float> (sampleRate));
}

float MicCharacterProcessor::dbToDelta (float db) noexcept
{
    return juce::Decibels::decibelsToGain (db) - 1.0f;
}
