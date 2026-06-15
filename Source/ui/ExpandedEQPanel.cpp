#include "ExpandedEQPanel.h"
#include "../PluginProcessor.h"
#include <cmath>

//==============================================================================
// BandCardComponent
//==============================================================================

void BandCardComponent::setState (int idx, const EQBandState& s, bool isSelected)
{
    bandIdx   = idx;
    bandState = s;
    selected  = isSelected;
    repaint();
}

static const char* bandTypeName (EQBandType t) noexcept
{
    switch (t)
    {
        case EQBandType::Bell:      return "Bell";
        case EQBandType::LowCut:    return "LC";
        case EQBandType::HighCut:   return "HC";
        case EQBandType::LowShelf:  return "LS";
        case EQBandType::HighShelf: return "HS";
        case EQBandType::Notch:     return "Notch";
        case EQBandType::Tilt:      return "Tilt";
        default:                    return "Bell";
    }
}

void BandCardComponent::paint (juce::Graphics& g)
{
    if (bandIdx < 0) return;

    auto b = getLocalBounds().toFloat().reduced (2.0f);

    // Background
    const auto bgCol = selected ? juce::Colour (0xff2a2010) : juce::Colour (0xff0f0f18);
    g.setColour (bgCol);
    g.fillRoundedRectangle (b, 5.0f);

    // Border
    const auto borderCol = selected ? juce::Colour (0xffffd451) : juce::Colour (0x33ffffff);
    g.setColour (borderCol);
    g.drawRoundedRectangle (b, 5.0f, selected ? 1.5f : 0.8f);

    // Dimmed if disabled
    const float alpha = bandState.enabled ? 1.0f : 0.4f;
    g.setOpacity (alpha);

    // Band number (top)
    g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
    g.setColour (selected ? juce::Colour (0xffffd451) : juce::Colour (0xffb0a080));
    g.drawText ("BAND " + juce::String (bandIdx + 1),
                b.withHeight (14.0f), juce::Justification::centred);

    // Type
    g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    g.setColour (selected ? juce::Colour (0xffffff80) : juce::Colour (0xffe0d0b0));
    g.drawText (bandTypeName (bandState.type),
                b.withY (b.getY() + 14.0f).withHeight (14.0f), juce::Justification::centred);

    // Frequency
    const float freq = bandState.frequencyHz;
    juce::String freqStr = freq >= 1000.0f
        ? juce::String (freq / 1000.0f, 1) + "k"
        : juce::String (juce::roundToInt (freq)) + "";
    g.setFont (juce::FontOptions (9.5f));
    g.setColour (juce::Colour (0xffc8d8ff));
    g.drawText (freqStr, b.withY (b.getY() + 28.0f).withHeight (16.0f), juce::Justification::centred);

    // Gain / slope
    const bool hasGain = (bandState.type == EQBandType::Bell
                       || bandState.type == EQBandType::LowShelf
                       || bandState.type == EQBandType::HighShelf
                       || bandState.type == EQBandType::Tilt);
    if (hasGain)
    {
        const float gain = bandState.gainDb;
        juce::String gainStr = (gain >= 0.0f ? "+" : "") + juce::String (gain, 1) + " dB";
        g.setFont (juce::FontOptions (8.5f));
        g.setColour (gain > 0.0f ? juce::Colour (0xff70ff90) : juce::Colour (0xffff7070));
        g.drawText (gainStr, b.withY (b.getY() + 44.0f).withHeight (14.0f), juce::Justification::centred);
    }
    else
    {
        g.setFont (juce::FontOptions (8.5f));
        g.setColour (juce::Colour (0xff90a0c0));
        g.drawText (juce::String (juce::roundToInt (bandState.slopeDbPerOct)) + " dB/oct",
                    b.withY (b.getY() + 44.0f).withHeight (14.0f), juce::Justification::centred);
    }

    g.setOpacity (1.0f);
}

void BandCardComponent::mouseUp (const juce::MouseEvent&)
{
    if (onSelected && bandIdx >= 0)
        onSelected (bandIdx);
}

//==============================================================================
// ExpandedEQPanel
//==============================================================================

