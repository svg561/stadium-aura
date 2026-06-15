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

    // Reserve label area at the bottom, then draw the face above it
    auto labelStrip = b.removeFromBottom (16.0f);
    auto face = b.reduced (4.0f, 4.0f);

    juce::ColourGradient paper (juce::Colour (0xffffe8b8), face.getX(), face.getY(),
                                juce::Colour (0xff9a6a30), face.getX(), face.getBottom(), false);
    g.setGradientFill (paper);
    g.fillRoundedRectangle (face, 6.0f);

    // Red zone on the right of the arc
    g.setColour (juce::Colour (0x44cc2010));
    g.fillRoundedRectangle (face.removeFromRight (face.getWidth() * 0.18f).reduced (0.0f, 4.0f), 3.0f);
    face = b.reduced (4.0f, 4.0f);   // restore face

    // Pivot: 15 px above the bottom of the face so it always stays inside
    const auto radius = juce::jmin (face.getWidth(), face.getHeight() * 1.3f) * 0.46f;
    const auto centre = juce::Point<float> (face.getCentreX(), face.getBottom() - 12.0f);

    // Tick marks along the arc
    for (int i = 0; i <= 10; ++i)
    {
        const auto a   = juce::jmap (static_cast<float> (i), 0.0f, 10.0f, -1.05f, 1.05f);
        const auto sinA = std::sin (a);
        const auto cosA = std::cos (a);
        const auto inner = centre + juce::Point<float> (sinA, -cosA) * (radius - (i % 5 == 0 ? 12.0f : 8.0f));
        const auto outer = centre + juce::Point<float> (sinA, -cosA) * radius;
        g.setColour (i >= 8 ? juce::Colour (0xffcc2010) : juce::Colour (0xff3a2410));
        g.drawLine ({ inner, outer }, i % 5 == 0 ? 2.0f : 1.0f);
        if (i % 5 == 0)
        {
            const auto labelPos = centre + juce::Point<float> (sinA, -cosA) * (radius - 20.0f);
            g.setColour (juce::Colour (0xff2a1c10));
            g.setFont (juce::FontOptions (7.0f, juce::Font::bold));
            const int labelVal = (mode == Mode::gainReduction) ? i * 2 : (i == 0 ? -30 : (i == 5 ? 0 : 3));
            g.drawText (juce::String (labelVal), juce::roundToInt (labelPos.x - 9.0f), juce::roundToInt (labelPos.y - 5.0f), 18, 10,
                        juce::Justification::centred);
        }
    }

    // Needle
    const auto maximum = mode == Mode::gainReduction ? 20.0f : 60.0f;
    const auto angle   = juce::jmap (juce::jlimit (0.0f, maximum, displayed), 0.0f, maximum, 1.05f, -1.05f);
    const auto tip     = centre + juce::Point<float> (std::sin (angle), -std::cos (angle)) * (radius - 10.0f);

    g.setColour (juce::Colour (0x99000000));
    g.drawLine ({ centre.translated (1.2f, 1.2f), tip.translated (1.2f, 1.2f) }, 2.0f);
    g.setColour (juce::Colour (0xff6a1208));
    g.drawLine ({ centre, tip }, 1.8f);

    // Pivot cap
    g.setColour (juce::Colour (0xff1a1008));
    g.fillEllipse (centre.x - 5.0f, centre.y - 5.0f, 10.0f, 10.0f);
    g.setColour (juce::Colour (0xffc9a050));
    g.drawEllipse (centre.x - 4.5f, centre.y - 4.5f, 9.0f, 9.0f, 1.0f);

    // Label
    g.setColour (juce::Colour (0xff2a1c10));
    g.setFont (juce::FontOptions (8.0f, juce::Font::bold));
    g.drawText (mode == Mode::gainReduction ? "GR" : (mode == Mode::input ? "IN" : "OUT"),
                labelStrip.toNearestInt(), juce::Justification::centred);
}

