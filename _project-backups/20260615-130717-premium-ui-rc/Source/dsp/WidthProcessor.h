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

    void processStereo (float& left, float& right, float width, bool monoCheck) noexcept
    {
        lowLeft += lowPassCoefficient * (left - lowLeft);
        lowRight += lowPassCoefficient * (right - lowRight);

        const auto highLeft = left - lowLeft;
        const auto highRight = right - lowRight;
        const auto lowMid = (lowLeft + lowRight) * 0.5f;
        const auto highMid = (highLeft + highRight) * 0.5f;
        auto highSide = (highLeft - highRight) * 0.5f * width;

        if (monoCheck)
            highSide = 0.0f;

        left = lowMid + highMid + highSide;
        right = lowMid + highMid - highSide;
    }

private:
    float lowPassCoefficient = 0.0f;
    float lowLeft = 0.0f;
    float lowRight = 0.0f;
};

