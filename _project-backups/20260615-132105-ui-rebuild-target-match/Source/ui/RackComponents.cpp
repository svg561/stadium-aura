#include "RackComponents.h"

namespace
{
void paintRoutingButton (juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& text,
                         bool active, bool highlighted, bool down)
{
    const auto glow = active ? juce::Colour (0x55ff9a2e) : juce::Colour (0x00000000);
    g.setColour (glow);
    g.fillRoundedRectangle (bounds.expanded (active ? 2.0f : 0.0f), 6.0f);
    juce::ColourGradient fill (active ? juce::Colour (0xff8f6428) : juce::Colour (0xff1a1d21),
                               bounds.getX(), bounds.getY(),
                               active ? juce::Colour (0xff3a2a14) : juce::Colour (0xff0b0d10),
                               bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (bounds, 5.0f);
    g.setColour (highlighted || down || active ? juce::Colour (0xffffd27a) : juce::Colour (0xff6f5d42));
    g.drawRoundedRectangle (bounds, 5.0f, 1.2f);
    g.setColour (active ? juce::Colour (0xff120f0b) : juce::Colour (0xffd8c7a4));
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText (text, bounds.toNearestInt(), juce::Justification::centred);
}
}

void RoutingButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    paintRoutingButton (g, getLocalBounds().toFloat().reduced (1.0f), getButtonText(),
                        getToggleState(), highlighted, down);
}

DisabledFeatureButton::DisabledFeatureButton (juce::String text, juce::String reason)
    : juce::TextButton (std::move (text))
{
    setEnabled (false);
    setTooltip (reason);
}

StereoLrMeter::StereoLrMeter (juce::String titleText) : title (std::move (titleText))
{
    startTimerHz (60);
}

void StereoLrMeter::setTargets (float leftLevel, float rightLevel) noexcept
{
    left.store (leftLevel, std::memory_order_relaxed);
    right.store (rightLevel, std::memory_order_relaxed);
}

void StereoLrMeter::timerCallback()
{
    displayedLeft += (left.load (std::memory_order_relaxed) - displayedLeft) * 0.28f;
    displayedRight += (right.load (std::memory_order_relaxed) - displayedRight) * 0.28f;
    repaint();
}

void StereoLrMeter::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (juce::Colour (0xff070808));
    g.fillRoundedRectangle (b, 4.0f);
    g.setColour (juce::Colour (0xffd0a24f));
    g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    g.drawText (title, b.removeFromTop (14.0f), juce::Justification::centred);
    const auto barWidth = (b.getWidth() - 6.0f) * 0.5f;
    for (int channel = 0; channel < 2; ++channel)
    {
        auto bar = b.removeFromLeft (barWidth).reduced (2.0f, 1.0f);
        if (channel == 1) bar = b.reduced (2.0f, 1.0f);
        g.setColour (juce::Colour (0xff15181c));
        g.fillRoundedRectangle (bar, 2.0f);
        const auto level = channel == 0 ? displayedLeft : displayedRight;
        auto fill = bar.withHeight (bar.getHeight() * juce::jlimit (0.0f, 1.0f, level));
        fill.setY (bar.getBottom() - fill.getHeight());
        juce::ColourGradient gradient (juce::Colour (0xffffc15b), fill.getX(), fill.getBottom(),
                                       juce::Colour (0xffd4551f), fill.getX(), fill.getY(), false);
        g.setGradientFill (gradient);
        g.fillRoundedRectangle (fill, 2.0f);
        g.setColour (juce::Colour (0xff8f7a58));
        g.setFont (juce::FontOptions (8.0f, juce::Font::bold));
        g.drawText (channel == 0 ? "L" : "R", bar.removeFromTop (10.0f), juce::Justification::centred);
    }
}

void SectionPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient panel (juce::Colour (0xff2a2724), bounds.getX(), bounds.getY(),
                                juce::Colour (0xff0a0c0e), bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill (panel);
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (juce::Colour (0xff7a5a2f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.4f);
    g.setColour (juce::Colour (0x18ffffff));
    g.drawHorizontalLine (juce::roundToInt (bounds.getY() + 2.0f), bounds.getX() + 8.0f, bounds.getRight() - 8.0f);
    auto heading = bounds.removeFromTop (28.0f).reduced (4.0f, 3.0f);
    g.setColour (juce::Colour (0xff121313));
    g.fillRoundedRectangle (heading, 3.0f);
    g.setColour (juce::Colour (0xffe0ad5c));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (title.toUpperCase(), heading, juce::Justification::centred);
}

VuMeterComponent::VuMeterComponent() { startTimerHz (60); }

void VuMeterComponent::setTargets (float inputGain, float reductionDb, float outputGain, Mode newMode) noexcept
{
    input.store (inputGain, std::memory_order_relaxed);
    reduction.store (reductionDb, std::memory_order_relaxed);
    output.store (outputGain, std::memory_order_relaxed);
    mode = newMode;
}

void VuMeterComponent::timerCallback()
{
    float target = reduction.load (std::memory_order_relaxed);
    if (mode == Mode::input) target = -juce::Decibels::gainToDecibels (input.load (std::memory_order_relaxed), -60.0f);
    if (mode == Mode::output) target = -juce::Decibels::gainToDecibels (output.load (std::memory_order_relaxed), -60.0f);
    target = juce::jlimit (0.0f, mode == Mode::gainReduction ? 20.0f : 60.0f, target);
    displayed += (target - displayed) * (target > displayed ? 0.22f : 0.06f);
    repaint();
}

void VuMeterComponent::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (3.0f);
    g.setColour (juce::Colour (0xff080908));
    g.fillRoundedRectangle (b, 7.0f);
    auto face = b.reduced (8.0f, 7.0f);
    juce::ColourGradient paper (juce::Colour (0xffffe0a2), face.getX(), face.getY(),
                                juce::Colour (0xff9d7138), face.getX(), face.getBottom(), false);
    g.setGradientFill (paper);
    g.fillRoundedRectangle (face, 5.0f);
    const auto centre = juce::Point<float> (face.getCentreX(), face.getBottom() + 8.0f);
    const auto radius = face.getWidth() * 0.43f;
    for (int i = 0; i <= 10; ++i)
    {
        const auto a = juce::jmap (static_cast<float> (i), 0.0f, 10.0f, -1.02f, 1.02f);
        const auto inner = centre + juce::Point<float> (std::sin (a), -std::cos (a)) * (radius - 8.0f);
        const auto outer = centre + juce::Point<float> (std::sin (a), -std::cos (a)) * radius;
        g.setColour (juce::Colour (0xff3c2815));
        g.drawLine ({ inner, outer }, i % 2 == 0 ? 1.5f : 0.8f);
    }
    const auto maximum = mode == Mode::gainReduction ? 20.0f : 60.0f;
    const auto angle = juce::jmap (juce::jlimit (0.0f, maximum, displayed), 0.0f, maximum, 1.02f, -1.02f);
    const auto tip = centre + juce::Point<float> (std::sin (angle), -std::cos (angle)) * (radius - 12.0f);
    g.setColour (juce::Colour (0xff5c160d));
    g.drawLine ({ centre, tip }, 2.0f);
    g.setColour (juce::Colour (0xff21150c));
    g.fillEllipse (centre.x - 5.0f, centre.y - 5.0f, 10.0f, 10.0f);
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText (mode == Mode::gainReduction ? "GAIN REDUCTION" : (mode == Mode::input ? "INPUT VU" : "OUTPUT VU"),
                face.removeFromBottom (25.0f), juce::Justification::centred);
}

