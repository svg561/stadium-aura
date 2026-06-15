#include "RackComponents.h"

void SectionPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient panel (juce::Colour (0xff242320), bounds.getX(), bounds.getY(),
                                juce::Colour (0xff090b0d), bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill (panel);
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (juce::Colour (0xff654d31));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.2f);
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
    const auto alpha = 0.18f + activity * 0.76f;
    for (int i = 0; i < 3; ++i)
    {
        auto tube = juce::Rectangle<float> (b.getX() + b.getWidth() * (0.13f + i * 0.29f), b.getY() + 10.0f,
                                            b.getWidth() * 0.22f, b.getHeight() - 20.0f);
        g.setColour (juce::Colour::fromFloatRGBA (1.0f, 0.25f, 0.02f, alpha * 0.25f));
        g.fillEllipse (tube.expanded (8.0f, 3.0f));
        juce::ColourGradient glass (juce::Colour::fromFloatRGBA (0.95f, 0.38f, 0.08f, alpha), tube.getCentreX(), tube.getBottom(),
                                    juce::Colour (0x2220100a), tube.getCentreX(), tube.getY(), false);
        g.setGradientFill (glass);
        g.fillRoundedRectangle (tube, tube.getWidth() * 0.4f);
        g.setColour (juce::Colour (0xaaffb14d));
        g.drawRoundedRectangle (tube, tube.getWidth() * 0.4f, 1.0f);
        g.drawLine (tube.getCentreX(), tube.getY() + 12.0f, tube.getCentreX(), tube.getBottom() - 10.0f, 1.2f);
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
    g.setColour (juce::Colour (0xffb6aa91));
    g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    g.drawText ("REAL TILT EQ", b.removeFromTop (16.0f), juce::Justification::centredRight);
}
