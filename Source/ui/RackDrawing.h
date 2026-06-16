#pragma once

#include <JuceHeader.h>

namespace RackDrawing
{
namespace Palette
{
    inline juce::Colour panelTop()      { return juce::Colour (0xff15191D); }
    inline juce::Colour panelBottom()   { return juce::Colour (0xff0A0D10); }
    inline juce::Colour accentGold()    { return juce::Colour (0xffFFC24A); }
    inline juce::Colour accentGlow()    { return juce::Colour (0xffFF8A22); }
    inline juce::Colour textPrimary()   { return juce::Colour (0xffF2E6CC); }
    inline juce::Colour textSecondary() { return juce::Colour (0xffAFA79A); }
    inline juce::Colour textDim()       { return juce::Colour (0xff6F6A62); }
    inline juce::Colour safeGreen()     { return juce::Colour (0xff6fcf4a); }
    inline juce::Colour clipRed()       { return juce::Colour (0xffe04030); }
    inline juce::Colour insetBg()       { return juce::Colour (0xff040506); }
    inline juce::Colour insetBorder()   { return juce::Colour (0xff2a2118); }
}

inline void paintBrushedMetal (juce::Graphics& g, juce::Rectangle<float> area, bool vertical = true)
{
    juce::ColourGradient base (Palette::panelTop(), area.getX(), area.getY(),
                               Palette::panelBottom(), area.getRight(), area.getBottom(), false);
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
                             juce::Colour accent = Palette::accentGold())
{
    g.setColour (juce::Colour (0x66000000));
    g.fillRoundedRectangle (bounds.translated (0.0f, 2.0f), 5.0f);
    paintBrushedMetal (g, bounds);
    g.setColour (juce::Colour (0xff3a2f22));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 5.0f, 1.4f);
    g.setColour (juce::Colour (0x22ffffff));
    g.drawLine (bounds.getX() + 6.0f, bounds.getY() + 1.0f, bounds.getRight() - 6.0f, bounds.getY() + 1.0f, 1.0f);
    auto header = bounds.removeFromTop (24.0f).reduced (4.0f, 2.0f);
    g.setColour (Palette::panelBottom());
    g.fillRoundedRectangle (header, 3.0f);
    g.setColour (accent);
    g.drawRoundedRectangle (header.reduced (0.5f), 3.0f, 0.8f);
    g.setColour (Palette::textPrimary());
    g.setFont (juce::FontOptions (13.5f, juce::Font::bold).withKerningFactor (0.08f));
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
    g.setColour (Palette::insetBg());
    g.fillRoundedRectangle (area, 4.0f);
    g.setColour (Palette::insetBorder());
    g.drawRoundedRectangle (area.reduced (0.5f), 4.0f, 1.0f);
    g.setColour (juce::Colour (0x18ffffff));
    g.drawRoundedRectangle (area.reduced (1.5f), 3.0f, 0.5f);
}

inline void paintPremiumButton (juce::Graphics& g, juce::Rectangle<float> bounds,
                                const juce::String& text, bool highlighted, bool down, bool active, bool enabled)
{
    if (! enabled)
    {
        g.setOpacity (0.35f);
        highlighted = down = active = false;
    }

    auto b = bounds.reduced (1.0f);
    if (down)
        b = b.translated (0.0f, 1.5f);

    if (active)
    {
        g.setColour (Palette::accentGlow().withAlpha (0.28f));
        g.fillRoundedRectangle (b.expanded (3.0f), 5.0f);
    }
    else if (highlighted && enabled)
    {
        g.setColour (Palette::accentGold().withAlpha (0.18f));
        g.fillRoundedRectangle (b.expanded (2.0f), 4.0f);
    }

    juce::ColourGradient fill (active ? juce::Colour (0xff8a5a18) : juce::Colour (0xff15181c),
                               b.getX(), b.getY(),
                               active ? juce::Colour (0xff3a2808) : juce::Colour (0xff080a0c),
                               b.getRight(), b.getBottom(), false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (b, 4.0f);

    const auto borderCol = ! enabled ? Palette::textDim()
                         : active || highlighted || down ? Palette::accentGold()
                         : Palette::textDim().brighter (0.25f);
    g.setColour (borderCol);
    g.drawRoundedRectangle (b, 4.0f, active ? 1.4f : 1.0f);

    g.setColour (active ? juce::Colour (0xff0d0a06) : Palette::textPrimary());
    g.setFont (juce::FontOptions (text.length() > 10 ? 10.0f : 11.5f, juce::Font::bold));
    g.drawFittedText (text, b.toNearestInt(), juce::Justification::centred, 1);

    if (! enabled)
        g.setOpacity (1.0f);
}
}
