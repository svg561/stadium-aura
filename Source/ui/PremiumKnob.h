#pragma once

#include <JuceHeader.h>

class PremiumKnob final : public juce::Slider
{
public:
    enum class SizeTier { Hero, Major, Secondary, Small };

    PremiumKnob (juce::String labelText, double defaultValue, juce::String suffix = {});
    void setHeroStyle (bool shouldBeHero) noexcept;
    bool isHeroStyle() const noexcept { return heroStyle; }
    void setHideNameLabel (bool shouldHide) noexcept;
    void setSizeTier (SizeTier tier) noexcept { sizeTier = tier; }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void resized() override;
    void paintOverChildren (juce::Graphics&) override;

private:
    void updateDragTooltip();
    int labelHeightForTier() const noexcept;
    int valueHeightForTier() const noexcept;

    juce::Label nameLabel;
    juce::Label valueLabel;
    juce::Label dragValueLabel;
    double resetValue = 0.0;
    int normalSensitivity = 250;
    bool heroStyle = false;
    bool hideNameLabel = false;
    bool dragging = false;
    SizeTier sizeTier = SizeTier::Major;
};
