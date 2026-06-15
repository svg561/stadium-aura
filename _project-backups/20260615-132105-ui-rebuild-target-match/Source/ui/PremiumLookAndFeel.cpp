#include "PremiumLookAndFeel.h"

PremiumLookAndFeel::PremiumLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffead9aa));
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff11151a));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff3d3528));
    setColour (juce::ComboBox::textColourId, juce::Colour (0xffead9aa));
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff151a20));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff151a20));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xffead9aa));
}

void PremiumLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                            float sliderPos, float, float, juce::Slider::SliderStyle style,
                                            juce::Slider&)
{
    auto b = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                     static_cast<float> (width), static_cast<float> (height)).reduced (8.0f);
    const auto vertical = style == juce::Slider::LinearVertical;
    g.setColour (juce::Colour (0xff080a0b));
    if (vertical) g.fillRoundedRectangle ({ b.getCentreX() - 3.0f, b.getY(), 6.0f, b.getHeight() }, 3.0f);
    else g.fillRoundedRectangle ({ b.getX(), b.getCentreY() - 3.0f, b.getWidth(), 6.0f }, 3.0f);
    g.setColour (juce::Colour (0xffc48935));
    auto thumb = vertical ? juce::Rectangle<float> (b.getCentreX() - 14.0f, sliderPos - 8.0f, 28.0f, 16.0f)
                          : juce::Rectangle<float> (sliderPos - 8.0f, b.getCentreY() - 14.0f, 16.0f, 28.0f);
    g.fillRoundedRectangle (thumb, 3.0f);
    g.setColour (juce::Colour (0xffffd98a));
    g.drawRoundedRectangle (thumb, 3.0f, 1.0f);
}

void PremiumLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float position, float start, float end, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                          static_cast<float> (width), static_cast<float> (height)).reduced (8.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle = start + position * (end - start);

    const auto hero = slider.getName() == "AURA";
    if (hero)
    {
        g.setColour (juce::Colour (0x554e2500));
        g.fillEllipse (bounds.expanded (10.0f));
        g.setColour (juce::Colour (0xffd59a48));
        g.drawEllipse (bounds.expanded (5.0f), 3.0f);
        g.setColour (juce::Colour (0xffffe2a4));
        g.drawEllipse (bounds.expanded (1.0f), 2.0f);
    }

    juce::ColourGradient shadow (juce::Colour (0xff050607), centre.x, centre.y + radius,
                                 hero ? juce::Colour (0xff40382c) : juce::Colour (0xff2e3338),
                                 centre.x, centre.y - radius, false);
    g.setGradientFill (shadow);
    g.fillEllipse (bounds);
    g.setColour (hero ? juce::Colour (0xffc5904a) : juce::Colour (0xff695b3a));
    g.drawEllipse (bounds, hero ? 2.0f : 1.2f);

    for (int tick = 0; tick <= (hero ? 20 : 10); ++tick)
    {
        const auto tickAngle = juce::jmap (static_cast<float> (tick), 0.0f, static_cast<float> (hero ? 20 : 10), start, end);
        const auto inner = radius + (hero ? 11.0f : 5.0f);
        const auto outer = inner + (tick % 5 == 0 ? 8.0f : 4.0f);
        g.setColour (tick <= static_cast<int> (position * (hero ? 20 : 10))
                         ? juce::Colour (0xffe4ad59) : juce::Colour (0xff65563b));
        g.drawLine (centre.x + std::sin (tickAngle) * inner,
                    centre.y - std::cos (tickAngle) * inner,
                    centre.x + std::sin (tickAngle) * outer,
                    centre.y - std::cos (tickAngle) * outer,
                    tick % 5 == 0 ? 1.7f : 1.0f);
    }

    juce::Path arc;
    arc.addCentredArc (centre.x, centre.y, radius + 4.0f, radius + 4.0f, 0.0f, start, angle, true);
    g.setColour (juce::Colour (0xffd3aa55));
    g.strokePath (arc, juce::PathStrokeType (hero ? 4.0f : 2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.2f, -radius * 0.72f, 2.4f, radius * 0.34f, 1.2f);
    g.setColour (juce::Colour (0xfff3dfaa));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
}

void PremiumLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool highlighted, bool down)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    const auto active = button.getToggleState();
    g.setColour (active ? juce::Colour (0xffb68a3d) : juce::Colour (0xff20262c));
    g.fillRoundedRectangle (bounds, 5.0f);
    g.setColour (highlighted || down ? juce::Colour (0xfff3dfaa) : juce::Colour (0xff766b54));
    g.drawRoundedRectangle (bounds, 5.0f, 1.0f);
    g.setColour (active ? juce::Colour (0xff0d1013) : juce::Colour (0xffded4bb));
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawFittedText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, 1);
}

void PremiumLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int,
                                       juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height)).reduced (1.0f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (bounds, 5.0f);
    g.setColour (juce::Colour (0xff695b3a));
    g.drawRoundedRectangle (bounds, 5.0f, 1.0f);
    juce::Path arrow;
    arrow.addTriangle (static_cast<float> (width - 18), height * 0.42f,
                       static_cast<float> (width - 10), height * 0.42f,
                       static_cast<float> (width - 14), height * 0.62f);
    g.setColour (juce::Colour (0xffd3aa55));
    g.fillPath (arrow);
}
