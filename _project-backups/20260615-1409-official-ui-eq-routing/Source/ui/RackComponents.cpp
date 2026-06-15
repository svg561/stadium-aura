#include "RackComponents.h"

void RackModulePanel::setRouteVisualState (bool highlighted, bool dimmed) noexcept
{
    routeHighlighted = highlighted;
    routeDimmed = dimmed;
    repaint();
}

void RackModulePanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setOpacity (routeDimmed ? 0.42f : 1.0f);
    RackDrawing::paintRackModule (g, bounds, title, routeHighlighted ? juce::Colour (0xffff9a2e) : juce::Colour (0xffc58a38));
    if (routeHighlighted)
    {
        g.setColour (juce::Colour (0x33ff9a2e));
        g.drawRoundedRectangle (bounds.reduced (1.0f), 5.0f, 2.0f);
    }
    g.setOpacity (1.0f);
}

void NavRouteButton::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown() || e.mods.isPopupMenu())
    {
        if (onRightClick != nullptr)
            onRightClick();
        return;
    }
    juce::TextButton::mouseDown (e);
}

void NavRouteButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    const auto active = getToggleState();
    if (active)
    {
        g.setColour (juce::Colour (0x55ff9a2e));
        g.fillRoundedRectangle (b.expanded (3.0f), 5.0f);
    }
    juce::ColourGradient fill (active ? juce::Colour (0xffa86818) : juce::Colour (0xff181b1f),
                               b.getX(), b.getY(), active ? juce::Colour (0xff4a3010) : juce::Colour (0xff0a0c0e),
                               b.getRight(), b.getBottom(), false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (b, 4.0f);
    g.setColour (active ? juce::Colour (0xffffd27a) : juce::Colour (0xff5f5340));
    g.drawRoundedRectangle (b, 4.0f, active ? 1.4f : 0.9f);
    g.setColour (active ? juce::Colour (0xff100c08) : juce::Colour (0xffd8c7a4));
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    g.drawText (getButtonText(), b.toNearestInt(), juce::Justification::centred);
}

IconBarButton::IconBarButton (juce::String label, bool enabled)
    : juce::TextButton (std::move (label))
{
    setEnabled (enabled);
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1a1d21));
    setColour (juce::TextButton::textColourOffId, juce::Colour (0xffd8b878));
}

DisabledFeatureButton::DisabledFeatureButton (juce::String text, juce::String reason)
    : juce::TextButton (std::move (text))
{
    setEnabled (false);
    setTooltip (reason);
    setColour (juce::TextButton::textColourOffId, juce::Colour (0xff5a5348));
}

VerticalRmsMeter::VerticalRmsMeter (juce::String titleText) : title (std::move (titleText))
{
    startTimerHz (60);
}

void VerticalRmsMeter::setTarget (float linearGain) noexcept
{
    target.store (linearGain, std::memory_order_relaxed);
}

void VerticalRmsMeter::timerCallback()
{
    const auto next = target.load (std::memory_order_relaxed);
    displayed += (next - displayed) * (next > displayed ? 0.35f : 0.08f);
    repaint();
}

