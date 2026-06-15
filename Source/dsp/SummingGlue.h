#pragma once

#include <JuceHeader.h>

class SummingGlue
{
public:
    enum class Mode { cleanConsole, vintageDesk, modernPunch, tubeConsole };

    static void processStereo (float& left, float& right, float amount, int trackCount,
                               Mode mode = Mode::cleanConsole) noexcept
    {
        float trackMultiplier = 0.25f;
        if (trackCount >= 32) trackMultiplier = 1.15f;
        else if (trackCount >= 24) trackMultiplier = 1.0f;
        else if (trackCount >= 16) trackMultiplier = 0.85f;
        else if (trackCount >= 8) trackMultiplier = 0.60f;
        constexpr float modeDrive[] { 0.36f, 0.72f, 0.58f, 0.88f };
        constexpr float modeBias[] { 0.00f, 0.08f, -0.05f, 0.16f };
        const auto index = juce::jlimit (0, 3, static_cast<int> (mode));
        const auto density = trackMultiplier * modeDrive[index];
        const auto drive = 1.0f + amount * density;
        const auto bias = modeBias[index] * amount * 0.025f;
        const auto compensation = 1.0f / (1.0f + amount * density * 0.18f);
        left = juce::jmap (amount, left, (std::tanh (left * drive + bias) - std::tanh (bias)) * compensation);
        right = juce::jmap (amount, right, (std::tanh (right * drive + bias) - std::tanh (bias)) * compensation);

        const auto crosstalk = amount * (mode == Mode::modernPunch ? 0.0015f : 0.0035f);
        const auto oldLeft = left;
        left += right * crosstalk;
        right += oldLeft * crosstalk;
    }
};
