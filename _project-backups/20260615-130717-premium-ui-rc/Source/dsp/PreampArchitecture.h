#pragma once

#include <JuceHeader.h>

class PreampArchitecture
{
public:
    enum class Mode { vintage73, apiPunch, avalonClean };

    void prepare (double sampleRate, int channels)
    {
        lowState.assign (static_cast<size_t> (channels), 0.0f);
        lowCoefficient = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 260.0f
                                         / static_cast<float> (sampleRate));
        smoothing = 1.0f - std::exp (-1.0f / (0.030f * static_cast<float> (sampleRate)));
    }

    void reset() noexcept { std::fill (lowState.begin(), lowState.end(), 0.0f); }

    float processSample (int channel, float input, Mode mode, float amount = 0.25f) noexcept
    {
        float targetBody = 0.0f, targetDrive = 0.0f, targetBias = 0.0f, targetOutput = 1.0f;
        switch (mode)
        {
            case Mode::vintage73: targetBody = 0.13f; targetDrive = 0.30f; targetBias = 0.12f; targetOutput = 0.91f; break;
            case Mode::apiPunch: targetBody = -0.04f; targetDrive = 0.22f; targetBias = -0.06f; targetOutput = 0.94f; break;
            case Mode::avalonClean: targetBody = -0.025f; targetDrive = 0.08f; targetBias = 0.04f; targetOutput = 0.98f; break;
        }
        const auto driveAmount = juce::jlimit (0.0f, 1.0f, amount);
        targetBody *= 0.25f + driveAmount * 0.75f;
        targetDrive *= 0.15f + driveAmount * 1.35f;
        targetBias *= driveAmount;
        body += (targetBody - body) * smoothing;
        drive += (targetDrive - drive) * smoothing;
        bias += (targetBias - bias) * smoothing;
        output += (targetOutput - output) * smoothing;

        auto& low = lowState[static_cast<size_t> (channel)];
        low += lowCoefficient * (input - low);
        const auto weighted = input + low * body;
        const auto transient = weighted + (weighted - low) * (mode == Mode::apiPunch ? 0.10f * driveAmount : 0.0f);
        const auto saturated = std::tanh (transient * (1.0f + drive) + bias * 0.03f)
                             - std::tanh (bias * 0.03f);
        return juce::jmap (drive, transient, saturated) * output;
    }

private:
    std::vector<float> lowState;
    float lowCoefficient = 0.0f;
    float smoothing = 0.0f;
    float body = 0.0f, drive = 0.0f, bias = 0.0f, output = 1.0f;
};
