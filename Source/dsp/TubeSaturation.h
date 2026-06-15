#pragma once

#include <JuceHeader.h>

class TubeSaturation
{
public:
    enum class Type { cleanTriode, warmTriode, hotTriode, vintagePentode, bigBottle, creamOptoTube };

    static float processSample (float input, float driveDb, float amount, float bias,
                                Type type = Type::warmTriode) noexcept
    {
        constexpr float driveScale[] { 0.72f, 1.0f, 1.35f, 1.18f, 0.92f, 0.84f };
        constexpr float asymmetry[] { 0.15f, 0.48f, 0.72f, -0.34f, 0.58f, 0.28f };
        constexpr float oddBlend[] { 0.08f, 0.14f, 0.26f, 0.38f, 0.12f, 0.06f };
        const auto index = juce::jlimit (0, 5, static_cast<int> (type));
        const auto drive = juce::Decibels::decibelsToGain (driveDb * driveScale[index]);
        const auto dc = bias * 0.045f + asymmetry[index] * amount * 0.018f;
        const auto shifted = input * drive + dc;
        const auto soft = std::tanh (shifted) - std::tanh (dc);
        const auto cubic = shifted / (1.0f + std::abs (shifted) + 0.16f * shifted * shifted);
        const auto shaped = juce::jmap (oddBlend[index], soft, cubic);
        const auto compensation = 1.0f / std::sqrt (juce::jmax (1.0f, drive))
                                * (1.0f - amount * (type == Type::hotTriode ? 0.12f : 0.04f));
        return juce::jmap (amount, input, shaped * compensation);
    }
};
