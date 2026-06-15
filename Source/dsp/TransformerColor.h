#pragma once

#include <JuceHeader.h>

class TransformerColor
{
public:
    void prepare (double newSampleRate, int channels)
    {
        sampleRate = newSampleRate;
        lowMid.assign (static_cast<size_t> (channels), 0.0f);
        updateCoefficient();
    }

    float processSample (int channel, float input, float amount) noexcept
    {
        auto& state = lowMid[static_cast<size_t> (channel)];
        state += coefficient * (input - state);
        const auto weighted = input + state * amount * 0.12f;
        const auto rounded = std::tanh (weighted * (1.0f + amount * 0.8f));
        const auto compensation = 1.0f / (1.0f + amount * 0.22f);
        return juce::jmap (amount, input, rounded * compensation);
    }

    void reset() noexcept { std::fill (lowMid.begin(), lowMid.end(), 0.0f); }

private:
    void updateCoefficient()
    {
        coefficient = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 220.0f
                                      / static_cast<float> (sampleRate));
    }

    double sampleRate = 44100.0;
    float coefficient = 0.0f;
    std::vector<float> lowMid;
};

