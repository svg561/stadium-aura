#include "RackMeter.h"

RackMeter::RackMeter (juce::String name, Kind kind) : title (std::move (name)), meterKind (kind)
{
    startTimerHz (60);
}

void RackMeter::timerCallback()
{
    const auto next = target.load (std::memory_order_relaxed);
    const auto attack = 0.45f;
    const auto release = 0.045f;
    displayed += (next - displayed) * (next > displayed ? attack : release);
    if (displayed >= held) { held = displayed; holdFrames = 45; }
    else if (--holdFrames <= 0) held += (displayed - held) * 0.08f;
    repaint();
}

void RackMeter::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff090c0f));
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (juce::Colour (0xff4b4232));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);
    auto label = bounds.removeFromBottom (18.0f);
    auto meter = bounds.reduced (7.0f, 5.0f);
    const auto normalized = meterKind == Kind::level
        ? juce::jlimit (0.0f, 1.0f, juce::jmap (juce::Decibels::gainToDecibels (displayed, -60.0f), -60.0f, 0.0f, 0.0f, 1.0f))
        : juce::jlimit (0.0f, 1.0f, displayed / 18.0f);
    const auto horizontal = meterKind == Kind::reduction;
    const auto fill = horizontal ? meter.removeFromLeft (meter.getWidth() * normalized)
                                 : meter.removeFromBottom (meter.getHeight() * normalized);
    juce::ColourGradient gradient (juce::Colour (0xff88b93f), fill.getBottomLeft(),
                                   juce::Colour (0xffffad32), horizontal ? fill.getTopRight() : fill.getTopLeft(), false);
    gradient.addColour (0.82, juce::Colour (0xffd16949));
    g.setGradientFill (gradient);
    g.fillRoundedRectangle (fill, 2.0f);
    const auto heldNormalized = meterKind == Kind::level
        ? juce::jlimit (0.0f, 1.0f, juce::jmap (juce::Decibels::gainToDecibels (held, -60.0f), -60.0f, 0.0f, 0.0f, 1.0f))
        : juce::jlimit (0.0f, 1.0f, held / 18.0f);
    g.setColour (juce::Colour (0xfff3dfaa));
    if (horizontal)
    {
        const auto heldX = bounds.getX() + 7.0f + heldNormalized * (bounds.getWidth() - 14.0f);
        g.fillRect (heldX, bounds.getY() + 5.0f, 1.0f, bounds.getHeight() - 28.0f);
    }
    else
    {
        const auto heldY = bounds.getBottom() - 18.0f - 5.0f - heldNormalized * (bounds.getHeight() - 28.0f);
        g.fillRect (bounds.getX() + 7.0f, heldY, bounds.getWidth() - 14.0f, 1.0f);
    }
    g.setColour (juce::Colour (0xffd8ccb0));
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText (title, label, juce::Justification::centred);
}