ExpandedEQPanel::ExpandedEQPanel (StadiumAuraAudioProcessor& proc)
    : processorRef (proc)
{
    // ── Title ────────────────────────────────────────────────────────────────
    titleLabel.setText ("AURA EQ", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xffffd451));
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    // ── Bypass button ────────────────────────────────────────────────────────
    bypassButton.setClickingTogglesState (true);
    bypassButton.setColour (juce::TextButton::buttonOnColourId,  juce::Colour (0xff3a6030));
    bypassButton.setColour (juce::TextButton::buttonColourId,    juce::Colour (0xff1a1a24));
    bypassButton.setColour (juce::TextButton::textColourOnId,    juce::Colour (0xff70ff80));
    bypassButton.setColour (juce::TextButton::textColourOffId,   juce::Colour (0xffb0a090));
    bypassButton.setTooltip ("EQ Power / Bypass");
    addAndMakeVisible (bypassButton);

    // Attach to EQ_GLOBAL_BYPASS parameter if it exists
    if (proc.apvts.getParameter ("EQ_GLOBAL_BYPASS") != nullptr)
    {
        bypassAttachment = std::make_unique<ButtonAttach> (proc.apvts, "EQ_GLOBAL_BYPASS", bypassButton);
        bypassButton.setToggleState (
            proc.apvts.getRawParameterValue ("EQ_GLOBAL_BYPASS")->load() > 0.5f,
            juce::dontSendNotification);
    }

    // ── Stereo mode dropdown ──────────────────────────────────────────────────
    stereoModeBox.addItemList ({ "Stereo", "Mid", "Side", "Left", "Right" }, 1);
    stereoModeBox.setSelectedItemIndex (0, juce::dontSendNotification);
    stereoModeBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1a1a28));
    stereoModeBox.setColour (juce::ComboBox::textColourId,       juce::Colour (0xffb0a080));
    addAndMakeVisible (stereoModeBox);

    // ── Analyzer mode dropdown ────────────────────────────────────────────────
    analyzerModeBox.addItemList ({ "Pre EQ", "Post EQ", "Off" }, 1);
    analyzerModeBox.setSelectedItemIndex (0, juce::dontSendNotification);
    analyzerModeBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1a1a28));
    analyzerModeBox.setColour (juce::ComboBox::textColourId,       juce::Colour (0xffb0a080));
    addAndMakeVisible (analyzerModeBox);

    // ── Scale dropdown ────────────────────────────────────────────────────────
    scaleBox.addItemList ({ juce::CharPointer_UTF8 ("\xc2\xb1" "3 dB"),
                             juce::CharPointer_UTF8 ("\xc2\xb1" "6 dB"),
                             juce::CharPointer_UTF8 ("\xc2\xb1" "12 dB"),
                             juce::CharPointer_UTF8 ("\xc2\xb1" "30 dB") }, 1);
    scaleBox.setSelectedItemIndex (2, juce::dontSendNotification); // ±12 dB default
    scaleBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1a1a28));
    scaleBox.setColour (juce::ComboBox::textColourId,       juce::Colour (0xffb0a080));
    scaleBox.onChange = [this]
    {
        eqGraph.setDbRangeIndex (scaleBox.getSelectedItemIndex());
    };
    addAndMakeVisible (scaleBox);

    // ── Aura Trace toggle ─────────────────────────────────────────────────────
    auraTraceButton.setClickingTogglesState (true);
    auraTraceButton.setColour (juce::TextButton::buttonOnColourId,  juce::Colour (0xff2a2050));
    auraTraceButton.setColour (juce::TextButton::buttonColourId,    juce::Colour (0xff1a1a24));
    auraTraceButton.setColour (juce::TextButton::textColourOnId,    juce::Colour (0xff80c8ff));
    auraTraceButton.setColour (juce::TextButton::textColourOffId,   juce::Colour (0xff705050));
    auraTraceButton.setTooltip ("Show vocal frequency zones (Mud/Box/Presence/Bite/Air)");
    auraTraceButton.onClick = [this]
    {
        auraTraceOn = auraTraceButton.getToggleState();
        eqGraph.setAuraTraceOn (auraTraceOn);
    };
    addAndMakeVisible (auraTraceButton);

    // ── Close button ──────────────────────────────────────────────────────────
    closeButton.setColour (juce::TextButton::buttonColourId,  juce::Colour (0xff1a1a24));
    closeButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffa0a0a0));
    closeButton.setTooltip ("Close expanded EQ");
    closeButton.onClick = [this]
    {
        if (onClose) onClose();
    };
    addAndMakeVisible (closeButton);

    // ── EQ graph ─────────────────────────────────────────────────────────────
    eqGraph.bindToParameters (proc.apvts);
    addAndMakeVisible (eqGraph);

    // ── Band cards ────────────────────────────────────────────────────────────
    cardsViewport.setScrollBarsShown (false, true);
    cardsViewport.setScrollOnDragMode (juce::Viewport::ScrollOnDragMode::nonHover);
    cardsViewport.setViewedComponent (&cardsContainer, false);
    addAndMakeVisible (cardsViewport);

    for (size_t i = 0; i < 24; ++i)
    {
        bandCards[i].onSelected = [this] (int idx) { selectBand (idx); };
        cardsContainer.addAndMakeVisible (bandCards[i]);
    }

    addBandButton.setColour (juce::TextButton::buttonColourId,  juce::Colour (0xff1a1a24));
    addBandButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff7090c0));
    addBandButton.setTooltip ("Add a new Bell band at 1 kHz");
    addBandButton.onClick = [this]
    {
        // Find first disabled band and enable it
        for (int i = 0; i < 24; ++i)
        {
            const auto pfx = "EQ_BAND_" + juce::String (i + 1).paddedLeft ('0', 2) + "_";
            if (auto* p = processorRef.apvts.getRawParameterValue (pfx + "ENABLED"))
            {
                if (p->load() < 0.5f)
                {
                    if (auto* ep = processorRef.apvts.getParameter (pfx + "ENABLED"))
                        ep->setValueNotifyingHost (1.0f);
                    if (auto* fp = processorRef.apvts.getParameter (pfx + "FREQ"))
                        fp->setValueNotifyingHost (fp->convertTo0to1 (1000.0f));
                    if (auto* gp = processorRef.apvts.getParameter (pfx + "GAIN"))
                        gp->setValueNotifyingHost (gp->convertTo0to1 (0.0f));
                    if (auto* tp = processorRef.apvts.getParameter (pfx + "TYPE"))
                        tp->setValueNotifyingHost (0.0f);
                    selectBand (i);
                    break;
                }
            }
        }
    };
    cardsContainer.addAndMakeVisible (addBandButton);

    setOpaque (false);
}