void VerticalRmsMeter::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (2.0f);
    RackDrawing::paintInsetDisplay (g, b);
    auto label = b.removeFromBottom (16.0f);
    auto scale = b.removeFromLeft (18.0f);
    auto meter = b.reduced (3.0f, 2.0f);
    g.setColour (juce::Colour (0xff6f6250));
    g.setFont (juce::FontOptions (7.0f));
    for (int db = 0; db >= -50; db -= 10)
    {
        const auto y = juce::jmap (static_cast<float> (db), 0.0f, -50.0f, meter.getY(), meter.getBottom());
        g.drawText (juce::String (db), scale.withY (y - 5.0f).withHeight (10.0f), juce::Justification::centredRight);
        g.setColour (juce::Colour (0x18ffffff));
        g.drawHorizontalLine (juce::roundToInt (y), meter.getX(), meter.getRight());
        g.setColour (juce::Colour (0xff6f6250));
    }
    const auto db = juce::Decibels::gainToDecibels (displayed, -60.0f);
    const auto norm = juce::jlimit (0.0f, 1.0f, juce::jmap (db, -50.0f, 0.0f, 0.0f, 1.0f));
    auto fill = meter.removeFromBottom (meter.getHeight() * norm);
    juce::ColourGradient grad (juce::Colour (0xff6fcf4a), fill.getX(), fill.getBottom(),
                               juce::Colour (0xffffb347), fill.getX(), fill.getY(), false);
    grad.addColour (0.75, juce::Colour (0xffe85a2f));
    g.setGradientFill (grad);
    g.fillRoundedRectangle (fill, 2.0f);
    g.setColour (juce::Colour (0xffd8b878));
    g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
    g.drawText (title, label, juce::Justification::centred);
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
    displayed += (target - displayed) * (target > displayed ? 0.28f : 0.07f);
    repaint();
}

void VuMeterComponent::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (2.0f);
    RackDrawing::paintInsetDisplay (g, b);
    auto face = b.reduced (6.0f, 8.0f);
    juce::ColourGradient paper (juce::Colour (0xffffe8b8), face.getX(), face.getY(),
                                juce::Colour (0xff8a5f28), face.getX(), face.getBottom(), false);
    g.setGradientFill (paper);
    g.fillRoundedRectangle (face, 6.0f);
    const auto centre = juce::Point<float> (face.getCentreX(), face.getBottom() + 10.0f);
    const auto radius = face.getWidth() * 0.46f;
    for (int i = 0; i <= 12; ++i)
    {
        const auto a = juce::jmap (static_cast<float> (i), 0.0f, 12.0f, -1.15f, 1.15f);
        const auto inner = centre + juce::Point<float> (std::sin (a), -std::cos (a)) * (radius - 10.0f);
        const auto outer = centre + juce::Point<float> (std::sin (a), -std::cos (a)) * radius;
        g.setColour (juce::Colour (0xff3a2410));
        g.drawLine ({ inner, outer }, i % 3 == 0 ? 2.0f : 1.0f);
        if (i % 3 == 0)
        {
            const auto labelPos = centre + juce::Point<float> (std::sin (a), -std::cos (a)) * (radius - 18.0f);
            g.setFont (juce::FontOptions (7.0f, juce::Font::bold));
            g.drawText (juce::String (i * (mode == Mode::gainReduction ? 2 : 5)),
                        juce::roundToInt (labelPos.x - 8.0f), juce::roundToInt (labelPos.y - 4.0f), 16, 8,
                        juce::Justification::centred);
        }
    }
    const auto maximum = mode == Mode::gainReduction ? 20.0f : 60.0f;
    const auto angle = juce::jmap (juce::jlimit (0.0f, maximum, displayed), 0.0f, maximum, 1.12f, -1.12f);
    const auto tip = centre + juce::Point<float> (std::sin (angle), -std::cos (angle)) * (radius - 14.0f);
    g.setColour (juce::Colour (0x88000000));
    g.drawLine ({ centre.translated (1.0f, 1.0f), tip.translated (1.0f, 1.0f) }, 2.5f);
    g.setColour (juce::Colour (0xff6a1208));
    g.drawLine ({ centre, tip }, 2.2f);
    g.setColour (juce::Colour (0xff1a1008));
    g.fillEllipse (centre.x - 6.0f, centre.y - 6.0f, 12.0f, 12.0f);
    g.setColour (juce::Colour (0xffc9a050));
    g.drawEllipse (centre.x - 5.0f, centre.y - 5.0f, 10.0f, 10.0f, 1.0f);
    g.setColour (juce::Colour (0xff2a1c10));
    g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    g.drawText (mode == Mode::gainReduction ? "VU GR" : (mode == Mode::input ? "INPUT" : "OUTPUT"),
                face.removeFromBottom (18.0f), juce::Justification::centred);
}

