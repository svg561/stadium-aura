#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/PremiumKnob.h"
#include "ui/PremiumLookAndFeel.h"
#include "ui/RackMeter.h"

class TubeGlowComponent final : public juce::Component
{
public:
    explicit TubeGlowComponent (juce::String modelName) : model (std::move (modelName)) {}
    void setActivity (float newActivity) noexcept { activity = juce::jlimit (0.0f, 1.0f, newActivity); repaint(); }
    void paint (juce::Graphics&) override;

private:
    juce::String model;
    float activity = 0.0f;
};

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
    void addKnob (PremiumKnob&, const char* parameterId, std::unique_ptr<SliderAttachment>&);

    StadiumAuraAudioProcessor& auraProcessorRef;
    PremiumLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltipWindow { this, 350 };

    PremiumKnob input { "INPUT", 0.0, " dB" }, output { "OUTPUT", 0.0, " dB" };
    PremiumKnob mix { "MIX", 100.0, " %" }, aura { "AURA", 35.0, " %" };
    PremiumKnob tube { "TUBE", 20.0, " %" }, saturation { "SAT", 20.0, " %" };
    PremiumKnob transformer { "IRON", 15.0, " %" }, summing { "SUM", 20.0, " %" };
    PremiumKnob glue { "GLUE", 20.0, " %" }, width { "WIDTH", 100.0, " %" };
    PremiumKnob tone { "TONE", 0.0, " %" };
    PremiumKnob compression { "COMP", 0.0, " %" }, ceiling { "CEILING", -0.3, " dB" };
    PremiumKnob bias { "BIAS", 0.0, " %" }, makeup { "MAKEUP", 0.0, " dB" };

    juce::ToggleButton bypass { "BYPASS" }, mono { "MONO" }, limiter { "LIMITER" }, dim { "DIM" };
    juce::ComboBox compressorMode, trackCount, quality, presets, micCharacter, preampMode, tubeSwap;
    RackMeter inputMeter { "IN", RackMeter::Kind::level };
    RackMeter outputMeter { "OUT", RackMeter::Kind::level };
    RackMeter grMeter { "GR", RackMeter::Kind::reduction };
    RackMeter limiterMeter { "LIM", RackMeter::Kind::reduction };
    TubeGlowComponent preampTube { "12AX7 PREAMP" };
    TubeGlowComponent driverTube { "12AU7 DRIVER" };

    std::array<std::unique_ptr<SliderAttachment>, 15> sliderAttachments;
    std::unique_ptr<ButtonAttachment> bypassAttachment, monoAttachment, limiterAttachment, dimAttachment;
    std::unique_ptr<ComboAttachment> compressorModeAttachment, trackCountAttachment, qualityAttachment;
    std::unique_ptr<ComboAttachment> micCharacterAttachment, preampModeAttachment, tubeSwapAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StadiumAuraAudioProcessorEditor)
};
