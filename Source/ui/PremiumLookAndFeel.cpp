#include "PremiumLookAndFeel.h"
#include "PremiumKnob.h"
#include "RackDrawing.h"

PremiumLookAndFeel::PremiumLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffead9aa));
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff0d1013));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff4a3d2c));
    setColour (juce::ComboBox::textColourId, juce::Colour (0xffead9aa));
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff101418));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff101418));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xffead9aa));
}

void PremiumLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                            float sliderPos, float, float, juce::Slider::SliderStyle style,
                                            juce::Slider&)
{
    auto b = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                     static_cast<float> (width), static_cast<float> (height)).reduced (6.0f);
    const auto vertical = style == juce::Slider::LinearVertical;
    g.setColour (juce::Colour (0xff060809));
    if (vertical)
        g.fillRoundedRectangle ({ b.getCentreX() - 4.0f, b.getY(), 8.0f, b.getHeight() }, 4.0f);
    else
        g.fillRoundedRectangle ({ b.getX(), b.getCentreY() - 4.0f, b.getWidth(), 8.0f }, 4.0f);
    g.setColour (juce::Colour (0xff2a2118));
    if (vertical) g.drawRoundedRectangle ({ b.getCentreX() - 4.0f, b.getY(), 8.0f, b.getHeight() }, 4.0f, 1.0f);
    else g.drawRoundedRectangle ({ b.getX(), b.getCentreY() - 4.0f, b.getWidth(), 8.0f }, 4.0f, 1.0f);
    auto thumb = vertical ? juce::Rectangle<float> (b.getCentreX() - 16.0f, sliderPos - 9.0f, 32.0f, 18.0f)
                          : juce::Rectangle<float> (sliderPos - 9.0f, b.getCentreY() - 16.0f, 18.0f, 32.0f);
    juce::ColourGradient cap (juce::Colour (0xffffe6a8), thumb.getCentreX(), thumb.getY(),
                              juce::Colour (0xffb87328), thumb.getCentreX(), thumb.getBottom(), false);
    g.setGradientFill (cap);
    g.fillRoundedRectangle (thumb, 4.0f);
    g.setColour (juce::Colour (0xfffff0c8));
    g.drawRoundedRectangle (thumb, 4.0f, 1.0f);
}

void PremiumLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float position, float start, float end, juce::Slider& slider)
{
    const auto size = juce::jmin (width, height);
    const auto ox = static_cast<float> (x) + (static_cast<float> (width) - static_cast<float> (size)) * 0.5f;
    const auto oy = static_cast<float> (y) + (static_cast<float> (height) - static_cast<float> (size)) * 0.5f;
    auto bounds = juce::Rectangle<float> (ox, oy, static_cast<float> (size), static_cast<float> (size)).reduced (3.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle = start + position * (end - start);
    const auto hero = slider.getName() == "AURA";
    const auto small = radius < 24.0f;

    if (hero)
    {
        g.setColour (juce::Colour (0x44ff8a18));
        g.fillEllipse (bounds.expanded (10.0f));
        juce::Path glowArc;
        glowArc.addCentredArc (centre.x, centre.y, radius + 8.0f, radius + 8.0f, 0.0f, start, end, true);
        g.setColour (juce::Colour (0x33ff9a2e));
        g.strokePath (glowArc, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setColour (juce::Colour (0x99000000));
    g.fillEllipse (bounds.translated (0.0f, 2.5f));

    const auto capBounds = bounds.reduced (hero ? 4.0f : (small ? 7.0f : 5.0f));
    juce::ColourGradient cap (juce::Colour (0xff2a2e34), capBounds.getCentreX(), capBounds.getY() + 2.0f,
                              juce::Colour (0xff08090c), capBounds.getCentreX(), capBounds.getBottom(), false);
    cap.addColour (0.35, juce::Colour (0xff1a1d22));
    g.setGradientFill (cap);
    g.fillEllipse (capBounds);

    g.setColour (juce::Colour (0x28ffffff));
    g.drawEllipse (capBounds.reduced (1.0f), 1.0f);

    const auto ringBounds = bounds.reduced (1.0f);
    g.setColour (juce::Colour (0xff4a3d2c));
    g.drawEllipse (ringBounds, 1.2f);

    const auto tickCount = hero ? 20 : (small ? 7 : 11);
    for (int tick = 0; tick <= tickCount; ++tick)
    {
        const auto tickAngle = juce::jmap (static_cast<float> (tick), 0.0f, static_cast<float> (tickCount), start, end);
        const auto inner = radius - (small ? 2.0f : 4.0f);
        const auto outer = inner - (tick % (hero ? 5 : 3) == 0 ? (hero ? 7.0f : 4.0f) : (hero ? 3.0f : 2.0f));
        g.setColour (tick <= static_cast<int> (position * tickCount)
                         ? juce::Colour (0xffffb347) : juce::Colour (0xff3a3428));
        g.drawLine (centre.x + std::sin (tickAngle) * inner, centre.y - std::cos (tickAngle) * inner,
                    centre.x + std::sin (tickAngle) * outer, centre.y - std::cos (tickAngle) * outer,
                    tick % (hero ? 5 : 3) == 0 ? 1.4f : 0.8f);
    }

    juce::Path valueArc;
    valueArc.addCentredArc (centre.x, centre.y, radius - 1.0f, radius - 1.0f, 0.0f, start, angle, true);
    g.setColour (hero ? juce::Colour (0xffff9a2e) : juce::Colour (0xffc58a38));
    g.strokePath (valueArc, juce::PathStrokeType (hero ? 2.8f : 1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path pointer;
    const auto pointerLen = radius * (hero ? 0.42f : (small ? 0.36f : 0.40f));
    pointer.startNewSubPath (0.0f, 0.0f);
    pointer.lineTo (0.0f, -pointerLen);
    g.setColour (juce::Colour (0xfff8f0d8));
    g.strokePath (pointer, juce::PathStrokeType (hero ? 2.4f : 1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                  juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));

    if (hero)
    {
        g.setColour (juce::Colour (0xffc58a28));
        g.fillEllipse (centre.x - 12.0f, centre.y - 12.0f, 24.0f, 24.0f);
        g.setColour (juce::Colour (0xff100c06));
        g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        g.drawText ("SA", juce::Rectangle<float> (centre.x - 12.0f, centre.y - 12.0f, 24.0f, 24.0f).toNearestInt(),
                    juce::Justification::centred);
    }
}

void PremiumLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool highlighted, bool down)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    const auto active = button.getToggleState();
    if (active)
    {
        g.setColour (juce::Colour (0x44ff9a2e));
        g.fillRoundedRectangle (bounds.expanded (2.0f), 4.0f);
    }
    juce::ColourGradient fill (active ? juce::Colour (0xff8a5a18) : juce::Colour (0xff15181c),
                               bounds.getX(), bounds.getY(), active ? juce::Colour (0xff3a2808) : juce::Colour (0xff080a0c),
                               bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (highlighted || down || active ? juce::Colour (0xffffd27a) : juce::Colour (0xff5a4e38));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
    g.setColour (active ? juce::Colour (0xff0d0a06) : juce::Colour (0xffe0d0a8));
    g.setFont (juce::FontOptions (button.getButtonText().length() > 8 ? 9.0f : 10.0f, juce::Font::bold));
    g.drawFittedText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, 1);
}

void PremiumLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int,
                                       juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height)).reduced (1.0f);
    RackDrawing::paintInsetDisplay (g, bounds);
    g.setColour (box.findColour (juce::ComboBox::textColourId));
    g.setFont (juce::FontOptions (9.5f));
    g.drawFittedText (box.getText(), bounds.reduced (8.0f, 0.0f).withTrimmedRight (18.0f).toNearestInt(),
                      juce::Justification::centredLeft, 1);
    juce::Path arrow;
    arrow.addTriangle (static_cast<float> (width - 16), height * 0.40f,
                       static_cast<float> (width - 8), height * 0.40f,
                       static_cast<float> (width - 12), height * 0.62f);
    g.setColour (juce::Colour (0xffd3aa55));
    g.fillPath (arrow);
}
