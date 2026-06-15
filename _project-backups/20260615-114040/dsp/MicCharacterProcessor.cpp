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

float MicCharacterProcessor::onePoleCoefficient (float frequency, double sampleRate) noexcept
{
    return 1.0f - std::exp (-juce::MathConstants<float>::twoPi * frequency
                            / static_cast<float> (sampleRate));
}

float MicCharacterProcessor::dbToDelta (float db) noexcept
{
    return juce::Decibels::decibelsToGain (db) - 1.0f;
}

