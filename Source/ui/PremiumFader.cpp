#include "PremiumFader.h"

PremiumFader::PremiumFader (juce::String label, double defaultValue, juce::String suffix, bool isHorizontal)
    : resetValue (defaultValue), horizontal (isHorizontal)
{
    setName (label);
    setSliderStyle (horizontal ? juce::Slider::LinearHorizontal : juce::Slider::LinearVertical);
    setTextBoxStyle (horizontal ? juce::Slider::NoTextBox : juce::Slider::TextBoxBelow, false, 72, 20);
    setTextValueSuffix (suffix);
    setScrollWheelEnabled (true);
    setDoubleClickReturnValue (true, defaultValue);
    setVelocityBasedMode (true);
    setVelocityModeParameters (0.55, 1, 0.08, false, juce::ModifierKeys::noModifiers);
    nameLabel.setText (label, juce::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setColour (juce::Label::textColourId, juce::Colour (0xffd8ccb0));
    nameLabel.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    nameLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (nameLabel);
    onValueChange = [this] { setTooltip (getName() + ": " + getTextFromValue (getValue())); };
}

void PremiumFader::mouseDown (const juce::MouseEvent& event)
{
    if (event.mods.isCommandDown() || event.mods.isCtrlDown())
    {
        setValue (resetValue, juce::sendNotificationSync);
        return;
    }
    juce::Slider::mouseDown (event);
}

void PremiumFader::mouseDrag (const juce::MouseEvent& event)
{
    setMouseDragSensitivity (event.mods.isShiftDown() ? normalSensitivity * 6 : normalSensitivity);
    juce::Slider::mouseDrag (event);
}

void PremiumFader::mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    auto precise = wheel;
    precise.deltaY *= event.mods.isShiftDown() ? 0.04f : 0.18f;
    juce::Slider::mouseWheelMove (event, precise);
}

void PremiumFader::setHorizontal (bool shouldBeHorizontal) noexcept
{
    horizontal = shouldBeHorizontal;
    setSliderStyle (horizontal ? juce::Slider::LinearHorizontal : juce::Slider::LinearVertical);
    setTextBoxStyle (horizontal ? juce::Slider::NoTextBox : juce::Slider::TextBoxBelow, false, 72, 20);
    resized();
}

void PremiumFader::resized()
{
    juce::Slider::resized();
    if (horizontal)
        nameLabel.setBounds (getLocalBounds().removeFromLeft (juce::jmax (52, getWidth() / 5)));
    else
        nameLabel.setBounds (getLocalBounds().removeFromTop (18));
}
