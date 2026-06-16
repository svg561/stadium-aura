#include "PremiumKnob.h"
#include "RackDrawing.h"

PremiumKnob::PremiumKnob (juce::String labelText, double defaultValue, juce::String)
    : resetValue (defaultValue)
{
    setName (labelText);
    setTooltip (labelText);
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setScrollWheelEnabled (true);
    setDoubleClickReturnValue (true, defaultValue);
    setVelocityBasedMode (false);
    setSliderSnapsToMousePosition (false);
    setMouseDragSensitivity (normalSensitivity);
    setRotaryParameters (juce::MathConstants<float>::pi * 1.20f,
                         juce::MathConstants<float>::pi * 2.80f, true);
    setTextValueSuffix ({});

    nameLabel.setText (labelText.toUpperCase(), juce::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setColour (juce::Label::textColourId, RackDrawing::Palette::textSecondary());
    nameLabel.setFont (juce::FontOptions (heroStyle ? 13.5f : 11.5f, juce::Font::bold).withKerningFactor (heroStyle ? 0.10f : 0.04f));
    nameLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (nameLabel);

    valueLabel.setInterceptsMouseClicks (false, false);
    valueLabel.setJustificationType (juce::Justification::centred);
    valueLabel.setColour (juce::Label::textColourId, RackDrawing::Palette::accentGold());
    valueLabel.setColour (juce::Label::backgroundColourId, juce::Colour (0xff101418));
    valueLabel.setFont (juce::FontOptions (heroStyle ? 13.0f : 11.0f, juce::Font::bold));
    addAndMakeVisible (valueLabel);

    dragValueLabel.setInterceptsMouseClicks (false, false);
    dragValueLabel.setJustificationType (juce::Justification::centred);
    dragValueLabel.setColour (juce::Label::backgroundColourId, juce::Colour (0xe0101418));
    dragValueLabel.setColour (juce::Label::textColourId, RackDrawing::Palette::accentGold());
    dragValueLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    dragValueLabel.setVisible (false);
    addChildComponent (dragValueLabel);

    onValueChange = [this]
    {
        valueLabel.setText (getTextFromValue (getValue()), juce::dontSendNotification);
        setTooltip (getName() + ": " + getTextFromValue (getValue()));
        if (dragging)
            updateDragTooltip();
    };
    valueLabel.setText (getTextFromValue (getValue()), juce::dontSendNotification);
}

void PremiumKnob::setHeroStyle (bool shouldBeHero) noexcept
{
    heroStyle = shouldBeHero;
    sizeTier = shouldBeHero ? SizeTier::Hero : sizeTier;
    nameLabel.setFont (juce::FontOptions (shouldBeHero ? 13.5f : 11.5f, juce::Font::bold).withKerningFactor (shouldBeHero ? 0.10f : 0.04f));
    valueLabel.setFont (juce::FontOptions (shouldBeHero ? 26.0f : 11.0f, juce::Font::bold));
    resized();
    repaint();
}

void PremiumKnob::setHideNameLabel (bool shouldHide) noexcept
{
    hideNameLabel = shouldHide;
    nameLabel.setVisible (! shouldHide);
    resized();
}

int PremiumKnob::labelHeightForTier() const noexcept
{
    switch (sizeTier)
    {
        case SizeTier::Hero: return 16;
        case SizeTier::Major: return 14;
        case SizeTier::Secondary: return 13;
        case SizeTier::Small: return 12;
        default: return 14;
    }
}

int PremiumKnob::valueHeightForTier() const noexcept
{
    switch (sizeTier)
    {
        case SizeTier::Hero: return 28;
        case SizeTier::Major: return 18;
        case SizeTier::Secondary: return 16;
        case SizeTier::Small: return 14;
        default: return 18;
    }
}

void PremiumKnob::updateDragTooltip()
{
    dragValueLabel.setText (getTextFromValue (getValue()), juce::dontSendNotification);
    dragValueLabel.setVisible (true);
}

void PremiumKnob::mouseDown (const juce::MouseEvent& event)
{
    juce::Slider::mouseDown (event);
    dragging = true;
    updateDragTooltip();
}

void PremiumKnob::mouseDrag (const juce::MouseEvent& event)
{
    setMouseDragSensitivity (event.mods.isShiftDown() ? normalSensitivity * 5 : normalSensitivity);
    juce::Slider::mouseDrag (event);
    updateDragTooltip();
}

void PremiumKnob::mouseDoubleClick (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);
    setValue (resetValue, juce::sendNotificationSync);
}

void PremiumKnob::mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    auto precise = wheel;
    precise.deltaY *= event.mods.isShiftDown() ? 0.05f : 0.15f;
    precise.deltaX *= event.mods.isShiftDown() ? 0.05f : 0.15f;
    juce::Slider::mouseWheelMove (event, precise);
}

void PremiumKnob::mouseEnter (const juce::MouseEvent& e)
{
    juce::Slider::mouseEnter (e);
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void PremiumKnob::mouseExit (const juce::MouseEvent& e)
{
    juce::Slider::mouseExit (e);
    dragging = false;
    dragValueLabel.setVisible (false);
}

void PremiumKnob::paintOverChildren (juce::Graphics& g)
{
    auto valueBounds = valueLabel.getBounds().toFloat().reduced (1.0f);
    g.setColour (RackDrawing::Palette::insetBorder());
    g.drawRoundedRectangle (valueBounds, 3.0f, 0.8f);

    if (! dragging || ! dragValueLabel.isVisible())
        return;

    auto tip = dragValueLabel.getBounds().toFloat();
    g.setColour (RackDrawing::Palette::accentGold().withAlpha (0.85f));
    g.drawRoundedRectangle (tip, 4.0f, 0.8f);
}

void PremiumKnob::resized()
{
    auto area = getLocalBounds();
    const int labelH = hideNameLabel ? 0 : labelHeightForTier();
    const int valueH = valueHeightForTier();
    if (! hideNameLabel)
        nameLabel.setBounds (area.removeFromTop (labelH));
    else
        nameLabel.setBounds ({});
    valueLabel.setBounds (area.removeFromBottom (valueH));
    dragValueLabel.setBounds (area.withSizeKeepingCentre (juce::jmin (area.getWidth(), 96), 20).translated (0, -8));
    juce::Slider::resized();
}