void TubeChamberComponent::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (5.0f);
    g.setColour (juce::Colour (0xff070808));
    g.fillRoundedRectangle (b, 8.0f);
    g.setColour (juce::Colour (0xff2a2118));
    g.drawRoundedRectangle (b, 8.0f, 1.0f);
    const auto alpha = 0.22f + activity * 0.78f;
    for (int i = 0; i < 2; ++i)
    {
        auto tube = juce::Rectangle<float> (b.getX() + b.getWidth() * (0.18f + i * 0.38f), b.getY() + 12.0f,
                                            b.getWidth() * 0.28f, b.getHeight() - 24.0f);
        g.setColour (juce::Colour::fromFloatRGBA (1.0f, 0.42f, 0.05f, alpha * 0.35f));
        g.fillEllipse (tube.expanded (10.0f, 4.0f));
        juce::ColourGradient glass (juce::Colour::fromFloatRGBA (1.0f, 0.55f, 0.12f, alpha), tube.getCentreX(), tube.getBottom(),
                                    juce::Colour (0x33180d05), tube.getCentreX(), tube.getY(), false);
        g.setGradientFill (glass);
        g.fillRoundedRectangle (tube, tube.getWidth() * 0.42f);
        g.setColour (juce::Colour (0xccffd27a));
        g.drawRoundedRectangle (tube, tube.getWidth() * 0.42f, 1.2f);
        g.setColour (juce::Colour (0x99fff0b0));
        g.fillEllipse (tube.getCentreX() - 4.0f, tube.getY() + tube.getHeight() * 0.18f, 8.0f, 18.0f);
        g.drawLine (tube.getCentreX(), tube.getY() + 14.0f, tube.getCentreX(), tube.getBottom() - 12.0f, 1.4f);
        g.setFont (juce::FontOptions (8.0f, juce::Font::bold));
        g.setColour (juce::Colour (0xffd8b06a));
        g.drawText (i == 0 ? "12AX7" : "12AU7", tube.withTrimmedTop (tube.getHeight() * 0.72f), juce::Justification::centred);
    }
}

void CompactEqComponent::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (4.0f);
    g.setColour (juce::Colour (0xff050809));
    g.fillRoundedRectangle (b, 4.0f);
    g.setColour (juce::Colour (0xff26302f));
    for (int i = 1; i < 5; ++i) g.drawVerticalLine (juce::roundToInt (b.getX() + b.getWidth() * i / 5.0f), b.getY(), b.getBottom());
    for (int i = 1; i < 4; ++i) g.drawHorizontalLine (juce::roundToInt (b.getY() + b.getHeight() * i / 4.0f), b.getX(), b.getRight());
    juce::Path curve;
    for (int x = 0; x < juce::roundToInt (b.getWidth()); ++x)
    {
        const auto n = static_cast<float> (x) / juce::jmax (1.0f, b.getWidth() - 1.0f);
        const auto tilt = (n - 0.5f) * tone * 0.62f;
        const auto y = b.getCentreY() - tilt * b.getHeight();
        if (x == 0) curve.startNewSubPath (b.getX(), y); else curve.lineTo (b.getX() + x, y);
    }
    g.setColour (juce::Colour (0xffefab45));
    g.strokePath (curve, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved));
    const juce::Colour bandColours[] { juce::Colour (0xff4aa3ff), juce::Colour (0xff49d17a), juce::Colour (0xfff0c14d),
                                       juce::Colour (0xffff8a3d), juce::Colour (0xffff5d6e) };
    for (int i = 0; i < 5; ++i)
    {
        const auto x = b.getX() + b.getWidth() * (0.12f + i * 0.19f);
        const auto y = b.getCentreY() - tone * b.getHeight() * (0.18f + i * 0.04f);
        g.setColour (bandColours[i]);
        g.fillEllipse (x - 5.0f, y - 5.0f, 10.0f, 10.0f);
        g.setColour (juce::Colour (0x88ffffff));
        g.drawEllipse (x - 5.0f, y - 5.0f, 10.0f, 10.0f, 1.0f);
    }
    g.setColour (juce::Colour (0xffb6aa91));
    g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    g.drawText ("REAL TILT EQ", b.removeFromTop (16.0f), juce::Justification::centredRight);
}
