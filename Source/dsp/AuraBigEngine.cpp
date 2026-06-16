#include "AuraBigEngine.h"

namespace
{
constexpr float kDefaultLimiterCeilingDb = -0.6f;
constexpr float kEmergencyCeilingDb = -0.3f;
constexpr float kNearZero = 0.001f;

float softSaturate (float x) noexcept
{
    if (! std::isfinite (x))
        return 0.f;

    return std::tanh (x);
}

float sanitizeSample (float sample) noexcept
{
    if (! std::isfinite (sample))
        sample = 0.f;

    return juce::jlimit (-2.f, 2.f, sample);
}

float onePoleHighPass (float input, float& state, float coeff) noexcept
{
    state += coeff * (input - state);
    return input - state;
}
}

void AuraBigEngine::prepare (double newSampleRate, int blockSize, int channelCount)
{
    sampleRate = newSampleRate;
    numChannels = juce::jmax (1, channelCount);
    dryBuffer.setSize (numChannels, juce::jmax (1, blockSize), false, false, true);
    stageDryBuffer.setSize (numChannels, juce::jmax (1, blockSize), false, false, true);

    amountSmoother.reset (sampleRate, 0.050);
    inputGainSmoother.reset (sampleRate, 0.020);
    outputGainSmoother.reset (sampleRate, 0.020);
    bodyGainSmoother.reset (sampleRate, 0.030);
    airGainSmoother.reset (sampleRate, 0.030);
    harshGainSmoother.reset (sampleRate, 0.030);
    presenceAirGainSmoother.reset (sampleRate, 0.030);

    reset();
}

void AuraBigEngine::reset()
{
    sweetSpotState = SweetSpotState::TooLow;
    inputRmsDb = -60.f;
    inputPeakDb = -60.f;
    active = false;
    limiterGain = 1.f;
    dryBuffer.clear();
    stageDryBuffer.clear();
    tubeHeatMeter = 0.f;
    edgeHeatMeter = 0.f;
    ironHeatMeter = 0.f;
    limiterGrDbMeter = 0.f;
    visualState = {};

    amountSmoother.setCurrentAndTargetValue (0.f);
    inputGainSmoother.setCurrentAndTargetValue (1.f);
    outputGainSmoother.setCurrentAndTargetValue (1.f);
    bodyGainSmoother.setCurrentAndTargetValue (0.f);
    airGainSmoother.setCurrentAndTargetValue (0.f);
    harshGainSmoother.setCurrentAndTargetValue (0.f);
    presenceAirGainSmoother.setCurrentAndTargetValue (0.f);

    for (auto& band : toneZ1) band = { 0.f, 0.f };
    for (auto& band : toneZ2) band = { 0.f, 0.f };
    for (auto& band : transistorShelfZ1) band = { 0.f, 0.f };
    for (auto& band : transistorShelfZ2) band = { 0.f, 0.f };
    for (auto& band : transformerShelfZ1) band = { 0.f, 0.f };
    for (auto& band : transformerShelfZ2) band = { 0.f, 0.f };
    for (auto& band : presenceAirZ1) band = { 0.f, 0.f };
    for (auto& band : presenceAirZ2) band = { 0.f, 0.f };
    for (auto& band : presenceBandZ1) band = { 0.f, 0.f };
    for (auto& band : presenceBandZ2) band = { 0.f, 0.f };
    tubeDcState.fill (0.f);
    transformerHpfState.fill (0.f);

    setHighPass (subHpfCoefficients, 25.f);
    setPeak (presenceBandCoefficients, 5200.f, 0.f, 1.1f);
    updateOnePoleCoefficients();
}

void AuraBigEngine::updateOnePoleCoefficients() noexcept
{
    tubeDcCoeff = 1.f - std::exp (-juce::MathConstants<float>::twoPi * 18.f
                                 / static_cast<float> (sampleRate));
    transformerHpfCoeff = 1.f - std::exp (-juce::MathConstants<float>::twoPi * 30.f
                                           / static_cast<float> (sampleRate));
}

