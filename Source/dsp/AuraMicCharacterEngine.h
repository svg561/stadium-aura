#pragma once

#include <JuceHeader.h>

struct AuraMicProfile
{
    const char* id;
    const char* publicName;
    float inputTrimDb;
    float lowCutHz;
    float proximityDb;
    float bodyDb;
    float mudDb;
    float presenceDb;
    float airDb;
    float deHarshAmount;
    float sibilanceAmount;
    float colorAmount;
    float outputTrimDb;
};

static constexpr int kNumAuraMicProfiles = 10;

struct MicCharacterParams
{
    bool enabled = true;
    bool bypass = false;
    int profileIndex = 0;
    float inputTrimDb = 0.0f;
    float proximity = 0.0f;
    float bodyDb = 0.0f;
    float mudDb = 0.0f;
    float presenceDb = 0.0f;
    float airDb = 0.0f;
    float deHarsh = 0.0f;
    float sibilanceGuard = 0.0f;
    float colorAmount = 0.0f;
    float outputTrimDb = 0.0f;
    float lowCutHz = 80.0f;
    bool simpleMode = true;
};

class AuraMicCharacterEngine
{
public:
    void prepare (double sampleRate, int maximumBlockSize, int channels);
    void reset() noexcept;
    void process (juce::AudioBuffer<float>& buffer, const MicCharacterParams& params) noexcept;
    void applyProfileDefaults (MicCharacterParams& params, int profileIndex) noexcept;

    static const AuraMicProfile auraMicProfiles[kNumAuraMicProfiles];

private:
    struct BiquadCoeffs
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    };

    struct ChannelState
    {
        float hpfZ1 = 0.0f, hpfZ2 = 0.0f;
        float bodyZ1 = 0.0f, bodyZ2 = 0.0f;
        float mudZ1 = 0.0f, mudZ2 = 0.0f;
        float presZ1 = 0.0f, presZ2 = 0.0f;
        float airZ1 = 0.0f, airZ2 = 0.0f;
        float harshLow = 0.0f, harshBand = 0.0f, harshEnv = 0.0f;
        float sibLow = 0.0f, sibBand = 0.0f, sibEnv = 0.0f;
    };

    static float processBiquad (float input, float& z1, float& z2, const BiquadCoeffs& c) noexcept;
    void updateCoefficients (float lowCutHz, float bodyDb, float mudDb, float presenceDb, float airDb) noexcept;
    static BiquadCoeffs makeHighPass (double sampleRate, float frequency) noexcept;
    static BiquadCoeffs makePeak (double sampleRate, float frequency, float gainDb, float q) noexcept;
    static BiquadCoeffs makeHighShelf (double sampleRate, float frequency, float gainDb) noexcept;

    std::vector<ChannelState> states;
    double currentSampleRate = 44100.0;
    BiquadCoeffs hpfCoeffs, bodyCoeffs, mudCoeffs, presCoeffs, airCoeffs;
    float smoothBodyDb = 0.0f, smoothMudDb = 0.0f, smoothPresDb = 0.0f, smoothAirDb = 0.0f;
    float smoothLowCutHz = 80.0f;
    float coeffSmoothing = 0.0f;
    juce::SmoothedValue<float> inputTrimGain, outputTrimGain, colorMix, deHarshAmt, sibilanceAmt;
};
