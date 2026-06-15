#pragma once

#include <JuceHeader.h>

class PremiumLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    PremiumLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosition, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool down, int, int, int, int,
                       juce::ComboBox&) override;
};

