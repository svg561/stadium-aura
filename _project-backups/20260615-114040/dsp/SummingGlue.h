#pragma once

#include <JuceHeader.h>

class SummingGlue
{
public:
    static void processStereo (float& left, float& right, float amount, int trackCount) noexcept
    {
        const auto density = juce::jmap (static_cast<float> (trackCount), 1.0f, 32.0f, 0.55f, 1.25f);
        const auto drive = 1.0f + amount * density * 0.65f;
        const auto compensation = 1.0f / (1.0f + amount * density * 0.16f);
        left = juce::jmap (amount, left, std::tanh (left * drive) * compensation);
        right = juce::jmap (amount, right, std::tanh (right * drive) * compensation);

        const auto crosstalk = amount * 0.004f;
        const auto oldLeft = left;
        left += right * crosstalk;
        right += oldLeft * crosstalk;
    }
};