void TubeChamberComponent::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (3.0f);
    RackDrawing::paintInsetDisplay (g, b);
    auto window = b.reduced (8.0f, 6.0f);
    juce::ColourGradient glass (juce::Colour (0x33181820), window.getX(), window.getY(),
                                juce::Colour (0x88080a10), window.getRight(), window.getBottom(), false);
    g.setGradientFill (glass);
    g.fillRoundedRectangle (window, 6.0f);
    g.setColour (juce::Colour (0x44ffffff));
    g.drawRoundedRectangle (window.reduced (1.0f), 5.0f, 0.8f);
    const auto alpha = 0.25f + activity * 0.75f;
    for (int i = 0; i < 3; ++i)
    {
        auto tube = juce::Rectangle<float> (window.getX() + window.getWidth() * (0.12f + i * 0.30f),
                                            window.getY() + 14.0f, window.getWidth() * 0.20f, window.getHeight() - 28.0f);
        g.setColour (juce::Colour::fromFloatRGBA (1.0f, 0.45f, 0.05f, alpha * 0.45f));
        g.fillEllipse (tube.expanded (12.0f, 5.0f));
        juce::ColourGradient tubeGrad (juce::Colour::fromFloatRGBA (1.0f, 0.62f, 0.14f, alpha), tube.getCentreX(), tube.getBottom(),
                                       juce::Colour (0x44140804), tube.getCentreX(), tube.getY(), false);
        g.setGradientFill (tubeGrad);
        g.fillRoundedRectangle (tube, tube.getWidth() * 0.45f);
        g.setColour (juce::Colour (0xccffe08a));
        g.drawRoundedRectangle (tube, tube.getWidth() * 0.45f, 1.2f);
        g.setColour (juce::Colour::fromFloatRGBA (1.0f, 0.85f, 0.45f, alpha * 0.8f));
        g.fillEllipse (tube.getCentreX() - 3.0f, tube.getY() + tube.getHeight() * 0.15f, 6.0f, 16.0f);
        g.setColour (juce::Colour (0xff8a6030));
        g.drawLine (tube.getCentreX(), tube.getY() + 10.0f, tube.getCentreX(), tube.getBottom() - 8.0f, 1.3f);
    }
    g.setColour (juce::Colour (0xffb88848));
    g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
    g.drawText ("TUBE CHAMBER", b.removeFromTop (14.0f), juce::Justification::centred);
}

void EqSpectrumComponent::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (3.0f);
    RackDrawing::paintInsetDisplay (g, b);
    auto plot = b.reduced (8.0f, 10.0f);
    for (int i = 1; i < 8; ++i)
    {
        g.setColour (juce::Colour (0x12ffffff));
        g.drawVerticalLine (juce::roundToInt (plot.getX() + plot.getWidth() * i / 8.0f), plot.getY(), plot.getBottom());
    }
    for (int i = 1; i < 5; ++i)
        g.drawHorizontalLine (juce::roundToInt (plot.getY() + plot.getHeight() * i / 5.0f), plot.getX(), plot.getRight());
    juce::Path spectrum;
    for (int x = 0; x < juce::roundToInt (plot.getWidth()); ++x)
    {
        const auto n = static_cast<float> (x) / plot.getWidth();
        const auto y = plot.getCentreY() + std::sin (n * 18.0f) * plot.getHeight() * 0.08f
                     + std::sin (n * 41.0f + 1.2f) * plot.getHeight() * 0.04f;
        if (x == 0) spectrum.startNewSubPath (plot.getX(), y);
        else spectrum.lineTo (plot.getX() + x, y);
    }
    g.setColour (juce::Colour (0x33ff9a3a));
    g.strokePath (spectrum, juce::PathStrokeType (1.2f));
    juce::Path curve;
    for (int x = 0; x < juce::roundToInt (plot.getWidth()); ++x)
    {
        const auto n = static_cast<float> (x) / juce::jmax (1.0f, plot.getWidth() - 1.0f);
        const auto tilt = (n - 0.5f) * tone * 0.55f;
        const auto y = plot.getCentreY() - tilt * plot.getHeight();
        if (x == 0) curve.startNewSubPath (plot.getX(), y); else curve.lineTo (plot.getX() + x, y);
    }
    g.setColour (juce::Colour (0xffefab45));
    g.strokePath (curve, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved));
    const juce::Colour bandColours[] { juce::Colour (0xff4aa3ff), juce::Colour (0xff49d17a), juce::Colour (0xfff0c14d),
                                       juce::Colour (0xffff8a3d), juce::Colour (0xffff5d6e) };
    for (int i = 0; i < 5; ++i)
    {
        const auto x = plot.getX() + plot.getWidth() * (0.1f + i * 0.2f);
        const auto y = plot.getCentreY() - tone * plot.getHeight() * (0.12f + i * 0.05f);
        g.setColour (bandColours[i]);
        g.fillEllipse (x - 6.0f, y - 6.0f, 12.0f, 12.0f);
        g.setColour (juce::Colour (0x99ffffff));
        g.drawEllipse (x - 6.0f, y - 6.0f, 12.0f, 12.0f, 1.0f);
    }
    g.setColour (juce::Colour (0xffc9a060));
    g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    g.drawText ("EQ", b.removeFromTop (14.0f), juce::Justification::centredLeft);
}

