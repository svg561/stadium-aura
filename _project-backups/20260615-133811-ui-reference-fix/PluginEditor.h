#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/PremiumFader.h"
#include "ui/PremiumKnob.h"
#include "ui/PremiumLookAndFeel.h"
#include "ui/RackComponents.h"
#include "ui/RackMeter.h"

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
    void captureAbState (int slot);
    void restoreAbState (int slot);
    void layoutKnobRow (juce::Rectangle<int>& area, std::initializer_list<juce::Slider*> knobs);

    StadiumAuraAudioProcessor& processorRef;
    PremiumLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltipWindow { this, 350 };

    RackModulePanel matchingPanel { "SOURCE MATCH" }, micPanel { "MIC CHARACTER" };
    RackModulePanel preampPanel { "PREAMP ARCHITECTURE" }, heroPanel { "AURA MAGIC" };
    RackModulePanel compPanel { "DYNAMICS" }, satPanel { "SATURATION" };
    RackModulePanel xfmrPanel { "TRANSFORMER" }, consolePanel { "CONSOLE / SUMMING" };
    RackModulePanel gluePanel { "BUS GLUE" }, eqPanel { "EQ / TONE" };
    RackModulePanel widthPanel { "STEREO WIDTH" }, limiterPanel { "CEILING LIMITER" };

    std::array<NavRouteButton, 7> routing {{ NavRouteButton { "MIC" }, NavRouteButton { "PRE" },
        NavRouteButton { "COMP" }, NavRouteButton { "HARMONICS" }, NavRouteButton { "SUM" },
        NavRouteButton { "MASTER" }, NavRouteButton { "OUTPUT" } }};

    IconBarButton presetPrev { "<" }, presetNext { ">" }, saveBtn { "S" };
    IconBarButton abA { "A" }, abB { "B" }, undoTop { "U" }, redoTop { "R" };
    IconBarButton settingsBtn { "G" }, helpBtn { "?" };
    DisabledFeatureButton favoriteTop { "*", "Preset favorites are not implemented yet." };
    DisabledFeatureButton favoriteBottom { "*", "Preset favorites are not implemented yet." };
    DisabledFeatureButton undoBottom { "UNDO", "Host undo handles parameter changes." };
    DisabledFeatureButton redoBottom { "REDO", "Host redo handles parameter changes." };
    DisabledFeatureButton analyzeSource { "ANALYZE", "Learned source analysis is not implemented yet." };
    DisabledFeatureButton deHarsh { "DEH", "De-harsh uses Bad Freq in Source Match section." };
    DisabledFeatureButton proximity { "PROX", "Proximity control is not implemented yet." };
    DisabledFeatureButton hpfButton { "HPF", "Dedicated HPF control is not implemented yet." };
    DisabledFeatureButton phaseInvert { "PH", "Phase invert is not implemented yet." };
    DisabledFeatureButton compIn { "IN", "Compressor sidechain input routing is not implemented yet." };
    DisabledFeatureButton compOut { "OUT", "Compressor output routing is not implemented yet." };

    PremiumFader inputFader { "INPUT", 0.0, " dB", true };
    PremiumFader outputFader { "OUTPUT", 0.0, " dB", true };
    PremiumKnob correction { "CORR", 35.0, " %" }, targetAmount { "TGT", 50.0, " %" };
    PremiumKnob badFreq { "BAD", 35.0, " %" }, bodyProtect { "BODY", 50.0, " %" };
    PremiumKnob airProtect { "AIR", 50.0, " %" };
    PremiumKnob preampDrive { "DRV", 25.0, " %" }, preampTone { "TONE", 0.0, " %" };
    PremiumKnob preampOutput { "OUT", 0.0, " dB" };
    PremiumKnob aura { "AURA", 50.0, " %" };
    juce::Slider tubeDriveSlider;
    PremiumKnob tubeBias { "BIAS", 0.0, " %" };
    PremiumKnob threshold { "THR", -18.0, " dB" }, ratio { "RAT", 4.0, ":1" };
    PremiumKnob attack { "ATK", 20.0, " ms" }, release { "REL", 400.0, " ms" };
    PremiumKnob compMakeup { "MK", 0.0, " dB" }, bleed { "BLD", 0.0, " %" };
    PremiumKnob scHpf { "HPF", 80.0, " Hz" };
    PremiumKnob saturation { "SAT", 20.0, " %" }, transformer { "XFMR", 20.0, " %" };
    PremiumKnob consoleDensity { "DEN", 20.0, " %" }, glue { "GLUE", 20.0, " %" };
    PremiumKnob width { "WID", 100.0, " %" }, mix { "MIX", 100.0, " %" };
    PremiumKnob ceiling { "CEIL", -1.0, " dB" };

    juce::ComboBox sourceMic, targetMic, micCharacterMode, preampMode, tubeType;
    juce::ComboBox compressorMode, compTiming, vuMode, consoleMode, trackCount, quality, presets;
    juce::ToggleButton hardwareSafe { "SAFE" }, compressorEnable { "COMP ON" };
    juce::ToggleButton limiter { "LIMITER" }, bypass { "BYPASS" }, mono { "MONO" };
    juce::ToggleButton dim { "DIM -18 dB" };
    juce::Label logoTitle, logoSubtitle, auraPercent, sweetZone, sweetLow, sweetHot, presetCard, saBadge, grLabel;

    VerticalRmsMeter inputRms { "INPUT" }, outputRms { "OUTPUT" };
    RackMeter limiterMeter { "LIM GR", RackMeter::Kind::reduction };
    StereoLrMeter inputLr { "IN" }, outputLr { "OUT" };
    VuMeterComponent vuMeter;
    TubeChamberComponent tubeChamber;
    EqSpectrumComponent eqDisplay;
    HeroAuraRing auraRing;

    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;
    std::array<std::unique_ptr<juce::XmlElement>, 2> abSnapshots;
    int focusedRoute = 0;
    int activeAbSlot = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StadiumAuraAudioProcessorEditor)
};
