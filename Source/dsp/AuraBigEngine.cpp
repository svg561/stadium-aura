#include "AuraBigEngine.h"

namespace
{
constexpr float kDefaultLimiterCeilingDb = -0.6f;
constexpr float kEmergencyCeilingDb = -0.3f;
}

void AuraBigEngine::prepare (double newSampleRate, int blockSize, int channelCount)
{
    sampleRate = newSampleRate;
    numChannels = juce::jmax (1, channelCount);
    dryBuffer.setSize (numChannels, juce::jmax (1, blockSize), false, false, true);

    amountSmoother.reset (sampleRate, 0.050);
    inputGainSmoother.reset (sampleRate, 0.020);
    outputGainSmoother.reset (sampleRate, 0.020);
    bodyGainSmoother.reset (sampleRate, 0.030);
    airGainSmoother.reset (sampleRate, 0.030);
    harshGainSmoother.reset (sampleRate, 0.030);

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

    amountSmoother.setCurrentAndTargetValue (0.f);
    inputGainSmoother.setCurrentAndTargetValue (1.f);
    outputGainSmoother.setCurrentAndTargetValue (1.f);
    bodyGainSmoother.setCurrentAndTargetValue (0.f);
    airGainSmoother.setCurrentAndTargetValue (0.f);
    harshGainSmoother.setCurrentAndTargetValue (0.f);

    for (auto& band : toneZ1) band = { 0.f, 0.f };
    for (auto& band : toneZ2) band = { 0.f, 0.f };

    setHighPass (subHpfCoefficients, 25.f);
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
                                      int numSamples) noexcept
{
    if (params.inputBypass)
    {
        inputGainSmoother.skip (numSamples);
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
}

void AuraBigEngine::processToneLift (juce::AudioBuffer<float>& buffer,
                                     const AuraBigParams& params,
                                     float amount) noexcept
{
    if (params.toneBypass || amount <= 0.001f)
        return;

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
}

void AuraBigEngine::processPlaceholderStages (const AuraBigParams& params) noexcept
{
    juce::ignoreUnused (params.tube, params.transistor, params.transformer,
                         params.density, params.air, params.width);

    if (params.tubeBypass || params.globalBypass)
        { /* pass-through */ }
    if (params.transistorBypass || params.globalBypass)
        { /* pass-through */ }
    if (params.transformerBypass || params.globalBypass)
        { /* pass-through */ }
    if (params.densityBypass || params.globalBypass)
        { /* pass-through */ }
    if (params.airBypass || params.globalBypass)
        { /* pass-through */ }
    if (params.widthBypass || params.globalBypass)
        { /* pass-through */ }
}

void AuraBigEngine::processAdaptiveLimiter (juce::AudioBuffer<float>& buffer,
                                            const AuraBigParams& params,
                                            int numSamples) noexcept
{
    const bool emergencyOnly = params.safe && params.limiterBypass;
    if (params.limiterBypass && ! params.safe)
        return;

    const float ceilingDb = emergencyOnly ? kEmergencyCeilingDb : kDefaultLimiterCeilingDb;
    const float ceiling = juce::Decibels::decibelsToGain (ceilingDb);
    const float strength = emergencyOnly ? 0.35f : juce::jlimit (0.f, 1.f, params.limiter);
    const float releaseMs = juce::jmap (strength, 120.f, 25.f);
    const float releaseCoeff = std::exp (-1.0f / (releaseMs * 0.001f * static_cast<float> (sampleRate)));
    const auto channels = juce::jmin (buffer.getNumChannels(), numChannels);

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
        for (int channel = 0; channel < channels; ++channel)
            buffer.setSample (channel, i, buffer.getSample (channel, i) * appliedGain);
    }
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
        return;
    }

    const auto channels = juce::jmin (buffer.getNumChannels(), dryBuffer.getNumChannels(), numChannels);
    for (int channel = 0; channel < channels; ++channel)
        dryBuffer.copyFrom (channel, 0, buffer, channel, 0, numSamples);

    const float blockAmount = juce::jlimit (0.f, 1.f, params.amount);

    processInputTrim (buffer, params, numSamples);
    processToneLift (buffer, params, blockAmount);
    processPlaceholderStages (params);
    processAdaptiveLimiter (buffer, params, numSamples);
    processOutputTrim (buffer, params, numSamples);
    blendWithDry (buffer, numSamples);
}