StereoLrMeter::StereoLrMeter (juce::String titleText) : title (std::move (titleText)) { startTimerHz (60); }

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
    RackDrawing::paintInsetDisplay (g, b);
    g.setColour (juce::Colour (0xffb88848));
    g.setFont (juce::FontOptions (8.0f, juce::Font::bold));
    g.drawText (title, b.removeFromTop (12.0f), juce::Justification::centred);
    const auto barW = (b.getWidth() - 8.0f) * 0.5f;
    for (int ch = 0; ch < 2; ++ch)
    {
        auto bar = b.removeFromLeft (barW).reduced (2.0f, 1.0f);
        if (ch == 1) bar = b.reduced (2.0f, 1.0f);
        g.setColour (juce::Colour (0xff101316));
        g.fillRoundedRectangle (bar, 2.0f);
        const auto level = ch == 0 ? displayedLeft : displayedRight;
        auto fill = bar.withHeight (bar.getHeight() * juce::jlimit (0.0f, 1.0f, level));
        fill.setY (bar.getBottom() - fill.getHeight());
        juce::ColourGradient grad (juce::Colour (0xffffc15b), fill.getX(), fill.getBottom(),
                                   juce::Colour (0xffd4551f), fill.getX(), fill.getY(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (fill, 2.0f);
        g.setColour (juce::Colour (0xff8f7a58));
        g.setFont (juce::FontOptions (7.5f, juce::Font::bold));
        g.drawText (ch == 0 ? "L" : "R", bar.removeFromTop (10.0f), juce::Justification::centred);
    }
}

void HeroAuraRing::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const auto centre = b.getCentre();
    const auto radius = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    g.setColour (juce::Colour (0x44ff7a10));
    g.fillEllipse (b.expanded (8.0f));
    for (int tick = 0; tick <= 24; ++tick)
    {
        const auto a = juce::jmap (static_cast<float> (tick), 0.0f, 24.0f,
                                   juce::MathConstants<float>::pi * 1.15f,
                                   juce::MathConstants<float>::pi * 2.85f);
        const auto lit = tick <= static_cast<int> (value * 0.24f);
        const auto inner = radius + 6.0f;
        const auto outer = inner + (tick % 6 == 0 ? 10.0f : 5.0f);
        g.setColour (lit ? juce::Colour (0xffff9a2e) : juce::Colour (0xff4a3a28));
        g.drawLine (centre.x + std::sin (a) * inner, centre.y - std::cos (a) * inner,
                    centre.x + std::sin (a) * outer, centre.y - std::cos (a) * outer, tick % 6 == 0 ? 2.0f : 1.0f);
    }
    g.setColour (juce::Colour (0xffc58a28));
    g.fillEllipse (centre.x - 16.0f, centre.y - 16.0f, 32.0f, 32.0f);
    g.setColour (juce::Colour (0xff120c06));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText ("SA", juce::Rectangle<float> (centre.x - 16.0f, centre.y - 16.0f, 32.0f, 32.0f).toNearestInt(), juce::Justification::centred);
}