void TubeChamberComponent::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (3.0f);
    RackDrawing::paintInsetDisplay (g, b);

    g.setColour (juce::Colour (0xffb88848));
    g.setFont (juce::FontOptions (7.5f, juce::Font::bold));
    auto header = b.removeFromTop (13.0f);
    g.drawText ("TUBE CHAMBER", header, juce::Justification::centred);

    auto window = b.reduced (6.0f, 4.0f);

    // Dark glass background for the window
    juce::ColourGradient glass (juce::Colour (0xff1a1210), window.getX(), window.getY(),
                                juce::Colour (0xff0a0806), window.getRight(), window.getBottom(), true);
    g.setGradientFill (glass);
    g.fillRoundedRectangle (window, 5.0f);
    g.setColour (juce::Colour (0x33c88840));
    g.drawRoundedRectangle (window.reduced (0.5f), 5.0f, 1.0f);

    // Draw three vacuum tubes side by side
    const auto alpha = 0.22f + activity * 0.78f;
    const auto glowAlpha = activity * activity;   // non-linear warm-up glow
    const float tubeW = window.getWidth() / 3.0f - 6.0f;
    const float tubeH = window.getHeight() - 16.0f;
    const float tubeRadius = juce::jmin (tubeW * 0.5f, 8.0f);   // small corner = tall capsule

    for (int i = 0; i < 3; ++i)
    {
        const float cx = window.getX() + (i + 0.5f) * (window.getWidth() / 3.0f);
        auto tube = juce::Rectangle<float> (cx - tubeW * 0.5f, window.getY() + 8.0f, tubeW, tubeH);

        // Outer glow halo
        if (glowAlpha > 0.0f)
        {
            juce::ColourGradient halo (juce::Colour::fromFloatRGBA (1.0f, 0.55f, 0.05f, glowAlpha * 0.55f),
                                       tube.getCentreX(), tube.getCentreY(),
                                       juce::Colour::fromFloatRGBA (0.9f, 0.35f, 0.0f, 0.0f),
                                       tube.getCentreX(), tube.getY() - 6.0f, true);
            g.setGradientFill (halo);
            g.fillEllipse (tube.expanded (10.0f, 8.0f));
        }

        // Tube glass body — tall capsule: corner radius much smaller than width
        juce::ColourGradient tubeGrad (
            juce::Colour::fromFloatRGBA (0.98f, 0.58f + activity * 0.20f, 0.10f, alpha),
            tube.getCentreX(), tube.getBottom(),
            juce::Colour::fromFloatRGBA (0.15f, 0.10f, 0.04f, alpha * 0.7f),
            tube.getCentreX(), tube.getY(), false);
        g.setGradientFill (tubeGrad);
        g.fillRoundedRectangle (tube, tubeRadius);

        // Glass highlight sheen on the left face
        juce::ColourGradient sheen (juce::Colour (0x44ffffff), tube.getX() + 2.0f, tube.getY(),
                                    juce::Colour (0x00ffffff), tube.getCentreX(), tube.getBottom(), false);
        g.setGradientFill (sheen);
        g.fillRoundedRectangle (tube.reduced (1.0f, 1.0f), tubeRadius - 1.0f);

        // Outer rim
        g.setColour (juce::Colour::fromFloatRGBA (0.95f, 0.72f, 0.35f, alpha));
        g.drawRoundedRectangle (tube, tubeRadius, 1.0f);

        // Filament glow at the base
        g.setColour (juce::Colour::fromFloatRGBA (1.0f, 0.90f, 0.50f, alpha * 0.85f));
        g.fillEllipse (tube.getCentreX() - 2.5f, tube.getBottom() - 12.0f, 5.0f, 10.0f);

        // Pin leads at the bottom
        g.setColour (juce::Colour (0xff8a7050));
        for (int p = -1; p <= 1; ++p)
            g.drawLine (tube.getCentreX() + p * 3.5f, tube.getBottom(), tube.getCentreX() + p * 3.5f, tube.getBottom() + 5.0f, 0.9f);
    }
}

void EqSpectrumComponent::setAnalyzerLevels (const std::array<float, analyzerBins>& levels)
{
    analyzer = levels;
    repaint();
}

void EqSpectrumComponent::setBandValues (const BandValues& values)
{
    bands = values;
    repaint();
}

void EqSpectrumComponent::setExpanded (bool shouldExpand)
{
    if (expanded == shouldExpand)
        return;
    expanded = shouldExpand;
    activeNode = -1;
    repaint();
    if (onExpandedChanged != nullptr)
        onExpandedChanged (expanded);
}

