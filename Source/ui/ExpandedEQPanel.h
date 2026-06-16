#pragma once
#include <JuceHeader.h>
#include "EQPanel.h"
#include "PremiumKnob.h"
#include "RackComponents.h"

class StadiumAuraAudioProcessor;

class BandCardComponent : public juce::Component
{
public:
    BandCardComponent() = default;

    void setState (int idx, const EQBandState& s, bool isSelected);
    void paint    (juce::Graphics& g) override;
    void mouseUp  (const juce::MouseEvent& e) override;

    std::function<void (int)> onSelected;

private:
    int         bandIdx   = -1;
    EQBandState bandState {};
    bool        selected  = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BandCardComponent)
};

class ExpandedEQPanel : public juce::Component
{
public:
    explicit ExpandedEQPanel (StadiumAuraAudioProcessor& proc);
    ~ExpandedEQPanel() override;

    void paint   (juce::Graphics& g) override;
    void resized () override;
    bool keyPressed (const juce::KeyPress& key) override;

    void refreshFromParameters ();

    std::function<void()> onClose;

private:
    static constexpr int kHeaderH    = 44;
    static constexpr int kCardsH     = 84;
    static constexpr int kCardW      = 90;
    static constexpr int kOuterPad   = 8;
    static constexpr int kLeftStripW = 118;
    static constexpr int kRightStripW = 78;

    juce::Rectangle<int> getHeaderArea  () const;
    juce::Rectangle<int> getContentArea () const;
    juce::Rectangle<int> getLeftStripArea  () const;
    juce::Rectangle<int> getGraphArea   () const;
    juce::Rectangle<int> getRightStripArea () const;
    juce::Rectangle<int> getCardsArea   () const;

    juce::Label      titleLabel;
    juce::TextButton eqOnButton      { "EQ ON" };
    juce::TextButton closeButton     { "\u00d7" };
    juce::TextButton auraTraceButton { "TRACE" };
    juce::ComboBox   stereoModeBox;
    juce::ComboBox   analyzerModeBox;
    juce::ComboBox   scaleBox;

    juce::Label      selectedBandLabel;
    PremiumKnob      bandFreqKnob { "FREQ", 1000.0, " Hz" };
    PremiumKnob      bandGainKnob { "GAIN", 0.0, " dB" };
    PremiumKnob      bandQKnob    { "Q", 1.0, "" };
    juce::ComboBox   bandTypeBox;
    juce::ToggleButton bandEnableBtn { "ON" };

    VerticalRmsMeter inputMeter  { "INPUT" };
    VerticalRmsMeter outputMeter { "OUTPUT" };

    EQPanel eqGraph;

    juce::Viewport  cardsViewport;
    juce::Component cardsContainer;
    std::array<BandCardComponent, 24> bandCards;
    juce::TextButton addBandButton { "+ ADD" };

    StadiumAuraAudioProcessor& processorRef;
    std::array<EQBandState, 24> cachedBandStates {};
    int  selectedBandIdx = 0;
    bool auraTraceOn     = false;
    bool bypassed        = false;

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ButtonAttach> eqEnableAttachment;
    std::vector<std::unique_ptr<SliderAttach>> bandSliderAttachments;
    std::unique_ptr<ButtonAttach> bandEnableAttachment;
    std::unique_ptr<ComboAttach>  bandTypeAttachment;

    void rebuildBandControlAttachments();
    void selectBand (int idx);
    void updateBandCards();

    static juce::Colour bandTypeColour (EQBandType t) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExpandedEQPanel)
};