HorizontalReductionMeter::HorizontalReductionMeter (juce::String titleText) : title (std::move (titleText))
{
    startTimerHz (60);
}

void HorizontalReductionMeter::setTargetDb (float reductionDb) noexcept
{
    target.store (juce::jlimit (0.0f, 20.0f, reductionDb), std::memory_order_relaxed);
}

void HorizontalReductionMeter::timerCallback()
{
    const auto next = target.load (std::memory_order_relaxed);
    displayed += (next - displayed) * (next > displayed ? 0.32f : 0.10f);
    repaint();
}

void HorizontalReductionMeter::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (2.0f);
    RackDrawing::paintInsetDisplay (g, b);
    g.setColour (juce::Colour (0xffb88848));
    g.setFont (juce::FontOptions (8.0f, juce::Font::bold));
    g.drawText (title, b.removeFromTop (12.0f), juce::Justification::centredLeft);
    auto meter = b.reduced (4.0f, 2.0f);
    g.setColour (juce::Colour (0xff0a0c0e));
    g.fillRoundedRectangle (meter, 3.0f);
    const auto norm = juce::jlimit (0.0f, 1.0f, displayed / 12.0f);
    auto fill = meter.removeFromLeft (meter.getWidth() * norm);
    juce::ColourGradient grad (juce::Colour (0xffffc15b), fill.getX(), fill.getCentreY(),
                               juce::Colour (0xffe85a2f), fill.getRight(), fill.getCentreY(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (fill, 3.0f);
    g.setColour (juce::Colour (0xffd8b878));
    g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
    g.drawText (juce::String (displayed, 1) + " dB", meter, juce::Justification::centred);
}

void SegmentedChoiceBar::setChoices (juce::StringArray labels)
{
    choices = std::move (labels);
    resized();
    repaint();
}

void SegmentedChoiceBar::setSelectedIndex (int index, juce::NotificationType notification)
{
    selectedIndex = juce::jlimit (0, juce::jmax (0, choices.size() - 1), index);
    repaint();
    if (notification != juce::dontSendNotification && onChange != nullptr)
        onChange (selectedIndex);
}

void SegmentedChoiceBar::resized() {}

void SegmentedChoiceBar::mouseDown (const juce::MouseEvent& e)
{
    if (choices.isEmpty()) return;
    const auto index = juce::jlimit (0, choices.size() - 1,
                                     static_cast<int> (static_cast<float> (choices.size()) * e.position.x
                                                       / static_cast<float> (getWidth())));
    setSelectedIndex (index);
}

void SegmentedChoiceBar::paint (juce::Graphics& g)
{
    if (choices.isEmpty()) return;
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    const auto w = b.getWidth() / static_cast<float> (choices.size());
    for (int i = 0; i < choices.size(); ++i)
    {
        auto seg = b.removeFromLeft (w).reduced (1.0f);
        const auto active = i == selectedIndex;
        juce::ColourGradient fill (active ? juce::Colour (0xffa86818) : juce::Colour (0xff15181c),
                                   seg.getX(), seg.getY(), active ? juce::Colour (0xff3a2808) : juce::Colour (0xff080a0c),
                                   seg.getRight(), seg.getBottom(), false);
        g.setGradientFill (fill);
        g.fillRoundedRectangle (seg, 4.0f);
        g.setColour (active ? juce::Colour (0xffffd27a) : juce::Colour (0xff5a4e38));
        g.drawRoundedRectangle (seg, 4.0f, active ? 1.2f : 0.8f);
        g.setColour (active ? juce::Colour (0xff0d0a06) : juce::Colour (0xffe0d0a8));
        g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        g.drawFittedText (choices[i], seg.toNearestInt(), juce::Justification::centred, 1);
    }
}
