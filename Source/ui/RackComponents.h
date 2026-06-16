#pragma once

#include <JuceHeader.h>
#include "RackDrawing.h"

class RackModulePanel final : public juce::Component
{
public:
    explicit RackModulePanel (juce::String titleText) : title (std::move (titleText)) { setInterceptsMouseClicks (false, true); }
    void setRouteVisualState (bool highlighted, bool dimmed) noexcept;
    void paint (juce::Graphics&) override;
private:
    juce::String title;
    bool routeHighlighted = false;
    bool routeDimmed = false;
};

class NavRouteButton final : public juce::TextButton
{
public:
    explicit NavRouteButton (const juce::String& text) : juce::TextButton (text) { setClickingTogglesState (true); }
    std::function<void()> onRightClick;
    void paintButton (juce::Graphics& g, bool highlighted, bool down) override;
    void mouseDown (const juce::MouseEvent& e) override;
};

class IconBarButton final : public juce::TextButton
{
public:
    IconBarButton (juce::String label, bool enabled = true);
};

class VerticalRmsMeter final : public juce::Component, private juce::Timer
{
public:
    explicit VerticalRmsMeter (juce::String titleText);
    void setTarget (float linearGain) noexcept;
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override;
    juce::String title;
    std::atomic<float> target { 0.0f };
    float displayed = 0.0f;
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
    void setDistortionLevel (float level) noexcept { distortionLevel = juce::jlimit (0.0f, 1.0f, level); }
    void paint (juce::Graphics&) override;
private:
    float activity = 0.0f;
    float distortionLevel = 0.0f;
};

class EqSpectrumComponent final : public juce::Component
{
public:
    static constexpr int analyzerBins = 48;
    struct BandValues
    {
        float hpfHz = 35.0f;
        float lowShelfHz = 120.0f;
        float lowShelfGainDb = 0.0f;
        float bellHz = 650.0f;
        float bellGainDb = 0.0f;
        float bellQ = 1.0f;
        float highShelfHz = 6500.0f;
        float highShelfGainDb = 0.0f;
        float lpfHz = 18000.0f;
        bool enabled = true;
    };

    void setTone (float value) noexcept { tone = juce::jlimit (-1.0f, 1.0f, value); repaint(); }
    void setAnalyzerLevels (const std::array<float, analyzerBins>& levels);
    void setBandValues (const BandValues& values);
    void setExpanded (bool shouldExpand);
    bool isExpanded() const noexcept { return expanded; }
    std::function<void (bool)> onExpandedChanged;
    std::function<void (int, float, float)> onNodeDragged;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
private:
    juce::Rectangle<float> getPlotBounds() const;
    juce::Point<float> nodePositionForBand (int band, juce::Rectangle<float> plot) const;
    int hitTestNode (juce::Point<float> position) const;
    static float frequencyToNorm (float frequency) noexcept;
    static float gainToNorm (float gainDb) noexcept;

    std::array<float, analyzerBins> analyzer {};
    BandValues bands;
    float tone = 0.0f;
    bool expanded = false;
    int activeNode = -1;
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

class HeroAuraRing final : public juce::Component
{
public:
    void setValue (float percent) noexcept { value = percent; repaint(); }
    void paint (juce::Graphics&) override;
private:
    float value = 50.0f;
};

class AuraBigHeatRing final : public juce::Component
{
public:
    void setHeat (float heatLevel) noexcept { heat = juce::jlimit (0.0f, 1.0f, heatLevel); repaint(); }
    void paint (juce::Graphics&) override;
private:
    float heat = 0.0f;
};

class AuraBigStageLed final : public juce::Component
{
public:
    explicit AuraBigStageLed (juce::String stageLabel);
    void setState (float heatLevel, bool bypassed, bool hardClip) noexcept;
    void paint (juce::Graphics&) override;
private:
    juce::String label;
    float heat = 0.0f;
    bool bypassed = false;
    bool hardClip = false;
};

class HorizontalReductionMeter final : public juce::Component, private juce::Timer
{
public:
    explicit HorizontalReductionMeter (juce::String titleText);
    void setTargetDb (float reductionDb) noexcept;
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override;
    juce::String title;
    std::atomic<float> target { 0.0f };
    float displayed = 0.0f;
};

class VerticalReductionMeter final : public juce::Component, private juce::Timer
{
public:
    explicit VerticalReductionMeter (juce::String titleText);
    void setTargetDb (float reductionDb) noexcept;
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override;
    juce::String title;
    std::atomic<float> target { 0.0f };
    float displayed = 0.0f;
};

class SegmentedChoiceBar final : public juce::Component
{
public:
    void setChoices (juce::StringArray labels);
    void setSelectedIndex (int index, juce::NotificationType notification = juce::sendNotification);
    int getSelectedIndex() const noexcept { return selectedIndex; }
    std::function<void (int)> onChange;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::StringArray choices;
    int selectedIndex = 0;
};

class AuraHorizontalVUMeter final : public juce::Component, private juce::Timer
{
public:
    enum class MeterMode { Input, Output, GainReduction, Saturation };
    explicit AuraHorizontalVUMeter (MeterMode m = MeterMode::Output);
    void setMode (MeterMode newMode) noexcept;
    void setTargetDb (float db) noexcept;
    void setSaturation (float amount) noexcept;
    void setDistortionWarning (bool warning) noexcept;
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override;
    MeterMode mode;
    float targetDb = -60.0f, displayedDb = -60.0f;
    float targetSat = 0.0f,  displayedSat = 0.0f;
    bool  distortionWarning = false;
};
