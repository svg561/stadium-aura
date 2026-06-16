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
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool down, int, int, int, int,
                       juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label& labelToPosition) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void drawLabel (juce::Graphics&, juce::Label&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle,
                           juce::Slider&) override;
};
