#pragma once

#include <JuceHeader.h>

class SafetyLimiter
{
public:
    void prepare (double sampleRate, int maximumBlockSize, int channels);
    void reset() noexcept;
    void process (juce::AudioBuffer<float>& buffer, juce::SmoothedValue<float>& ceilingLinear,
                  bool enabled) noexcept;
    float getReductionDb() const noexcept { return reductionDb; }
    int getLatencySamples() const noexcept { return lookaheadSamples; }

private:
    juce::AudioBuffer<float> delayBuffer;
    int lookaheadSamples = 0;
    int writePosition = 0;
    float gain = 1.0f;
    float releaseCoefficient = 0.0f;
    float reductionDb = 0.0f;
};
