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
    void captureAbState (int slot);
    void restoreAbState (int slot);

    StadiumAuraAudioProcessor& processorRef;
    PremiumLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltipWindow { this, 350 };

    SectionPanel matchingPanel { "Matching Engine" }, micCharacterPanel { "Mic Character" };
    SectionPanel preampPanel { "Preamp Architecture" }, auraPanel { "Aura Magic Control" };
    SectionPanel compressorPanel { "Compressor" }, consolePanel { "Console / Summing" };
    SectionPanel eqPanel { "EQ Display" }, utilityPanel { "Output Utility" };

    std::array<RoutingButton, 7> routing {{ RoutingButton { "MIC" }, RoutingButton { "PRE" },
        RoutingButton { "COMP" }, RoutingButton { "HARMONICS" }, RoutingButton { "SUM" },
        RoutingButton { "MASTER" }, RoutingButton { "OUTPUT" } }};

    PremiumFader inputFader { "INPUT", 0.0, " dB", true };
    PremiumFader outputFader { "OUTPUT", 0.0, " dB", true };
    PremiumKnob correction { "CORRECTION", 35.0, " %" }, targetAmount { "TARGET", 50.0, " %" };
    PremiumKnob badFreq { "BAD FREQ", 35.0, " %" }, bodyProtect { "BODY SAFE", 50.0, " %" };
    PremiumKnob airProtect { "AIR SAFE", 50.0, " %" };
    PremiumKnob micAmount { "AMOUNT", 0.0, " dB" }, deHarsh { "DE-HARSH", 35.0, " %" };
    PremiumKnob preampDrive { "DRIVE", 25.0, " %" }, preampTone { "TONE", 0.0, " %" };
    PremiumKnob preampOutput { "OUTPUT", 0.0, " dB" };
    PremiumKnob aura { "AURA", 50.0, " %" };
    juce::Slider tubeDriveSlider;
    PremiumKnob tubeBias { "BIAS", 0.0, " %" }, tubeOutput { "TUBE OUT", 0.0, " dB" };
    PremiumKnob threshold { "THRESHOLD", -18.0, " dB" }, ratio { "RATIO", 4.0, ":1" };
    PremiumKnob attack { "ATTACK", 20.0, " ms" }, release { "RELEASE", 400.0, " ms" };
    PremiumKnob compMakeup { "MAKEUP", 0.0, " dB" }, bleed { "BLEED", 0.0, " %" };
    PremiumKnob scHpf { "SC HPF", 80.0, " Hz" };
    PremiumKnob saturation { "SATURATION", 20.0, " %" }, transformer { "TRANSFORMER", 20.0, " %" };
    PremiumKnob consoleDensity { "DENSITY", 20.0, " %" }, glue { "BUS GLUE", 20.0, " %" };
    PremiumKnob width { "WIDTH", 100.0, " %" }, mix { "MIX", 100.0, " %" };
    PremiumKnob ceiling { "CEILING", -1.0, " dB" };

    juce::ComboBox sourceMic, targetMic, micCharacterMode, preampMode, tubeType;
    juce::ComboBox compressorMode, compTiming, vuMode, consoleMode, trackCount, quality, presets;
    juce::ToggleButton hardwareSafe { "HARDWARE SAFE" }, compressorEnable { "COMP ON" };
    juce::ToggleButton limiter { "LIMITER" }, bypass { "BYPASS" }, mono { "MONO" };
    juce::ToggleButton dim { "DIM -18 dB" };
    juce::TextButton abA { "A" }, abB { "B" }, copyAb { "A>B" };
    DisabledFeatureButton analyzeSource { "ANALYZE SOURCE", "Learned source analysis is not implemented yet." };
    DisabledFeatureButton proximity { "PROXIMITY", "Proximity control is not implemented yet." };
    DisabledFeatureButton hpfButton { "HPF", "Dedicated HPF control is not implemented yet." };
    DisabledFeatureButton phaseInvert { "PHASE", "Phase invert is not implemented yet." };
    DisabledFeatureButton compIn { "IN", "Compressor sidechain input routing is not implemented yet." };
    DisabledFeatureButton compOut { "OUT", "Compressor output routing is not implemented yet." };
    DisabledFeatureButton undoButton { "UNDO", "Host undo handles parameter changes." };
    DisabledFeatureButton redoButton { "REDO", "Host redo handles parameter changes." };
    DisabledFeatureButton favoriteButton { "FAV", "Preset favorites are not implemented yet." };
    juce::Label logoTitle, logoSubtitle, auraPercent, sweetZone, sweetLow, sweetHot, presetCard, saBadge;

    RackMeter inputRms { "INPUT RMS", RackMeter::Kind::level };
    RackMeter outputRms { "OUTPUT RMS", RackMeter::Kind::level };
    RackMeter limiterMeter { "LIMITER GR", RackMeter::Kind::reduction };
    StereoLrMeter inputLr { "INPUT" }, outputLr { "OUTPUT" };
    VuMeterComponent vuMeter;
    TubeChamberComponent tubeChamber;
    CompactEqComponent compactEq;

    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;
    std::array<std::unique_ptr<juce::XmlElement>, 2> abSnapshots;
    int focusedRoute = 0;
    int activeAbSlot = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StadiumAuraAudioProcessorEditor)
};