void AuraBigEngine::measureInput (const juce::AudioBuffer<float>& buffer) noexcept
{
    const auto numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    float sumSquares = 0.f;
    float peak = 0.f;
    const auto channels = juce::jmin (buffer.getNumChannels(), numChannels);

    for (int channel = 0; channel < channels; ++channel)
    {
        const auto* data = buffer.getReadPointer (channel);
        for (int i = 0; i < numSamples; ++i)
        {
            const auto sample = data[i];
            sumSquares += sample * sample;
            peak = juce::jmax (peak, std::abs (sample));
        }
    }

    const auto meanSquare = sumSquares / static_cast<float> (juce::jmax (1, numSamples * channels));
    inputRmsDb = juce::Decibels::gainToDecibels (std::sqrt (meanSquare), -60.f);
    inputPeakDb = juce::Decibels::gainToDecibels (peak, -60.f);
    updateSweetSpotState();
}

void AuraBigEngine::updateSweetSpotState() noexcept
{
    if (inputPeakDb > -1.f)
        sweetSpotState = SweetSpotState::Clipping;
    else if ((inputPeakDb >= -6.f && inputPeakDb <= -1.f) || inputRmsDb > -12.f)
        sweetSpotState = SweetSpotState::Hot;
    else if (inputRmsDb >= -24.f && inputRmsDb <= -12.f && inputPeakDb < -6.f)
        sweetSpotState = SweetSpotState::Sweet;
    else if (inputRmsDb < -30.f)
        sweetSpotState = SweetSpotState::TooLow;
    else
        sweetSpotState = SweetSpotState::TooLow;
}

float AuraBigEngine::measureBlockPeak (const juce::AudioBuffer<float>& buffer) const noexcept
{
    const auto numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return 0.f;

    float peak = 0.f;
    const auto channels = juce::jmin (buffer.getNumChannels(), numChannels);

    for (int channel = 0; channel < channels; ++channel)
    {
        const auto* data = buffer.getReadPointer (channel);
        for (int i = 0; i < numSamples; ++i)
            peak = juce::jmax (peak, std::abs (data[i]));
    }

    return juce::jlimit (0.f, 1.f, peak);
}

float AuraBigEngine::measurePresenceBandPeak (const juce::AudioBuffer<float>& buffer) const noexcept
{
    const auto numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return 0.f;

    float peak = 0.f;
    const auto channels = juce::jmin (buffer.getNumChannels(), numChannels);

    for (int channel = 0; channel < channels; ++channel)
    {
        const auto ch = static_cast<size_t> (channel);
        auto z1 = presenceBandZ1[ch][0];
        auto z2 = presenceBandZ2[ch][0];

        const auto* data = buffer.getReadPointer (channel);
        for (int i = 0; i < numSamples; ++i)
        {
            const auto filtered = processBiquad (data[i], z1, z2, presenceBandCoefficients);
            peak = juce::jmax (peak, std::abs (filtered));
        }
    }

    return juce::jlimit (0.f, 1.f, peak);
}

void AuraBigEngine::updateGlobalVisualHeat (float amount) noexcept
{
    const float stageMax = juce::jmax (visualState.stageIn,
                                       juce::jmax (visualState.stageTone,
                                       juce::jmax (visualState.stageTube,
                                       juce::jmax (visualState.stageEdge,
                                       juce::jmax (visualState.stageIron,
                                       juce::jmax (visualState.stageDensity,
                                       juce::jmax (visualState.stageAir,
                                       juce::jmax (visualState.stageWidth, visualState.stageLimit))))))));
    const float heatMeters = juce::jmax (tubeHeatMeter, juce::jmax (edgeHeatMeter, ironHeatMeter));
    visualState.globalHeat = juce::jlimit (0.f, 1.f, juce::jmax (stageMax * amount, heatMeters));
    visualState.limiterGrDb = limiterGrDbMeter;
    visualState.clipping = sweetSpotState == SweetSpotState::Clipping || inputPeakDb > -0.5f;
}

float AuraBigEngine::processBiquad (float input, float& z1, float& z2,
                                    const BiquadCoefficients& c) noexcept
{
    const auto output = c.b0 * input + z1;
    z1 = c.b1 * input - c.a1 * output + z2;
    z2 = c.b2 * input - c.a2 * output;
    return output;
}

