#pragma once

#include <JuceHeader.h>

class PremiumKnob final : public juce::Slider
{
public:
    PremiumKnob (juce::String labelText, double defaultValue, juce::String suffix);
    void setHeroStyle (bool shouldBeHero) noexcept { heroStyle = shouldBeHero; repaint(); }
    bool isHeroStyle() const noexcept { return heroStyle; }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void resized() override;

private:
    juce::Label nameLabel;
    double resetValue = 0.0;
    int normalSensitivity = 250;
    bool heroStyle = false;
};
