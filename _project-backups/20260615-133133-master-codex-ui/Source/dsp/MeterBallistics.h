#pragma once

#include <JuceHeader.h>

class MeterBallistics
{
public:
    void prepare (double sampleRate, double attackSeconds, double releaseSeconds)
    {
        attack = coefficient (sampleRate, attackSeconds);
        release = coefficient (sampleRate, releaseSeconds);
        current = 0.0f;
    }

    float process (float input) noexcept
    {
        const auto c = input > current ? attack : release;
        current = c * current + (1.0f - c) * input;
        return current;
    }

    float getCurrentValue() const noexcept { return current; }

private:
    static float coefficient (double sampleRate, double seconds)
    {
        return static_cast<float> (std::exp (-1.0 / (sampleRate * seconds)));
    }

    float attack = 0.0f;
    float release = 0.0f;
    float current = 0.0f;
};