void AuraBigEngine::setHighPass (BiquadCoefficients& c, float frequency) noexcept
{
    const auto omega = juce::MathConstants<float>::twoPi * juce::jlimit (20.f, 20000.f, frequency)
                     / static_cast<float> (sampleRate);
    const auto alpha = std::sin (omega) / 1.41421356f;
    const auto cosw = std::cos (omega);
    const auto a0 = 1.f + alpha;
    c.b0 = (1.f + cosw) * 0.5f / a0;
    c.b1 = -(1.f + cosw) / a0;
    c.b2 = c.b0;
    c.a1 = -2.f * cosw / a0;
    c.a2 = (1.f - alpha) / a0;
}

void AuraBigEngine::setLowShelf (BiquadCoefficients& c, float frequency, float gainDb) noexcept
{
    const auto omega = juce::MathConstants<float>::twoPi * juce::jlimit (20.f, 20000.f, frequency)
                     / static_cast<float> (sampleRate);
    const auto a = std::pow (10.f, gainDb / 40.f);
    const auto cosw = std::cos (omega);
    const auto beta = std::sqrt (a) / 0.70710678f;
    const auto sinw = std::sin (omega);

    const auto b0 = a * ((a + 1.f) - (a - 1.f) * cosw + beta * sinw);
    const auto b1 = 2.f * a * ((a - 1.f) - (a + 1.f) * cosw);
    const auto b2 = a * ((a + 1.f) - (a - 1.f) * cosw - beta * sinw);
    const auto a0 = (a + 1.f) + (a - 1.f) * cosw + beta * sinw;
    const auto a1 = -2.f * ((a - 1.f) + (a + 1.f) * cosw);
    const auto a2 = (a + 1.f) + (a - 1.f) * cosw - beta * sinw;

    c.b0 = b0 / a0; c.b1 = b1 / a0; c.b2 = b2 / a0;
    c.a1 = a1 / a0; c.a2 = a2 / a0;
}

void AuraBigEngine::setHighShelf (BiquadCoefficients& c, float frequency, float gainDb) noexcept
{
    const auto omega = juce::MathConstants<float>::twoPi * juce::jlimit (20.f, 20000.f, frequency)
                     / static_cast<float> (sampleRate);
    const auto a = std::pow (10.f, gainDb / 40.f);
    const auto cosw = std::cos (omega);
    const auto beta = std::sqrt (a) / 0.70710678f;
    const auto sinw = std::sin (omega);

    const auto b0 = a * ((a + 1.f) + (a - 1.f) * cosw + beta * sinw);
    const auto b1 = -2.f * a * ((a - 1.f) + (a + 1.f) * cosw);
    const auto b2 = a * ((a + 1.f) + (a - 1.f) * cosw - beta * sinw);
    const auto a0 = (a + 1.f) - (a - 1.f) * cosw + beta * sinw;
    const auto a1 = 2.f * ((a - 1.f) - (a + 1.f) * cosw);
    const auto a2 = (a + 1.f) - (a - 1.f) * cosw - beta * sinw;

    c.b0 = b0 / a0; c.b1 = b1 / a0; c.b2 = b2 / a0;
    c.a1 = a1 / a0; c.a2 = a2 / a0;
}

void AuraBigEngine::setPeak (BiquadCoefficients& c, float frequency, float gainDb, float q) noexcept
{
    const auto omega = juce::MathConstants<float>::twoPi * juce::jlimit (20.f, 20000.f, frequency)
                     / static_cast<float> (sampleRate);
    const auto alpha = std::sin (omega) / (2.f * juce::jlimit (0.2f, 8.f, q));
    const auto a = std::pow (10.f, gainDb / 40.f);
    const auto cosw = std::cos (omega);
    const auto a0 = 1.f + alpha / a;

    c.b0 = (1.f + alpha * a) / a0;
    c.b1 = (-2.f * cosw) / a0;
    c.b2 = (1.f - alpha * a) / a0;
    c.a1 = (-2.f * cosw) / a0;
    c.a2 = (1.f - alpha / a) / a0;
}

