#pragma once

#include <JuceHeader.h>

class TubeSaturation
{
public:
    static float processSample (float input, float driveDb, float amount, float bias) noexcept
    {
        const auto drive = juce::Decibels::decibelsToGain (driveDb);
        const auto shifted = input * drive + bias * 0.045f;
        const auto shaped = std::tanh (shifted) - std::tanh (bias * 0.045f);
        const auto compensation = 1.0f / std::sqrt (juce::jmax (1.0f, drive));
        return juce::jmap (amount, input, shaped * compensation);
    }
};

