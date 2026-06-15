#pragma once
#include <JuceHeader.h>
#include "EQPanel.h"

// Forward declaration to avoid pulling in the full processor header here
class StadiumAuraAudioProcessor;

//==============================================================================
// A single card representing one EQ band in the bottom cards row.
//==============================================================================
class BandCardComponent : public juce::Component
{
public:
    BandCardComponent() = default;

    void setState (int idx, const EQBandState& s, bool isSelected);
    void paint    (juce::Graphics& g) override;
    void mouseUp  (const juce::MouseEvent& e) override;

    std::function<void (int)> onSelected;

private:
    int        bandIdx   = -1;
    EQBandState bandState {};
    bool       selected  = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BandCardComponent)
};

//==============================================================================
// Full-screen overlay panel that shows the expanded AURA EQ interface.
// Open it by calling setVisible(true); close via the close button (onClose callback).
//==============================================================================
class ExpandedEQPanel : public juce::Component
{
public:
    explicit ExpandedEQPanel (StadiumAuraAudioProcessor& proc);
    ~ExpandedEQPanel() override;

    void paint   (juce::Graphics& g) override;
    void resized () override;

    // Call from editor's timerCallback when this panel is visible
    void refreshFromParameters ();

    // Called when the close button is pressed — editor should hide this panel
    std::function<void()> onClose;

private:
    // ── Layout geometry ──────────────────────────────────────────────────────
    static constexpr int kHeaderH  = 44;
    static constexpr int kCardsH   = 84;
    static constexpr int kCardW    = 90;
    static constexpr int kOuterPad = 8;

    juce::Rectangle<int> getHeaderArea () const;
    juce::Rectangle<int> getGraphArea  () const;
    juce::Rectangle<int> getCardsArea  () const;

    // ── Header controls ──────────────────────────────────────────────────────
    juce::Label      titleLabel;
    juce::TextButton bypassButton    { "PWR" };
    juce::TextButton closeButton     { "X" };
    juce::TextButton auraTraceButton { "TRACE" };
    juce::ComboBox   stereoModeBox;
    juce::ComboBox   analyzerModeBox;
    juce::ComboBox   scaleBox;

    // ── EQ graph ─────────────────────────────────────────────────────────────
    EQPanel eqGraph;

    // ── Band cards row (horizontally scrollable) ──────────────────────────────
    juce::Viewport  cardsViewport;
    juce::Component cardsContainer;
    std::array<BandCardComponent, 24> bandCards;
    juce::TextButton addBandButton { "+ ADD" };

    // ── State ─────────────────────────────────────────────────────────────────
    StadiumAuraAudioProcessor& processorRef;
    std::array<EQBandState, 24> cachedBandStates {};
    int  selectedBandIdx = -1;
    bool auraTraceOn     = false;
    bool bypassed        = false;

    // ── APVTS attachment for bypass button ────────────────────────────────────
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<ButtonAttach> bypassAttachment;

    // ── Helpers ───────────────────────────────────────────────────────────────
    void buildCardLayout ();
    void selectBand      (int idx);
    void updateBandCards ();

    static juce::Colour bandTypeColour (EQBandType t) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExpandedEQPanel)
};