void AuraBigEngine::updateToneCoefficients (float bodyDb, float airDb, float harshDb) noexcept
{
    setLowShelf (bodyShelfCoefficients, 200.f, bodyDb);
    setHighShelf (airShelfCoefficients, 10000.f, airDb);
    setPeak (harshBellCoefficients, 4500.f, harshDb, 1.2f);
}

void AuraBigEngine::processInputTrim (juce::AudioBuffer<float>& buffer,
                                      const AuraBigParams& params,
                                      float amount,
                                      int numSamples) noexcept
{
    if (params.inputBypass)
    {
        inputGainSmoother.skip (numSamples);
        visualState.stageIn = 0.f;
        return;
    }

    inputGainSmoother.setTargetValue (juce::Decibels::decibelsToGain (params.inputDb));
    const auto channels = juce::jmin (buffer.getNumChannels(), numChannels);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto gain = inputGainSmoother.getNextValue();
        for (int channel = 0; channel < channels; ++channel)
            buffer.setSample (channel, i, buffer.getSample (channel, i) * gain);
    }

    const auto peak = measureBlockPeak (buffer);
    const auto trimActivity = juce::jlimit (0.f, 1.f, std::abs (params.inputDb) / 12.f);
    visualState.stageIn = juce::jlimit (0.f, 1.f, amount * trimActivity * (0.35f + peak * 0.65f));
}

void AuraBigEngine::processToneLift (juce::AudioBuffer<float>& buffer,
                                     const AuraBigParams& params,
                                     float amount) noexcept
{
    if (params.toneBypass || amount <= 0.001f)
    {
        visualState.stageTone = 0.f;
        return;
    }

    const float toneAmt = amount * params.tone;
    const float bodyLiftDb = toneAmt * 3.0f;
    const float airLiftDb = toneAmt * 2.0f;
    const float harshDipDb = amount > 0.25f ? -amount * 2.0f : 0.f;

    bodyGainSmoother.setTargetValue (bodyLiftDb);
    airGainSmoother.setTargetValue (airLiftDb);
    harshGainSmoother.setTargetValue (harshDipDb);

    updateToneCoefficients (bodyGainSmoother.getCurrentValue(),
                            airGainSmoother.getCurrentValue(),
                            harshGainSmoother.getCurrentValue());

    bodyGainSmoother.skip (1);
    airGainSmoother.skip (1);
    harshGainSmoother.skip (1);

    const auto channels = juce::jmin (buffer.getNumChannels(), numChannels);
    const auto numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        if ((i & 63) == 0)
        {
            updateToneCoefficients (bodyGainSmoother.getNextValue(),
                                    airGainSmoother.getNextValue(),
                                    harshGainSmoother.getNextValue());
        }
        else
        {
            bodyGainSmoother.skip (1);
            airGainSmoother.skip (1);
            harshGainSmoother.skip (1);
        }

        for (int channel = 0; channel < channels; ++channel)
        {
            auto sample = buffer.getSample (channel, i);
            sample = processBiquad (sample, toneZ1[0][static_cast<size_t> (channel)],
                                  toneZ2[0][static_cast<size_t> (channel)], subHpfCoefficients);
            sample = processBiquad (sample, toneZ1[1][static_cast<size_t> (channel)],
                                  toneZ2[1][static_cast<size_t> (channel)], bodyShelfCoefficients);
            sample = processBiquad (sample, toneZ1[2][static_cast<size_t> (channel)],
                                  toneZ2[2][static_cast<size_t> (channel)], harshBellCoefficients);
            sample = processBiquad (sample, toneZ1[3][static_cast<size_t> (channel)],
                                  toneZ2[3][static_cast<size_t> (channel)], airShelfCoefficients);
            buffer.setSample (channel, i, sample);
        }
    }

    const auto peak = measureBlockPeak (buffer);
    visualState.stageTone = juce::jlimit (0.f, 1.f, amount * params.tone * (0.3f + peak * 0.7f));
}

