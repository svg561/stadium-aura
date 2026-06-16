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
    void measureInput (const juce::AudioBuffer<float>& buffer) noexcept;
    void updateSweetSpotState() noexcept;
    void applyInputGain (juce::AudioBuffer<float>& buffer, int numSamples) noexcept;
    void applyOutputGain (juce::AudioBuffer<float>& buffer, int numSamples) noexcept;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> amountSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inputGainSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGainSmoother;

    SweetSpotState sweetSpotState = SweetSpotState::TooLow;
    float inputRmsDb = -60.f;
    float inputPeakDb = -60.f;
    bool active = false;
    int numChannels = 2;
};
