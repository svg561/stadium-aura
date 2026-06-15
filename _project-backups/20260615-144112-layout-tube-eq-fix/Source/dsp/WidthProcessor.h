#pragma once

#include <JuceHeader.h>

class WidthProcessor
{
public:
    void prepare (double sampleRate)
    {
        lowPassCoefficient = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 120.0f
                                              / static_cast<float> (sampleRate));
        lowLeft = lowRight = 0.0f;
    }

    void reset() noexcept { lowLeft = lowRight = 0.0f; }

    void processStereo (float& left, float& right, float width, float monoBlend) noexcept
    {
        lowLeft += lowPassCoefficient * (left - lowLeft);
        lowRight += lowPassCoefficient * (right - lowRight);

        const auto highLeft = left - lowLeft;
        const auto highRight = right - lowRight;
        const auto lowMid = (lowLeft + lowRight) * 0.5f;
        const auto highMid = (highLeft + highRight) * 0.5f;
        const auto monoSafeWidth = width * (1.0f - monoBlend);
        auto highSide = (highLeft - highRight) * 0.5f * monoSafeWidth;

        left = lowMid + highMid + highSide;
        right = lowMid + highMid - highSide;

        const auto mid = (left + right) * 0.5f;
        left = juce::jmap (monoBlend, left, mid);
        right = juce::jmap (monoBlend, right, mid);
    }

private:
    float lowPassCoefficient = 0.0f;
    float lowLeft = 0.0f;
    float lowRight = 0.0f;
};