void AuraBigEngine::processTubeWarmth (juce::AudioBuffer<float>& buffer,
                                       const AuraBigParams& params,
                                       float amount) noexcept
{
    if (params.globalBypass || params.tubeBypass || amount <= kNearZero || params.tube <= kNearZero)
    {
        visualState.stageTube = 0.f;
        return;
    }

    const float tubeDrive = amount * params.tube * 1.0f;
    const float drive = 1.0f + tubeDrive * 2.5f;
    const float asym = tubeDrive * 0.018f;
    const float compensation = 1.0f / (1.0f + tubeDrive * 0.35f);
    const auto channels = juce::jmin (buffer.getNumChannels(), numChannels);
    const auto numSamples = buffer.getNumSamples();
    float blockHeat = 0.f;

    for (int i = 0; i < numSamples; ++i)
    {
        for (int channel = 0; channel < channels; ++channel)
        {
            auto sample = buffer.getSample (channel, i);
            const float shifted = sample * drive + asym;
            auto saturated = std::tanh (shifted) - std::tanh (asym);
            saturated = onePoleHighPass (saturated, tubeDcState[static_cast<size_t> (channel)], tubeDcCoeff);
            sample = sanitizeSample (saturated * compensation);
            buffer.setSample (channel, i, sample);
            blockHeat = juce::jmax (blockHeat, tubeDrive * std::abs (sample));
        }
    }

    tubeHeatMeter = juce::jmax (tubeHeatMeter * 0.90f, juce::jlimit (0.f, 1.f, blockHeat));
    visualState.stageTube = juce::jlimit (0.f, 1.f, juce::jmax (tubeHeatMeter, amount * params.tube * blockHeat));
}

void AuraBigEngine::processTransistorEdge (juce::AudioBuffer<float>& buffer,
                                           const AuraBigParams& params,
                                           float amount) noexcept
{
    if (params.globalBypass || params.transistorBypass || amount <= kNearZero || params.transistor <= kNearZero)
    {
        visualState.stageEdge = 0.f;
        return;
    }

    const float transistorDrive = amount * params.transistor * 0.75f;
    const float drive = 1.0f + transistorDrive * 1.8f;
    const float shelfDb = transistorDrive * 1.2f;
    const float compensation = 1.0f / (1.0f + transistorDrive * 0.28f);
    setHighShelf (transistorShelfCoefficients, 3200.f, shelfDb);

    const auto channels = juce::jmin (buffer.getNumChannels(), numChannels);
    const auto numSamples = buffer.getNumSamples();
    float blockHeat = 0.f;

    for (int i = 0; i < numSamples; ++i)
    {
        for (int channel = 0; channel < channels; ++channel)
        {
            auto sample = buffer.getSample (channel, i);
            sample = processBiquad (sample, transistorShelfZ1[static_cast<size_t> (channel)][0],
                                    transistorShelfZ2[static_cast<size_t> (channel)][0],
                                    transistorShelfCoefficients);
            const float driven = sample * drive;
            sample = sanitizeSample ((driven / (1.0f + std::abs (driven))) * compensation);
            buffer.setSample (channel, i, sample);
            blockHeat = juce::jmax (blockHeat, transistorDrive * std::abs (sample));
        }
    }

    edgeHeatMeter = juce::jmax (edgeHeatMeter * 0.90f, juce::jlimit (0.f, 1.f, blockHeat));
    visualState.stageEdge = juce::jlimit (0.f, 1.f, juce::jmax (edgeHeatMeter, amount * params.transistor * blockHeat));
}