ExpandedEQPanel::~ExpandedEQPanel() = default;

//==============================================================================
juce::Rectangle<int> ExpandedEQPanel::getHeaderArea() const
{
    return getLocalBounds().reduced (kOuterPad).removeFromTop (kHeaderH);
}

juce::Rectangle<int> ExpandedEQPanel::getGraphArea() const
{
    auto b = getLocalBounds().reduced (kOuterPad);
    b.removeFromTop (kHeaderH);
    b.removeFromBottom (kCardsH);
    return b;
}

juce::Rectangle<int> ExpandedEQPanel::getCardsArea() const
{
    return getLocalBounds().reduced (kOuterPad).removeFromBottom (kCardsH);
}

//==============================================================================
void ExpandedEQPanel::paint (juce::Graphics& g)
{
    // Semi-transparent dark background
    auto fullBounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xe8080c14));
    g.fillRoundedRectangle (fullBounds.reduced (static_cast<float>(kOuterPad) * 0.5f), 10.0f);

    // Gold border
    g.setColour (juce::Colour (0x80ffd451));
    g.drawRoundedRectangle (fullBounds.reduced (static_cast<float>(kOuterPad) * 0.5f), 10.0f, 1.5f);

    // Header background
    auto header = getHeaderArea().toFloat();
    juce::ColourGradient hdrGrad (juce::Colour (0xff121520), header.getTopLeft(),
                                   juce::Colour (0xff0d0f18), header.getBottomLeft(), false);
    g.setGradientFill (hdrGrad);
    g.fillRect (header.expanded (0.0f, 0.0f));
    g.setColour (juce::Colour (0x40ffd451));
    g.drawLine (header.getX(), header.getBottom(), header.getRight(), header.getBottom(), 1.0f);

    // Header title section highlight
    g.setColour (juce::Colour (0x15ffd451));
    g.fillRect (header.removeFromLeft (120.0f));
}

