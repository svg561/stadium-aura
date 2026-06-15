#include "SafetyLimiter.h"

void SafetyLimiter::prepare (double sampleRate, int, int channels)
{
    lookaheadSamples = juce::jmax (1, static_cast<int> (sampleRate * 0.002));
    delayBuffer.setSize (channels, lookaheadSamples + 1, false, true, false);
    releaseCoefficient = std::exp (-1.0f / (0.080f * static_cast<float> (sampleRate)));
    reset();
}

void SafetyLimiter::reset() noexcept
{
    delayBuffer.clear();
    writePosition = 0;
    gain = 1.0f;
    reductionDb = 0.0f;
}

void SafetyLimiter::process (juce::AudioBuffer<float>& buffer, juce::SmoothedValue<float>& ceilingSmoother,
                             bool enabled) noexcept
{
    const auto channels = juce::jmin (buffer.getNumChannels(), delayBuffer.getNumChannels());

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto ceilingLinear = ceilingSmoother.getNextValue();
        float peak = 0.0f;
        for (int channel = 0; channel < channels; ++channel)
        {
            const auto input = buffer.getSample (channel, sample);
            peak = juce::jmax (peak, std::abs (input));
            delayBuffer.setSample (channel, writePosition, input);
        }

        const auto requiredGain = enabled && peak > ceilingLinear ? ceilingLinear / peak : 1.0f;
        gain = requiredGain < gain ? requiredGain
                                   : releaseCoefficient * gain + (1.0f - releaseCoefficient);

        const auto readPosition = (writePosition + 1) % delayBuffer.getNumSamples();
        for (int channel = 0; channel < channels; ++channel)
        {
            auto output = delayBuffer.getSample (channel, readPosition) * gain;
            output = juce::jlimit (-ceilingLinear, ceilingLinear, output);
            buffer.setSample (channel, sample, output);
        }

        writePosition = readPosition;
    }

    reductionDb = juce::Decibels::gainToDecibels (gain, -100.0f);
}