void AuraBigEngine::processTransformerWeight (juce::AudioBuffer<float>& buffer,
                                              const AuraBigParams& params,
                                              float amount) noexcept
{
    if (params.globalBypass || params.transformerBypass || amount <= kNearZero || params.transformer <= kNearZero)
    {
        visualState.stageIron = 0.f;
        return;
    }

    const float transformerDrive = amount * params.transformer * 0.85f;
    const float shelfDb = juce::jmin (transformerDrive * 2.0f, 2.0f);
    const float compensation = 1.0f / (1.0f + transformerDrive * 0.22f);
    setLowShelf (transformerShelfCoefficients, 180.f, shelfDb);

    const auto channels = juce::jmin (buffer.getNumChannels(), numChannels);
    const auto numSamples = buffer.getNumSamples();
    float blockHeat = 0.f;

    for (int i = 0; i < numSamples; ++i)
    {
        for (int channel = 0; channel < channels; ++channel)
        {
            auto sample = buffer.getSample (channel, i);
            const auto shelved = processBiquad (sample, transformerShelfZ1[static_cast<size_t> (channel)][0],
                                                transformerShelfZ2[static_cast<size_t> (channel)][0],
                                                transformerShelfCoefficients);
            const auto weighted = sample * 0.65f + shelved * 0.35f;
            const auto saturated = softSaturate (weighted * (1.0f + transformerDrive * 0.8f));
            auto processed = saturated * compensation;
            processed = onePoleHighPass (processed, transformerHpfState[static_cast<size_t> (channel)],
                                         transformerHpfCoeff);
            sample = sanitizeSample (processed);
            buffer.setSample (channel, i, sample);
            blockHeat = juce::jmax (blockHeat, transformerDrive * std::abs (sample));
        }
    }

    ironHeatMeter = juce::jmax (ironHeatMeter * 0.90f, juce::jlimit (0.f, 1.f, blockHeat));
    visualState.stageIron = juce::jlimit (0.f, 1.f, juce::jmax (ironHeatMeter, amount * params.transformer * blockHeat));
}

void AuraBigEngine::processVocalDensity (juce::AudioBuffer<float>& buffer,
                                         const AuraBigParams& params,
                                         float amount) noexcept
{
    if (params.globalBypass || params.densityBypass || amount <= kNearZero || params.density <= kNearZero)
    {
        visualState.stageDensity = 0.f;
        return;
    }

    const float densityBlend = juce::jmin (amount * params.density * 0.35f, 0.35f);
    const auto channels = juce::jmin (buffer.getNumChannels(), stageDryBuffer.getNumChannels(), numChannels);
    const auto numSamples = buffer.getNumSamples();

    for (int channel = 0; channel < channels; ++channel)
        stageDryBuffer.copyFrom (channel, 0, buffer, channel, 0, numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        for (int channel = 0; channel < channels; ++channel)
        {
            const auto dry = stageDryBuffer.getSample (channel, i);
            auto dense = softSaturate (dry * (1.0f + densityBlend * 3.0f));
            dense *= 1.0f / (1.0f + std::abs (dense) * 0.1f);
            const auto wet = dry * (1.0f - densityBlend) + dense * densityBlend;
            buffer.setSample (channel, i, sanitizeSample (wet));
        }
    }

    const auto peak = measureBlockPeak (buffer);
    visualState.stageDensity = juce::jlimit (0.f, 1.f, amount * params.density * (0.25f + peak * 0.75f));
}

void AuraBigEngine::processAirPresence (juce::AudioBuffer<float>& buffer,
                                        const AuraBigParams& params,
                                        float amount) noexcept
{
    if (params.airBypass || amount <= kNearZero || params.air <= kNearZero)
    {
        presenceAirGainSmoother.setTargetValue (0.f);
        visualState.stageAir = 0.f;
        return;
    }

    const float presencePeak = measurePresenceBandPeak (buffer);
    float harshGuard = 1.0f;

    if (presencePeak > 0.30f)
        harshGuard -= juce::jmin (0.5f, (presencePeak - 0.30f) * 1.25f);

    if (std::abs (harshGainSmoother.getCurrentValue()) > 0.05f)
        harshGuard -= 0.5f * juce::jmin (1.f, std::abs (harshGainSmoother.getCurrentValue()) / 2.0f);

    harshGuard = juce::jmax (0.5f, harshGuard);

    const float airLiftDb = amount * params.air * 4.0f * harshGuard;
    presenceAirGainSmoother.setTargetValue (airLiftDb);
    setHighShelf (presenceAirShelfCoefficients, 11000.f, presenceAirGainSmoother.getCurrentValue());
    presenceAirGainSmoother.skip (1);

    const auto channels = juce::jmin (buffer.getNumChannels(), numChannels);
    const auto numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        if ((i & 63) == 0)
        {
            setHighShelf (presenceAirShelfCoefficients, 11000.f, presenceAirGainSmoother.getNextValue());
        }
        else
        {
            presenceAirGainSmoother.skip (1);
        }

        for (int channel = 0; channel < channels; ++channel)
        {
            auto sample = buffer.getSample (channel, i);
            sample = processBiquad (sample, presenceAirZ1[static_cast<size_t> (channel)][0],
                                    presenceAirZ2[static_cast<size_t> (channel)][0],
                                    presenceAirShelfCoefficients);
            buffer.setSample (channel, i, sanitizeSample (sample));
        }
    }

    const auto peak = measureBlockPeak (buffer);
    visualState.stageAir = juce::jlimit (0.f, 1.f, amount * params.air * (0.3f + peak * 0.7f));
}

