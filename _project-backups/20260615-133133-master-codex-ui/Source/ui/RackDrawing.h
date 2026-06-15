#pragma once

#include <JuceHeader.h>

namespace RackDrawing
{
inline void paintBrushedMetal (juce::Graphics& g, juce::Rectangle<float> area, bool vertical = true)
{
    juce::ColourGradient base (juce::Colour (0xff1a1c1f), area.getX(), area.getY(),
                               juce::Colour (0xff0a0b0d), area.getRight(), area.getBottom(), false);
    g.setGradientFill (base);
    g.fillRect (area);
    for (int i = 0; i < juce::roundToInt (vertical ? area.getHeight() : area.getWidth()); i += 2)
    {
        g.setColour (i % 4 == 0 ? juce::Colour (0x0cffffff) : juce::Colour (0x05000000));
        if (vertical) g.drawHorizontalLine (juce::roundToInt (area.getY()) + i, area.getX(), area.getRight());
        else g.drawVerticalLine (juce::roundToInt (area.getX()) + i, area.getY(), area.getBottom());
    }
}

inline void paintRackModule (juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& title,
                             juce::Colour accent = juce::Colour (0xffc58a38))
{
    g.setColour (juce::Colour (0x66000000));
    g.fillRoundedRectangle (bounds.translated (0.0f, 2.0f), 5.0f);
    paintBrushedMetal (g, bounds);
    g.setColour (juce::Colour (0xff3a2f22));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 5.0f, 1.4f);
    g.setColour (juce::Colour (0x22ffffff));
    g.drawLine (bounds.getX() + 6.0f, bounds.getY() + 1.0f, bounds.getRight() - 6.0f, bounds.getY() + 1.0f, 1.0f);
    auto header = bounds.removeFromTop (24.0f).reduced (4.0f, 2.0f);
    g.setColour (juce::Colour (0xff0d0f11));
    g.fillRoundedRectangle (header, 3.0f);
    g.setColour (accent);
    g.drawRoundedRectangle (header.reduced (0.5f), 3.0f, 0.8f);
    g.setColour (juce::Colour (0xffe8c070));
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText (title.toUpperCase(), header, juce::Justification::centred);
}

inline void paintScrews (juce::Graphics& g, juce::Rectangle<float> frame, float inset = 10.0f)
{
    const juce::Point<float> pts[] {
        { frame.getX() + inset, frame.getY() + inset },
        { frame.getRight() - inset, frame.getY() + inset },
        { frame.getX() + inset, frame.getBottom() - inset },
        { frame.getRight() - inset, frame.getBottom() - inset }
    };
    for (auto p : pts)
    {
        g.setColour (juce::Colour (0xff050607));
        g.fillEllipse (p.x - 5.0f, p.y - 5.0f, 10.0f, 10.0f);
        g.setColour (juce::Colour (0xff8a6a3f));
        g.drawEllipse (p.x - 4.0f, p.y - 4.0f, 8.0f, 8.0f, 1.0f);
        g.drawLine (p.x - 2.5f, p.y - 2.5f, p.x + 2.5f, p.y + 2.5f, 1.0f);
    }
}

inline void paintInsetDisplay (juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour (juce::Colour (0xff040506));
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (juce::Colour (0xff2a2118));
    g.drawRoundedRectangle (area.reduced (0.5f), 4.0f, 1.0f);
    g.setColour (juce::Colour (0x18ffffff));
    g.drawRoundedRectangle (area.reduced (1.5f), 3.0f, 0.5f);
}
}
