#pragma once

#include <JuceHeader.h>

class SectionPanel final : public juce::Component
{
public:
    explicit SectionPanel (juce::String titleText) : title (std::move (titleText)) { setInterceptsMouseClicks (false, true); }
    void paint (juce::Graphics&) override;
private:
    juce::String title;
};

class RoutingButton final : public juce::TextButton
{
public:
    explicit RoutingButton (const juce::String& text) : juce::TextButton (text) { setClickingTogglesState (true); }
    void paintButton (juce::Graphics& g, bool highlighted, bool down) override;
};

class StereoLrMeter final : public juce::Component, private juce::Timer
{
public:
    explicit StereoLrMeter (juce::String titleText);
    void setTargets (float leftLevel, float rightLevel) noexcept;
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override;
    juce::String title;
    std::atomic<float> left { 0.0f }, right { 0.0f };
    float displayedLeft = 0.0f, displayedRight = 0.0f;
};

class DisabledFeatureButton final : public juce::TextButton
{
public:
    explicit DisabledFeatureButton (juce::String text, juce::String reason);
};

class VuMeterComponent final : public juce::Component, private juce::Timer
{
public:
    enum class Mode { input, gainReduction, output };
    VuMeterComponent();
    void setTargets (float inputGain, float reductionDb, float outputGain, Mode newMode) noexcept;
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override;
    std::atomic<float> input { 0.0f }, reduction { 0.0f }, output { 0.0f };
    Mode mode = Mode::gainReduction;
    float displayed = 0.0f;
};

class TubeChamberComponent final : public juce::Component
{
public:
    void setActivity (float value) noexcept { activity = juce::jlimit (0.0f, 1.0f, value); repaint(); }
    void paint (juce::Graphics&) override;
private:
    float activity = 0.0f;
};

class CompactEqComponent final : public juce::Component
{
public:
    void setTone (float value) noexcept { tone = juce::jlimit (-1.0f, 1.0f, value); repaint(); }
    void paint (juce::Graphics&) override;
private:
    float tone = 0.0f;
};