void AuraBigEngine::processSubtleWidth (juce::AudioBuffer<float>& buffer,
                                        const AuraBigParams& params,
                                        float amount) noexcept
{
    if (params.widthBypass || amount <= kNearZero || params.width <= kNearZero
        || buffer.getNumChannels() < 2)
    {
        visualState.stageWidth = 0.f;
        return;
    }

    const float widthAmount = amount * params.width;
    const float sideGain = 1.0f + widthAmount;
    const auto numSamples = buffer.getNumSamples();
    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);
    float blockPeak = 0.f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float mid = (left[i] + right[i]) * 0.5f;
        const float side = (left[i] - right[i]) * 0.5f;
        left[i] = sanitizeSample (mid + side * sideGain);
        right[i] = sanitizeSample (mid - side * sideGain);
        blockPeak = juce::jmax (blockPeak, std::abs (left[i]), std::abs (right[i]));
    }

    if (blockPeak > 0.95f)
    {
        const float norm = 0.95f / blockPeak;
        for (int i = 0; i < numSamples; ++i)
        {
            left[i] = sanitizeSample (left[i] * norm);
            right[i] = sanitizeSample (right[i] * norm);
        }
    }

    visualState.stageWidth = juce::jlimit (0.f, 1.f, widthAmount * (0.4f + blockPeak * 0.6f));
}

void AuraBigEngine::processAdaptiveLimiter (juce::AudioBuffer<float>& buffer,
                                            const AuraBigParams& params,
                                            int numSamples) noexcept
{
    const bool emergencyOnly = params.safe && params.limiterBypass;
    if (params.limiterBypass && ! params.safe)
    {
        visualState.stageLimit = 0.f;
        limiterGrDbMeter = 0.f;
        return;
    }

    const float ceilingDb = emergencyOnly ? kEmergencyCeilingDb : kDefaultLimiterCeilingDb;
    const float ceiling = juce::Decibels::decibelsToGain (ceilingDb);
    const float strength = emergencyOnly ? 0.35f : juce::jlimit (0.f, 1.f, params.limiter);
    const float releaseMs = juce::jmap (strength, 120.f, 25.f);
    const float releaseCoeff = std::exp (-1.0f / (releaseMs * 0.001f * static_cast<float> (sampleRate)));
    const auto channels = juce::jmin (buffer.getNumChannels(), numChannels);
    float blockGrDb = 0.f;

    for (int i = 0; i < numSamples; ++i)
    {
        float peak = 0.f;
        for (int channel = 0; channel < channels; ++channel)
            peak = juce::jmax (peak, std::abs (buffer.getSample (channel, i)));

        const float detector = peak * limiterGain;
        if (detector > ceiling)
        {
            const float requiredGain = ceiling / juce::jmax (peak, 1.0e-8f);
            limiterGain = juce::jmin (limiterGain, requiredGain);
        }
        else
        {
            limiterGain = releaseCoeff * limiterGain + (1.f - releaseCoeff) * 1.f;
        }

        const float appliedGain = juce::jmap (strength, 1.f, limiterGain);
        blockGrDb = juce::jmin (blockGrDb, juce::Decibels::gainToDecibels (appliedGain, -60.f));

        for (int channel = 0; channel < channels; ++channel)
            buffer.setSample (channel, i, buffer.getSample (channel, i) * appliedGain);
    }

    limiterGrDbMeter = juce::jmin (limiterGrDbMeter * 0.88f, blockGrDb);
    visualState.stageLimit = juce::jlimit (0.f, 1.f,
        params.amount * strength * juce::jlimit (0.f, 1.f, -limiterGrDbMeter / 12.f));
}

