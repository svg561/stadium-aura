#include "SafetyLimiter.h"

void SafetyLimiter::prepare (double sampleRate, int, int channels)
{
    lookaheadSamples = juce::jmax (1, static_cast<int> (sampleRate * 0.002));
    delayBuffer.setSize (channels, lookaheadSamples + 1, false, true, false);
    releaseCoefficient = std::exp (-1.0f / (0.080f * static_cast<float> (sampleRate)));
    enableCoefficient = 1.0f - std::exp (-1.0f / (0.010f * static_cast<float> (sampleRate)));
    reset();
}

void SafetyLimiter::reset() noexcept
{
    delayBuffer.clear();
    writePosition = 0;
    gain = 1.0f;
    enableMix = 1.0f;
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

        enableMix += ((enabled ? 1.0f : 0.0f) - enableMix) * enableCoefficient;
        const auto requiredGain = peak > ceilingLinear ? ceilingLinear / peak : 1.0f;
        gain = requiredGain < gain ? requiredGain
                                   : releaseCoefficient * gain + (1.0f - releaseCoefficient);

        const auto readPosition = (writePosition + 1) % delayBuffer.getNumSamples();
        for (int channel = 0; channel < channels; ++channel)
        {
            const auto delayed = delayBuffer.getSample (channel, readPosition);
            const auto limited = juce::jlimit (-ceilingLinear, ceilingLinear, delayed * gain);
            buffer.setSample (channel, sample, delayed + (limited - delayed) * enableMix);
        }

        writePosition = readPosition;
    }

    reductionDb = juce::Decibels::gainToDecibels (juce::jmap (enableMix, 1.0f, gain), -100.0f);
}