float EqSpectrumComponent::frequencyToNorm (float frequency) noexcept
{
    constexpr float minHz = 20.0f;
    constexpr float maxHz = 20000.0f;
    return juce::jlimit (0.0f, 1.0f, std::log (juce::jlimit (minHz, maxHz, frequency) / minHz)
                                      / std::log (maxHz / minHz));
}

float EqSpectrumComponent::gainToNorm (float gainDb) noexcept
{
    return juce::jlimit (0.0f, 1.0f, juce::jmap (gainDb, -12.0f, 12.0f, 1.0f, 0.0f));
}

juce::Rectangle<float> EqSpectrumComponent::getPlotBounds() const
{
    auto b = getLocalBounds().toFloat().reduced (expanded ? 16.0f : 8.0f, expanded ? 34.0f : 16.0f);
    if (expanded)
        b.removeFromBottom (58.0f);
    return b;
}

juce::Point<float> EqSpectrumComponent::nodePositionForBand (int band, juce::Rectangle<float> plot) const
{
    float x = 0.0f, y = plot.getCentreY();
    if (band == 0) { x = frequencyToNorm (bands.hpfHz); y = 0.88f; }
    if (band == 1) { x = frequencyToNorm (bands.lowShelfHz); y = gainToNorm (bands.lowShelfGainDb); }
    if (band == 2) { x = frequencyToNorm (bands.bellHz); y = gainToNorm (bands.bellGainDb); }
    if (band == 3) { x = frequencyToNorm (bands.highShelfHz); y = gainToNorm (bands.highShelfGainDb); }
    if (band == 4) { x = frequencyToNorm (bands.lpfHz); y = 0.88f; }
    return { plot.getX() + plot.getWidth() * x, plot.getY() + plot.getHeight() * y };
}

int EqSpectrumComponent::hitTestNode (juce::Point<float> position) const
{
    const auto plot = getPlotBounds();
    for (int i = 0; i < 5; ++i)
        if (position.getDistanceFrom (nodePositionForBand (i, plot)) < (expanded ? 14.0f : 9.0f))
            return i;
    return -1;
}

void EqSpectrumComponent::mouseDown (const juce::MouseEvent& e)
{
    if (expanded && e.position.x > static_cast<float> (getWidth() - 42) && e.position.y < 30.0f)
    {
        setExpanded (false);
        return;
    }

    activeNode = hitTestNode (e.position);
    if (! expanded && activeNode < 0)
        setExpanded (true);
}

void EqSpectrumComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (activeNode < 0 || onNodeDragged == nullptr)
        return;
    const auto plot = getPlotBounds();
    const auto normX = juce::jlimit (0.0f, 1.0f, (e.position.x - plot.getX()) / plot.getWidth());
    const auto normY = juce::jlimit (0.0f, 1.0f, (e.position.y - plot.getY()) / plot.getHeight());
    onNodeDragged (activeNode, normX, normY);
}

