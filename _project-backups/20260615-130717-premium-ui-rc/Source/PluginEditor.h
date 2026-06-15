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
    void attachCombo (juce::ComboBox&, const char* parameterId, const juce::StringArray& items);
    void setFocusedRoute (int index);

    StadiumAuraAudioProcessor& processorRef;
    PremiumLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltipWindow { this, 350 };

    SectionPanel sourcePanel { "Source Mic Matching" }, preampPanel { "Mic Character / Preamp" };
    SectionPanel auraPanel { "Aura / Tube Chamber" }, dynamicsPanel { "Compressor / Real VU" };
    SectionPanel masterPanel { "Console / Master" }, utilityPanel { "Metering / Utility" };

    std::array<RoutingButton, 7> routing {{ RoutingButton { "MIC" }, RoutingButton { "PRE" },
        RoutingButton { "COMP" }, RoutingButton { "HARMONICS" }, RoutingButton { "SUM" },
        RoutingButton { "MASTER" }, RoutingButton { "OUTPUT" } }};

    PremiumFader input { "INPUT", 0.0, " dB" }, output { "OUTPUT", 0.0, " dB" };
    PremiumKnob correction { "CORRECTION", 35.0, " %" }, targetAmount { "TARGET", 50.0, " %" };
    PremiumKnob badFreq { "BAD FREQ", 35.0, " %" }, bodyProtect { "BODY SAFE", 50.0, " %" };
    PremiumKnob airProtect { "AIR SAFE", 50.0, " %" }, preampDrive { "PRE DRIVE", 25.0, " %" };
    PremiumKnob aura { "AURA", 50.0, " %" }, tubeDrive { "TUBE DRIVE", 25.0, " %" };
    PremiumKnob tubeBias { "TUBE BIAS", 0.0, " %" }, tubeOutput { "TUBE OUT", 0.0, " dB" };
    PremiumKnob threshold { "THRESHOLD", -18.0, " dB" }, ratio { "RATIO", 4.0, ":1" };
    PremiumKnob attack { "ATTACK", 20.0, " ms" }, release { "RELEASE", 400.0, " ms" };
    PremiumKnob compMakeup { "MAKEUP", 0.0, " dB" }, bleed { "BLEED", 0.0, " %" };
    PremiumKnob scHpf { "SC HPF", 80.0, " Hz" }, saturation { "SATURATION", 20.0, " %" };
    PremiumKnob transformer { "IRON", 20.0, " %" }, consoleDensity { "DENSITY", 20.0, " %" };
    PremiumKnob glue { "BUS GLUE", 20.0, " %" }, tone { "TONE", 0.0, " %" };
    PremiumKnob width { "WIDTH", 100.0, " %" }, mix { "MIX", 100.0, " %" };
    PremiumKnob ceiling { "CEILING", -1.0, " dB" };

    juce::ComboBox sourceMic, targetMic, preampMode, tubeType, compressorMode, compTiming;
    juce::ComboBox vuMode, consoleMode, trackCount, quality, presets;
    juce::ToggleButton hardwareSafe { "HARDWARE SAFE" }, compressorEnable { "COMP ON" };
    juce::ToggleButton limiter { "LIMITER" }, bypass { "BYPASS" }, mono { "MONO" }, dim { "DIM" };
    juce::Label sweetZone;

    RackMeter inputMeter { "INPUT", RackMeter::Kind::level };
    RackMeter outputMeter { "OUTPUT", RackMeter::Kind::level };
    RackMeter limiterMeter { "LIMITER GR", RackMeter::Kind::reduction };
    VuMeterComponent vuMeter;
    TubeChamberComponent tubeChamber;
    CompactEqComponent compactEq;

    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttachment>> comboAttachments;
    int focusedRoute = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StadiumAuraAudioProcessorEditor)
};
