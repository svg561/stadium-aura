#pragma once

#include <JuceHeader.h>
#include <atomic>
#include "dsp/AuraProcessor.h"
#include "dsp/EQBand.h"

class StadiumAuraAudioProcessor final : public juce::AudioProcessor
{
public:
    static constexpr int analyzerBinCount = 48;
    using AnalyzerSnapshot = std::array<float, analyzerBinCount>;

    StadiumAuraAudioProcessor();
    ~StadiumAuraAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return static_cast<int> (factoryPresets.size()); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destinationData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Spectrum analyzer — written from audio thread, read from UI thread
    SpectrumAnalyzer spectrumAnalyzer;

    std::atomic<float> inputMeter { 0.0f };
    std::atomic<float> inputLeftMeter { 0.0f };
    std::atomic<float> inputRightMeter { 0.0f };
    std::atomic<float> outputMeter { 0.0f };
    std::atomic<float> outputLeftMeter { 0.0f };
    std::atomic<float> outputRightMeter { 0.0f };
    std::atomic<float> gainReductionMeter { 0.0f };
    std::atomic<float> limiterReductionMeter { 0.0f };
    std::atomic<float> tubeActivityMeter { 0.0f };

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    AnalyzerSnapshot getAnalyzerSnapshot() const noexcept;

private:
    struct FactoryPreset
    {
        juce::String name;
        std::array<float, 25> values;
    };

    AuraParameters readParameters() const noexcept;
    static float peakForBuffer (const juce::AudioBuffer<float>& buffer) noexcept;
    static float peakForChannel (const juce::AudioBuffer<float>& buffer, int channel) noexcept;
    void updateAnalyzer (const juce::AudioBuffer<float>& buffer) noexcept;

    AuraProcessor auraProcessor;
    std::array<std::atomic<float>, analyzerBinCount> analyzerBins {};
    std::array<float, analyzerBinCount> analyzerLowpassState {};
    std::array<float, analyzerBinCount> analyzerPreviousLowpass {};
    std::array<float, analyzerBinCount> analyzerSmoothed {};
    int currentProgram = 0;
    const std::array<FactoryPreset, 13> factoryPresets {{
        { "Polished Default", { 0, 0, 100, 35, 20, 20, 0, 15, 20, 20, 0, 1, 3, 0, 0, 100, 0, 1, -0.3f, 1, 12, 0, 0, 0, 1 } },
        { "Vocal Chain", { 1, -1.2f, 100, 52, 30, 28, 12, 24, 12, 28, 12, 1, 5, 1, 1, 104, 0, 1, -0.3f, 2, 34, 1.5f, 0, 0, 2 } },
        { "Drum Bus Glue", { 0, -1.8f, 100, 48, 24, 30, -5, 32, 34, 55, -5, 3, 3, 2, 2, 110, 0, 1, -0.3f, 0, 48, 1, 0, 0, 2 } },
        { "Mix Bus Warmth", { 0, -1.4f, 100, 44, 20, 24, 8, 35, 38, 42, -8, 3, 4, 0, 0, 106, 0, 1, -0.3f, 1, 28, 0.5f, 0, 0, 1 } },
        { "Master Ready", { -0.5f, -1.3f, 100, 32, 12, 14, 0, 16, 24, 25, 5, 2, 3, 1, 2, 102, 0, 1, -0.3f, 2, 18, 0, 0, 0, 2 } },
        { "Clean Tracking", { 0, 0, 100, 16, 5, 6, 0, 5, 4, 8, 4, 0, 3, 1, 2, 100, 0, 1, -0.3f, 1, 8, 0, 0, 0, 1 } },
        { "Heavy Tube", { -2, -3.2f, 90, 82, 72, 65, 28, 62, 42, 48, -12, 4, 4, 0, 1, 112, 0, 1, -0.3f, 2, 38, 0, 0, 0, 2 } },
        { "Rap Vocal Air", { 0, -2, 100, 58, 22, 24, 5, 12, 10, 24, 18, 1, 6, 1, 2, 108, 0, 1, -0.3f, 0, 30, 1, 0, 0, 2 } },
        { "Warm Lead Vocal", { 0, -1.5f, 100, 62, 40, 34, 18, 40, 18, 35, -10, 1, 4, 0, 0, 103, 0, 1, -0.3f, 2, 38, 1, 0, 0, 2 } },
        { "Smooth R&B Vocal", { 0, -1.8f, 100, 60, 32, 30, 15, 28, 14, 38, 8, 1, 5, 1, 1, 106, 0, 1, -0.3f, 1, 42, 1.5f, 0, 0, 2 } },
        { "Aggressive Rap Vocal", { -1, -3, 92, 72, 48, 50, -10, 48, 55, 68, -6, 4, 3, 2, 1, 118, 0, 1, -1.0f, 1, 68, 2, 0, 0, 2 } },
        { "Console Tracking", { 0, -1, 100, 42, 22, 20, 4, 28, 34, 30, 2, 2, 3, 0, 0, 100, 0, 1, -1.0f, 0, 22, 0, 0, 0, 1 } },
        { "Aura Sweet Spot", { 0, -1.5f, 100, 50, 28, 24, 6, 28, 30, 34, 4, 2, 3, 0, 0, 106, 0, 1, -1.0f, 0, 30, 0.5f, 0, 0, 2 } }
    }};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StadiumAuraAudioProcessor)
};
