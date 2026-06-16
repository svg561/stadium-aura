#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/PremiumFader.h"
#include "ui/PremiumKnob.h"
#include "ui/PremiumLookAndFeel.h"
#include "ui/RackComponents.h"
#include "ui/RackMeter.h"
#include "ui/EQPanel.h"
#include "ui/ExpandedEQPanel.h"

class StadiumAuraAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit StadiumAuraAudioProcessorEditor (StadiumAuraAudioProcessor&);
    ~StadiumAuraAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    void timerCallback() override;
    void attachSlider (juce::Slider&, const char* parameterId);
    void attachButton (juce::Button&, const char* parameterId);
    void attachCombo (juce::ComboBox&, const char* id, const juce::StringArray& items);
    void setFocusedRoute (int index);
    void applyRouteVisuals();
    void toggleSectionBypass (int routeIndex);
    bool isSectionEnabled (int routeIndex) const;
    void setChoiceParameter (const char* id, int index);
    void setFloatParameter (const char* id, float value);
    void setEqNodeFromUi (int band, float normX, float normY);
    void updateEqDisplayState();
    void captureAbState (int slot);
    void restoreAbState (int slot);
    void refreshCompressorProfileBar();
    void updateCompressorControlVisibility();
    void layoutKnobRow (juce::Rectangle<int>& area, std::initializer_list<juce::Slider*> knobs);
    void layoutKnobGrid (juce::Rectangle<int>& area, int columns, std::initializer_list<juce::Slider*> knobs);

    StadiumAuraAudioProcessor& processorRef;
    PremiumLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltipWindow { this, 350 };

    RackModulePanel leftPanel { "INPUT / TONE" };
    RackModulePanel heroPanel { "AURA" };
    RackModulePanel rightPanel { "DYNAMICS / OUTPUT" };
    RackModulePanel eqPanel { "EQ / TONE" };

    std::array<NavRouteButton, 7> routing {{ NavRouteButton { "MIC" }, NavRouteButton { "PRE" },
        NavRouteButton { "COMP" }, NavRouteButton { "HARMONICS" }, NavRouteButton { "SUM" },
        NavRouteButton { "MASTER" }, NavRouteButton { "OUTPUT" } }};

    IconBarButton presetPrev { "<" }, presetNext { ">" };
    IconBarButton abA { "A" }, abB { "B" };
    IconBarButton settingsBtn { "G" }, helpBtn { "?" };
    DisabledFeatureButton favoriteBtn { "*", "Preset favorites are not implemented yet." };
    DisabledFeatureButton saveBtn { "SAVE", "Preset saving is not implemented yet." };
    DisabledFeatureButton undoBtn { "UNDO", "Host undo integration is not implemented yet." };
    DisabledFeatureButton redoBtn { "REDO", "Host redo integration is not implemented yet." };
    DisabledFeatureButton analyzeSource { "ANALYZE", "Learned source analysis is not implemented yet." };

    PremiumKnob inputKnob { "INPUT", 0.0, " dB" };
    PremiumKnob outputKnob { "OUTPUT", 0.0, " dB" };
    PremiumFader inputFader { "INPUT LEVEL", 0.0, " dB" };
    PremiumFader outputFader { "OUTPUT LEVEL", 0.0, " dB" };
    PremiumKnob bodyKnob { "BODY", 50.0, " %" };
    PremiumKnob presenceKnob { "PRES", 0.0, " %" };
    PremiumKnob airKnob { "AIR", 50.0, " %" };
    PremiumKnob tubeDriveKnob { "TUBE", 25.0, " %" };
    PremiumKnob saturation { "SAT", 20.0, " %" };
    PremiumKnob tubeBias { "BIAS", 0.0, " %" };
    PremiumKnob transformer { "XFMR", 20.0, " %" };
    PremiumKnob summing { "SUM", 20.0, " %" };
    PremiumKnob glue { "GLUE", 20.0, " %" };
    PremiumKnob correction { "CORR", 35.0, " %" };
    PremiumKnob targetAmount { "TGT", 50.0, " %" };
    PremiumKnob badFreq { "BAD", 35.0, " %" };
    PremiumKnob preampDrive { "DRV", 25.0, " %" };
    PremiumKnob aura { "AURA", 0.0, " %" };
    PremiumKnob compAmount { "COMP", 0.0, " %" };
    PremiumKnob compMakeup { "MK", 0.0, " dB" };
    PremiumKnob attack { "ATK", 20.0, " ms" };
    PremiumKnob release { "REL", 400.0, " ms" };
    PremiumKnob threshold { "THR", -18.0, " dB" };
    PremiumKnob ratio { "RAT", 4.0, ":1" };
    PremiumKnob bleed { "BLD", 0.0, " %" };
    PremiumKnob mix { "MIX", 100.0, " %" };
    PremiumKnob width { "WID", 100.0, " %" };
    PremiumKnob ceiling { "CEIL", -1.0, " dB" };

    juce::ComboBox sourceMic, targetMic, preampMode, tubeType, vuMode, consoleMode, presets;
    juce::ToggleButton hardwareSafe { "SAFE" };
    juce::ToggleButton compressorEnable { "COMP ON" };
    juce::ToggleButton limiter { "LIMITER" };
    juce::ToggleButton bypass { "BYPASS" };
    juce::ToggleButton mono { "MONO" };
    juce::ToggleButton dim { "DIM" };

    juce::Label logoTitle, logoSubtitle, sweetZone, sweetLow, sweetHot;
    juce::Label presetCard, latencyLabel, oversamplingLabel, monitorLabel;

    VerticalRmsMeter inputRms { "INPUT" };
    VerticalRmsMeter outputRms { "OUTPUT" };
    StereoLrMeter inputLrMeter { "INPUT" };
    StereoLrMeter outputLrMeter { "OUTPUT" };
    HorizontalReductionMeter grMeter { "GAIN REDUCTION" };
    HorizontalReductionMeter limMeter { "LIMITER GR" };
    VuMeterComponent vuMeter;
    TubeChamberComponent tubeChamber;
    EQPanel eqDisplay;
    SegmentedChoiceBar trackButtons;
    SegmentedChoiceBar compModeButtons;
    SegmentedChoiceBar qualityBar;
    SegmentedChoiceBar compModelBar;
    SegmentedChoiceBar compProfileBar;

    juce::ToggleButton compBypassBtn { "BYPASS" };
    juce::ComboBox compTimingMode;
    juce::ComboBox compScHpfMode;

    PremiumKnob compMixKnob     { "MIX",  100.0, " %" };
    PremiumKnob compDriveKnob   { "DRIVE", 0.0,  " %" };
    PremiumKnob compDensityKnob { "DENS",  0.0,  " %" };
    PremiumKnob compWarmthKnob  { "WARM",  0.0,  " %" };
    PremiumKnob compOutputKnob  { "OUT",   0.0,  " dB" };
    HorizontalReductionMeter compGrMeter { "COMP GR" };
    juce::Label compTargetGrLabel;

    // Legacy / auxiliary compressor controls
    PremiumKnob compInputKnob    { "INPUT",  0.0,  " dB" };
    PremiumKnob compSidechainKnob { "HPF",   90.0, " Hz" };

    // Emotion Lock controls
    juce::ToggleButton emotionLockBtn { "EMOTION LOCK" };
    juce::Label        emotionLockStatusLabel;

    // Aura Level controls
    juce::ToggleButton auraLevelBtn { "AURA LEVEL" };
    juce::Label        auraLevelStateLabel;

    // Horizontal vintage VU meters (meter strip)
    AuraHorizontalVUMeter inputVuMeter      { AuraHorizontalVUMeter::MeterMode::Input };
    AuraHorizontalVUMeter grHorizontalMeter { AuraHorizontalVUMeter::MeterMode::GainReduction };
    AuraHorizontalVUMeter outputVuMeter     { AuraHorizontalVUMeter::MeterMode::Output };

    // Aura big label (above aura knob in hero panel)
    juce::Label auraBigLabel;

    std::unique_ptr<ExpandedEQPanel> expandedEQPanel;

    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;
    std::array<std::unique_ptr<juce::XmlElement>, 2> abSnapshots;
    int focusedRoute = 0;
    int activeAbSlot = 0;
    // Re-assert standalone unmute for the first ~2 s so the saved device state
    // cannot silently re-mute the input after the editor is constructed.
    int startupUnmuteCountdown { 180 };  // 3 s at 60 Hz
    bool startupAutoInputTriggered { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StadiumAuraAudioProcessorEditor)
};