void AuraBigEngine::processOutputTrim (juce::AudioBuffer<float>& buffer,
                                       const AuraBigParams& params,
                                       int numSamples) noexcept
{
    outputGainSmoother.setTargetValue (juce::Decibels::decibelsToGain (params.outputDb));
    const auto channels = juce::jmin (buffer.getNumChannels(), numChannels);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto gain = outputGainSmoother.getNextValue();
        for (int channel = 0; channel < channels; ++channel)
            buffer.setSample (channel, i, buffer.getSample (channel, i) * gain);
    }
}

void AuraBigEngine::blendWithDry (juce::AudioBuffer<float>& buffer, int numSamples) noexcept
{
    const auto channels = juce::jmin (buffer.getNumChannels(), dryBuffer.getNumChannels(), numChannels);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto amt = amountSmoother.getNextValue();
        const auto wetMix = juce::jlimit (0.f, 1.f, amt);

        for (int channel = 0; channel < channels; ++channel)
        {
            const auto dry = dryBuffer.getSample (channel, i);
            const auto wet = buffer.getSample (channel, i);
            buffer.setSample (channel, i, dry + wetMix * (wet - dry));
        }
    }
}

void AuraBigEngine::process (juce::AudioBuffer<float>& buffer, const AuraBigParams& params)
{
    const auto numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    measureInput (buffer);

    const auto targetAmount = params.globalBypass ? 0.f : params.amount;
    amountSmoother.setTargetValue (targetAmount);
    active = targetAmount > 0.001f && ! params.globalBypass;

    if (params.globalBypass
        || (params.amount <= 0.001f
            && std::abs (amountSmoother.getCurrentValue() - amountSmoother.getTargetValue()) < 1.0e-5f
            && amountSmoother.getTargetValue() <= 0.001f))
    {
        amountSmoother.skip (numSamples);
        inputGainSmoother.skip (numSamples);
        outputGainSmoother.skip (numSamples);
        bodyGainSmoother.skip (numSamples);
        airGainSmoother.skip (numSamples);
        harshGainSmoother.skip (numSamples);
        presenceAirGainSmoother.skip (numSamples);
        tubeHeatMeter = 0.f;
        edgeHeatMeter = 0.f;
        ironHeatMeter = 0.f;
        limiterGrDbMeter = 0.f;
        visualState = {};
        return;
    }

    visualState.bypassIn = params.inputBypass;
    visualState.bypassTone = params.toneBypass;
    visualState.bypassTube = params.tubeBypass;
    visualState.bypassEdge = params.transistorBypass;
    visualState.bypassIron = params.transformerBypass;
    visualState.bypassDensity = params.densityBypass;
    visualState.bypassAir = params.airBypass;
    visualState.bypassWidth = params.widthBypass;
    visualState.bypassLimit = params.limiterBypass;

    const auto channels = juce::jmin (buffer.getNumChannels(), dryBuffer.getNumChannels(), numChannels);
    for (int channel = 0; channel < channels; ++channel)
        dryBuffer.copyFrom (channel, 0, buffer, channel, 0, numSamples);

    const float blockAmount = juce::jlimit (0.f, 1.f, params.amount);

    processInputTrim (buffer, params, blockAmount, numSamples);
    processToneLift (buffer, params, blockAmount);
    processTubeWarmth (buffer, params, blockAmount);
    processTransistorEdge (buffer, params, blockAmount);
    processTransformerWeight (buffer, params, blockAmount);
    processVocalDensity (buffer, params, blockAmount);
    processAirPresence (buffer, params, blockAmount);
    processSubtleWidth (buffer, params, blockAmount);
    processAdaptiveLimiter (buffer, params, numSamples);
    processOutputTrim (buffer, params, numSamples);
    blendWithDry (buffer, numSamples);
    updateGlobalVisualHeat (blockAmount);
}
