#include "PremiumKnob.h"

PremiumKnob::PremiumKnob (juce::String labelText, double defaultValue, juce::String)
    : resetValue (defaultValue)
{
    setName (labelText);
    setTooltip (labelText);
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 16);
    setScrollWheelEnabled (true);
    setDoubleClickReturnValue (true, defaultValue);
    setVelocityBasedMode (false);
    setMouseDragSensitivity (normalSensitivity);
    setRotaryParameters (juce::MathConstants<float>::pi * 1.20f,
                         juce::MathConstants<float>::pi * 2.80f, true);
    setTextValueSuffix ({});
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffead9aa));
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff0d1013));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff4a3d2c));

    nameLabel.setText (labelText, juce::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setColour (juce::Label::textColourId, juce::Colour (0xffb8a880));
    nameLabel.setFont (juce::FontOptions (9.5f, juce::Font::bold));
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
    setMouseDragSensitivity (event.mods.isShiftDown() ? normalSensitivity * 6 : normalSensitivity);
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
    auto area = getLocalBounds();
    nameLabel.setBounds (area.removeFromTop (14));
    juce::Slider::resized();
}
