#include "PremiumKnob.h"

PremiumKnob::PremiumKnob (juce::String labelText, double defaultValue, juce::String suffix)
    : resetValue (defaultValue)
{
    setName (labelText);
    setTooltip (labelText + ": " + juce::String (defaultValue, 1) + suffix);
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 20);
    setScrollWheelEnabled (true);
    setDoubleClickReturnValue (true, defaultValue);
    setVelocityBasedMode (false);
    setMouseDragSensitivity (normalSensitivity);
    setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                         juce::MathConstants<float>::pi * 2.75f, true);
    setTextValueSuffix (suffix);

    nameLabel.setText (labelText, juce::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setColour (juce::Label::textColourId, juce::Colour (0xffd8ccb0));
    nameLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    nameLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (nameLabel);

    onValueChange = [this]
    {
        setTooltip (getName() + ": " + getTextFromValue (getValue()));
    };
}

void PremiumKnob::mouseDown (const juce::MouseEvent& event)
{
    if (event.mods.isCommandDown() || event.mods.isCtrlDown())
    {
        setValue (resetValue, juce::sendNotificationSync);
        return;
    }
    juce::Slider::mouseDown (event);
}

void PremiumKnob::mouseDrag (const juce::MouseEvent& event)
{
    setMouseDragSensitivity (event.mods.isShiftDown() ? normalSensitivity * 5 : normalSensitivity);
    juce::Slider::mouseDrag (event);
}

void PremiumKnob::mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    auto precise = wheel;
    precise.deltaY *= event.mods.isShiftDown() ? 0.05f : 0.2f;
    precise.deltaX *= event.mods.isShiftDown() ? 0.05f : 0.2f;
    juce::Slider::mouseWheelMove (event, precise);
}

void PremiumKnob::resized()
{
    juce::Slider::resized();
    nameLabel.setBounds (getLocalBounds().removeFromTop (18));
}
