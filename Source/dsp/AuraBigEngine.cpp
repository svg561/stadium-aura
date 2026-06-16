#include "AuraBigEngine.h"

void AuraBigEngine::prepare (double sampleRate, int blockSize, int channelCount)
{
    numChannels = juce::jmax (1, channelCount);
    amountSmoother.reset (sampleRate, 0.050);
    inputGainSmoother.reset (sampleRate, 0.020);
    outputGainSmoother.reset (sampleRate, 0.020);
    amountSmoother.setCurrentAndTargetValue (0.f);
    inputGainSmoother.setCurrentAndTargetValue (1.f);
    outputGainSmoother.setCurrentAndTargetValue (1.f);
    juce::ignoreUnused (blockSize);
    reset();
}

void AuraBigEngine::reset()
{
    sweetSpotState = SweetSpotState::TooLow;
    inputRmsDb = -60.f;
    inputPeakDb = -60.f;
    active = false;
    amountSmoother.setCurrentAndTargetValue (0.f);
    inputGainSmoother.setCurrentAndTargetValue (1.f);
    outputGainSmoother.setCurrentAndTargetValue (1.f);
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
    else if (inputPeakDb >= -6.f || inputRmsDb > -12.f)
        sweetSpotState = SweetSpotState::Hot;
    else if (inputRmsDb >= -24.f && inputRmsDb <= -12.f && inputPeakDb < -6.f)
        sweetSpotState = SweetSpotState::Sweet;
    else if (inputRmsDb < -30.f)
        sweetSpotState = SweetSpotState::TooLow;
    else
        sweetSpotState = SweetSpotState::TooLow;
}

void AuraBigEngine::applyInputGain (juce::AudioBuffer<float>& buffer, int numSamples) noexcept
{
    const auto channels = juce::jmin (buffer.getNumChannels(), numChannels);
    for (int i = 0; i < numSamples; ++i)
    {
        const auto gain = inputGainSmoother.getNextValue();
        for (int channel = 0; channel < channels; ++channel)
            buffer.setSample (channel, i, buffer.getSample (channel, i) * gain);
    }
}

void AuraBigEngine::applyOutputGain (juce::AudioBuffer<float>& buffer, int numSamples) noexcept
{
    const auto channels = juce::jmin (buffer.getNumChannels(), numChannels);
    for (int i = 0; i < numSamples; ++i)
    {
        const auto gain = outputGainSmoother.getNextValue();
        for (int channel = 0; channel < channels; ++channel)
            buffer.setSample (channel, i, buffer.getSample (channel, i) * gain);
    }
}

void AuraBigEngine::process (juce::AudioBuffer<float>& buffer, const AuraBigParams& params)
{
    const auto numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    active = params.amount > 0.001f && ! params.globalBypass;
    amountSmoother.setTargetValue (params.amount);

    if (params.globalBypass || params.amount <= 0.001f)
    {
        measureInput (buffer);
        return;
    }

    if (! params.inputBypass && std::abs (params.inputDb) > 0.001f)
    {
        inputGainSmoother.setTargetValue (juce::Decibels::decibelsToGain (params.inputDb));
        applyInputGain (buffer, numSamples);
    }
    else
    {
        inputGainSmoother.skip (numSamples);
    }

    measureInput (buffer);

    // Skeleton stage placeholders — pass-through regardless of bypass flags until DSP is implemented.
    juce::ignoreUnused (params.tone, params.tube, params.transistor, params.transformer,
                         params.density, params.air, params.width, params.limiter, params.safe,
                         params.toneBypass, params.tubeBypass, params.transistorBypass,
                         params.transformerBypass, params.densityBypass, params.airBypass,
                         params.widthBypass, params.limiterBypass);

    if (std::abs (params.outputDb) > 0.001f)
    {
        outputGainSmoother.setTargetValue (juce::Decibels::decibelsToGain (params.outputDb));
        applyOutputGain (buffer, numSamples);
    }
    else
    {
        outputGainSmoother.skip (numSamples);
    }

    amountSmoother.skip (numSamples);
}