void ExpandedEQPanel::resized()
{
    // ── EQ graph ─────────────────────────────────────────────────────────────
    eqGraph.setBounds (getGraphArea());

    // ── Header layout ─────────────────────────────────────────────────────────
    auto hl = getHeaderArea().reduced (8, 4);

    titleLabel.setBounds   (hl.removeFromLeft (90));
    hl.removeFromLeft (4);
    bypassButton.setBounds (hl.removeFromLeft (38).reduced (2));
    hl.removeFromLeft (8);

    closeButton.setBounds     (hl.removeFromRight (30).reduced (2));
    hl.removeFromRight (6);
    auraTraceButton.setBounds (hl.removeFromRight (58).reduced (2));
    hl.removeFromRight (6);
    scaleBox.setBounds        (hl.removeFromRight (84).reduced (2, 2));
    hl.removeFromRight (6);
    analyzerModeBox.setBounds (hl.removeFromRight (84).reduced (2, 2));
    hl.removeFromRight (6);
    stereoModeBox.setBounds   (hl.removeFromRight (76).reduced (2, 2));

    // ── Band cards ────────────────────────────────────────────────────────────
    auto ca = getCardsArea();
    cardsViewport.setBounds (ca.reduced (4, 4));

    const int containerW = kCardW * 25; // 24 cards + add button
    cardsContainer.setBounds (0, 0, containerW, cardsViewport.getHeight());
    for (int i = 0; i < 24; ++i)
        bandCards[static_cast<size_t> (i)].setBounds (i * kCardW, 2, kCardW - 3, cardsContainer.getHeight() - 4);
    addBandButton.setBounds (24 * kCardW, 2, kCardW - 3, cardsContainer.getHeight() - 4);
}

//==============================================================================
void ExpandedEQPanel::refreshFromParameters()
{
    const double sr = processorRef.getSampleRate() > 0 ? processorRef.getSampleRate() : 44100.0;

    // Feed spectrum from the processor's analyzer into the EQ graph
    processorRef.spectrumAnalyzer.processFFT();
    eqGraph.setExternalAnalyzerMagnitudes (processorRef.spectrumAnalyzer.getMagnitudes());

    // Update EQ graph band states from APVTS
    eqGraph.updateFromParameters (processorRef.apvts, sr);

    // Cache band states for the band cards
    static const float slopeTable[] = { 6.f,12.f,18.f,24.f,36.f,48.f,72.f,96.f };
    for (int b = 0; b < 24; ++b)
    {
        const auto pfx = "EQ_BAND_" + juce::String (b + 1).paddedLeft ('0', 2) + "_";
        auto get = [&] (const juce::String& id) -> float
        {
            if (auto* p = processorRef.apvts.getRawParameterValue (id)) return p->load();
            return 0.0f;
        };
        auto& s = cachedBandStates[static_cast<size_t> (b)];
        s.enabled       = get (pfx + "ENABLED") > 0.5f;
        s.type          = static_cast<EQBandType> (juce::jlimit (0, 6, (int) get (pfx + "TYPE")));
        s.frequencyHz   = get (pfx + "FREQ");
        s.gainDb        = get (pfx + "GAIN");
        s.q             = get (pfx + "Q");
        const int si    = juce::jlimit (0, 7, (int) get (pfx + "SLOPE"));
        s.slopeDbPerOct = slopeTable[si];
        s.channelMode   = static_cast<EQChannelMode> (juce::jlimit (0, 4, (int) get (pfx + "CHANNEL_MODE")));
    }

    // Check bypass state for visual dimming
    if (auto* p = processorRef.apvts.getRawParameterValue ("EQ_GLOBAL_BYPASS"))
    {
        bypassed = p->load() > 0.5f;
        eqGraph.setAlpha (bypassed ? 0.55f : 1.0f);
    }

    updateBandCards();
}

void ExpandedEQPanel::updateBandCards()
{
    for (int i = 0; i < 24; ++i)
    {
        bandCards[static_cast<size_t> (i)].setState (i, cachedBandStates[static_cast<size_t> (i)], i == selectedBandIdx);
        bandCards[static_cast<size_t> (i)].setVisible (cachedBandStates[static_cast<size_t> (i)].enabled || i < 2);
    }
}

void ExpandedEQPanel::selectBand (int idx)
{
    selectedBandIdx = idx;
    eqGraph.selectBandExternally (idx);

    // Scroll band card into view
    if (juce::isPositiveAndBelow (idx, 24))
    {
        const int xInContainer = idx * kCardW;
        cardsViewport.setViewPosition (
            juce::jmax (0, xInContainer - cardsViewport.getWidth() / 2), 0);
    }

    updateBandCards();
}

juce::Colour ExpandedEQPanel::bandTypeColour (EQBandType t) noexcept
{
    switch (t)
    {
        case EQBandType::LowCut:
        case EQBandType::HighCut:    return juce::Colour (0xff39e6c3);
        case EQBandType::LowShelf:
        case EQBandType::HighShelf:  return juce::Colour (0xffff9f43);
        case EQBandType::Notch:      return juce::Colour (0xffff4fd8);
        case EQBandType::Bell:
        case EQBandType::Tilt:
        default:                     return juce::Colour (0xff9b5cff);
    }
}