void EqSpectrumComponent::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (3.0f);
    RackDrawing::paintInsetDisplay (g, b);

    g.setColour (juce::Colour (0xffc9a060));
    g.setFont (juce::FontOptions (expanded ? 13.0f : 9.0f, juce::Font::bold));
    g.drawText (expanded ? "EQ EDITOR - POST" : "EQ (Click to Expand)",
                b.removeFromTop (expanded ? 26.0f : 14.0f).toNearestInt(), juce::Justification::centredLeft);
    if (expanded)
    {
        g.setColour (juce::Colour (0xffd8b878));
        g.drawText ("X", getWidth() - 36, 6, 24, 20, juce::Justification::centred);
    }

    const auto plot = getPlotBounds();
    g.setColour (juce::Colour (0xff071012));
    g.fillRoundedRectangle (plot, 3.0f);
    for (int i = 1; i < 10; ++i)
    {
        g.setColour (juce::Colour (0x14ffffff));
        g.drawVerticalLine (juce::roundToInt (plot.getX() + plot.getWidth() * i / 10.0f), plot.getY(), plot.getBottom());
    }
    for (int i = 1; i < 5; ++i)
        g.drawHorizontalLine (juce::roundToInt (plot.getY() + plot.getHeight() * i / 5.0f), plot.getX(), plot.getRight());

    juce::Path spectrum;
    for (int i = 0; i < analyzerBins; ++i)
    {
        const auto x = plot.getX() + plot.getWidth() * static_cast<float> (i) / static_cast<float> (analyzerBins - 1);
        const auto y = plot.getBottom() - plot.getHeight() * juce::jlimit (0.0f, 1.0f, analyzer[static_cast<size_t> (i)]);
        if (i == 0) spectrum.startNewSubPath (x, y); else spectrum.lineTo (x, y);
    }
    g.setColour (juce::Colour (0x55ff9a3a));
    g.strokePath (spectrum, juce::PathStrokeType (expanded ? 1.8f : 1.2f));

    juce::Path curve;
    for (int x = 0; x <= juce::roundToInt (plot.getWidth()); ++x)
    {
        const auto n = static_cast<float> (x) / juce::jmax (1.0f, plot.getWidth());
        const auto lowShape = bands.lowShelfGainDb * (1.0f - n) * 0.035f;
        const auto highShape = bands.highShelfGainDb * n * 0.035f;
        const auto bellCentre = frequencyToNorm (bands.bellHz);
        const auto bellShape = bands.bellGainDb * std::exp (-std::pow ((n - bellCentre) * bands.bellQ * 5.0f, 2.0f)) * 0.045f;
        const auto cutLeft = n < frequencyToNorm (bands.hpfHz) ? (frequencyToNorm (bands.hpfHz) - n) * 0.9f : 0.0f;
        const auto cutRight = n > frequencyToNorm (bands.lpfHz) ? (n - frequencyToNorm (bands.lpfHz)) * 0.9f : 0.0f;
        const auto y = plot.getCentreY() - (lowShape + highShape + bellShape + tone * (n - 0.5f) * 0.45f) * plot.getHeight()
                     + (cutLeft + cutRight) * plot.getHeight();
        if (x == 0) curve.startNewSubPath (plot.getX(), y); else curve.lineTo (plot.getX() + x, y);
    }
    g.setColour (bands.enabled ? juce::Colour (0xffefab45) : juce::Colour (0xff665744));
    g.strokePath (curve, juce::PathStrokeType (expanded ? 2.8f : 2.0f, juce::PathStrokeType::curved));

    const juce::Colour bandColours[] { juce::Colour (0xff4aa3ff), juce::Colour (0xff49d17a), juce::Colour (0xfff0c14d),
                                       juce::Colour (0xffff8a3d), juce::Colour (0xffff5d6e) };
    const char* labels[] { "HPF", "LOW", "BELL", "HIGH", "LPF" };
    for (int i = 0; i < 5; ++i)
    {
        const auto p = nodePositionForBand (i, plot);
        g.setColour (bandColours[i]);
        g.fillEllipse (p.x - (expanded ? 7.0f : 5.0f), p.y - (expanded ? 7.0f : 5.0f), expanded ? 14.0f : 10.0f, expanded ? 14.0f : 10.0f);
        g.setColour (juce::Colour (0xbbffffff));
        g.drawEllipse (p.x - (expanded ? 7.0f : 5.0f), p.y - (expanded ? 7.0f : 5.0f), expanded ? 14.0f : 10.0f, expanded ? 14.0f : 10.0f, 1.0f);
        if (expanded)
        {
            g.setColour (juce::Colour (0xffe6d2a0));
            g.setFont (juce::FontOptions (8.0f, juce::Font::bold));
            g.drawText (labels[i], juce::roundToInt (p.x - 22.0f), juce::roundToInt (p.y + 9.0f), 44, 12, juce::Justification::centred);
        }
    }

    if (expanded)
    {
        auto readouts = getLocalBounds().reduced (18).removeFromBottom (46);
        g.setColour (juce::Colour (0xffd8b878));
        g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        g.drawFittedText ("HPF " + juce::String (bands.hpfHz, 0) + " Hz   LOW " + juce::String (bands.lowShelfGainDb, 1) + " dB   BELL "
                          + juce::String (bands.bellHz, 0) + " Hz / " + juce::String (bands.bellGainDb, 1) + " dB / Q "
                          + juce::String (bands.bellQ, 2) + "   HIGH " + juce::String (bands.highShelfGainDb, 1)
                          + " dB   LPF " + juce::String (bands.lpfHz / 1000.0f, 1) + " kHz",
                          readouts, juce::Justification::centred, 2);
    }
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
