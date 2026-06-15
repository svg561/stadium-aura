#pragma once

#include <JuceHeader.h>
#include "CompressorSection.h"
#include "MicCharacterProcessor.h"
#include "PreampArchitecture.h"
#include "SafetyLimiter.h"
#include "SummingGlue.h"
#include "TransformerColor.h"
#include "TubeSaturation.h"
#include "WidthProcessor.h"

struct AuraParameters
{
    float inputGainDb = 0.0f;
    float outputGainDb = 0.0f;
    float mix = 1.0f;
    float aura = 0.35f;
    float tubeDrive = 0.2f;
    float saturation = 0.2f;
    float harmonicBias = 0.0f;
    float transformer = 0.15f;
    float summing = 0.2f;
    float glue = 0.2f;
    float tone = 0.0f;
    int trackCount = 16;
    MicCharacterProcessor::Mode micCharacter = MicCharacterProcessor::Mode::u87;
    PreampArchitecture::Mode preampMode = PreampArchitecture::Mode::vintage73;
    int tubeSwap = 0;
    float width = 1.0f;
    bool monoCheck = false;
    bool limiterEnabled = true;
    float limiterCeilingDb = -0.3f;
    CompressorSection::Mode compressorMode = CompressorSection::Mode::smoothOpto;
    float compressorAmount = 0.0f;
    float makeupGainDb = 0.0f;
    bool bypassed = false;
    bool dimmed = false;
    int quality = 1;
};

class AuraProcessor
{
public:
    void prepare (double sampleRate, int maximumBlockSize, int channels);
    void reset() noexcept;
    void process (juce::AudioBuffer<float>& buffer, const AuraParameters& parameters) noexcept;

    float getGainReductionDb() const noexcept { return compressor.getGainReductionDb() + busCompressor.getGainReductionDb(); }
    float getLimiterReductionDb() const noexcept { return limiter.getReductionDb(); }
    float getTubeActivity() const noexcept { return tubeActivity; }
    int getLatencySamples() const noexcept { return limiter.getLatencySamples(); }

private:
    struct MacroValues
    {
        float driveDb;
        float saturation;
        float transformer;
        float glue;
        float compression;
        float width;
        float autoGainDb;
    };

    static MacroValues calculateMacro (float aura) noexcept;
    void initialiseSmoothers (double sampleRate);

    juce::AudioBuffer<float> dryBuffer;
    juce::AudioBuffer<float> bypassDelayBuffer;
    TransformerColor transformer;
    TransformerColor transformerHigh;
    WidthProcessor widthProcessor;
    CompressorSection compressor;
    CompressorSection busCompressor;
    SafetyLimiter limiter;
    MicCharacterProcessor micCharacter;
    PreampArchitecture preamp;

    juce::SmoothedValue<float> inputGain, outputGain, mix, aura, tubeDrive, saturation;
    juce::SmoothedValue<float> harmonicBias, transformerAmount, summing, glue, tone, width;
    juce::SmoothedValue<float> compressorAmount, makeupGain, ceiling, bypassFade, dimGain;
    double currentSampleRate = 44100.0;
    int bypassDelayPosition = 0;
    std::array<float, 2> previousColourInput { 0.0f, 0.0f };
    float tubeActivity = 0.0f;
    std::array<float, 2> toneLow { 0.0f, 0.0f };
    float toneCoefficient = 0.0f;
};
