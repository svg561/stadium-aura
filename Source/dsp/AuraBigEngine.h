#pragma once

#include <JuceHeader.h>

struct AuraBigParams
{
    bool globalBypass = false;
    float amount = 0.f;
    float inputDb = 0.f;
    float outputDb = 0.f;
    float tone = 0.35f;
    float tube = 0.35f;
    float transistor = 0.2f;
    float transformer = 0.3f;
    float density = 0.4f;
    float air = 0.3f;
    float width = 0.12f;
    float limiter = 0.5f;
    bool safe = true;
    bool inputBypass = false;
    bool toneBypass = false;
    bool tubeBypass = false;
    bool transistorBypass = false;
    bool transformerBypass = false;
    bool densityBypass = false;
    bool airBypass = false;
    bool widthBypass = false;
    bool limiterBypass = false;
};

enum class SweetSpotState { TooLow, Sweet, Hot, Clipping };

class AuraBigEngine
{
public:
    void prepare (double sampleRate, int blockSize, int channelCount);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const AuraBigParams& params);

    SweetSpotState getSweetSpotState() const noexcept { return sweetSpotState; }
    float getInputRmsDb() const noexcept { return inputRmsDb; }
    float getInputPeakDb() const noexcept { return inputPeakDb; }
    bool isActive() const noexcept { return active; }

private:
    struct BiquadCoefficients
    {
        float b0 = 1.f, b1 = 0.f, b2 = 0.f, a1 = 0.f, a2 = 0.f;
    };

    void measureInput (const juce::AudioBuffer<float>& buffer) noexcept;
    void updateSweetSpotState() noexcept;
    void processInputTrim (juce::AudioBuffer<float>& buffer, const AuraBigParams& params, int numSamples) noexcept;
    void processToneLift (juce::AudioBuffer<float>& buffer, const AuraBigParams& params, float amount) noexcept;
    void processPlaceholderStages (const AuraBigParams& params) noexcept;
    void processAdaptiveLimiter (juce::AudioBuffer<float>& buffer, const AuraBigParams& params, int numSamples) noexcept;
    void processOutputTrim (juce::AudioBuffer<float>& buffer, const AuraBigParams& params, int numSamples) noexcept;
    void blendWithDry (juce::AudioBuffer<float>& buffer, int numSamples) noexcept;

    void updateToneCoefficients (float bodyDb, float airDb, float harshDb) noexcept;
    static float processBiquad (float input, float& z1, float& z2,
                                  const BiquadCoefficients& c) noexcept;
    void setHighPass (BiquadCoefficients& c, float frequency) noexcept;
    void setLowShelf (BiquadCoefficients& c, float frequency, float gainDb) noexcept;
    void setHighShelf (BiquadCoefficients& c, float frequency, float gainDb) noexcept;
    void setPeak (BiquadCoefficients& c, float frequency, float gainDb, float q) noexcept;

    juce::AudioBuffer<float> dryBuffer;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> amountSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inputGainSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGainSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> bodyGainSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> airGainSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> harshGainSmoother;

    BiquadCoefficients bodyShelfCoefficients;
    BiquadCoefficients airShelfCoefficients;
    BiquadCoefficients harshBellCoefficients;
    BiquadCoefficients subHpfCoefficients;
    std::array<std::array<float, 2>, 4> toneZ1 {};
    std::array<std::array<float, 2>, 4> toneZ2 {};

    float limiterGain = 1.f;
    double sampleRate = 44100.0;

    SweetSpotState sweetSpotState = SweetSpotState::TooLow;
    float inputRmsDb = -60.f;
    float inputPeakDb = -60.f;
    bool active = false;
    int numChannels = 2;
};
