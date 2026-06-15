#pragma once

#include <JuceHeader.h>

class MicCharacterProcessor
{
public:
    enum class Mode { u67, c12, classic251, u87, u47, elaM251, c800g };
    enum class SourceMode { unknown, dynamicGeneral, condenserGeneral, ribbonGeneral, sm57Style,
                            sm7Style, c80Style, brightCondenser, darkCondenser, warmTubeMic, flatMeasurement };

    void prepare (double sampleRate, int channels);
    void reset() noexcept;
    float processSample (int channel, float input, Mode mode) noexcept;
    float processRelativeSample (int channel, float input, SourceMode source, Mode target,
                                 float correctionAmount, float targetAmount, float badFrequencyTamer,
                                 float airProtection, float bodyProtection, bool hardwareSafe) noexcept;

private:
    struct Profile
    {
        float highPassHz;
        float bodyDb;
        float presenceDb;
        float airDb;
        float harmonicAmount;
        float harmonicBias;
        float outputDb;
    };

    struct State
    {
        float dc = 0.0f;
        float low = 0.0f;
        float presenceLow = 0.0f;
        float airLow = 0.0f;
        float problemEnvelope = 0.0f;
    };

    static Profile profileFor (Mode mode) noexcept;
    static Profile sourceProfileFor (SourceMode mode) noexcept;
    float processProfile (State& state, float input, const Profile& profile, float amount,
                          float airProtection, float bodyProtection, bool invert) noexcept;
    static float onePoleCoefficient (float frequency, double sampleRate) noexcept;
    static float dbToDelta (float db) noexcept;

    std::vector<State> states;
    double currentSampleRate = 44100.0;
    Profile currentProfile { 70.0f, 0.0f, 0.0f, 0.0f, 0.02f, 0.0f, 0.0f };
    float profileSmoothing = 0.0f;
};
