#pragma once

#include <JuceHeader.h>

class RackMeter final : public juce::Component, private juce::Timer
{
public:
    enum class Kind { level, reduction };
    RackMeter (juce::String name, Kind kind);
    void setTarget (float value) noexcept { target.store (value, std::memory_order_relaxed); }
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    juce::String title;
    Kind meterKind;
    std::atomic<float> target { 0.0f };
    float displayed = 0.0f;
    float held = 0.0f;
    int holdFrames = 0;
};

