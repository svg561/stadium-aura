#pragma once

#include <JuceHeader.h>

class PremiumFader final : public juce::Slider
{
public:
    PremiumFader (juce::String label, double defaultValue, juce::String suffix, bool horizontal = false);
    void setHorizontal (bool shouldBeHorizontal) noexcept;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void resized() override;

private:
    juce::Label nameLabel;
    double resetValue = 0.0;
    int normalSensitivity = 280;
    bool horizontal = false;
};
