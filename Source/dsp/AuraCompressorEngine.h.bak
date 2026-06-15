#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <array>
#include "EmotionLockEngine.h"

enum class AuraCompressorModel
{
    LightningFET = 0,  // fast FET behavior
    VelvetOpto   = 1,  // smooth opto behavior
    CrownMu      = 2,  // vari-mu tube behavior
    PunchCell    = 3,  // VCA punch behavior
    GlueBus      = 4,  // bus compression behavior
    ModernClean  = 5   // transparent digital
};

struct CompressorParams
{
    bool enabled  = true;
    AuraCompressorModel model = AuraCompressorModel::VelvetOpto;

    float inputDb      = 0.0f;
    float thresholdDb  = -24.0f;
    float amount       = 0.5f;   // 0-1, scales effective threshold downward
    float ratio        = 3.0f;
    float attackMs     = 10.0f;
    float releaseMs    = 300.0f;
    float makeupDb     = 0.0f;
    float mix          = 1.0f;   // 0-1 wet/dry
    float sidechainHPF = 90.0f;
    float saturation   = 0.0f;   // 0-1

    bool autoGain      = false;
    bool auraLevel     = false;
    bool linkedStereo  = true;

    enum class DetectorMode { Peak, RMS, VocalFocus, Mid, Side } detectorMode = DetectorMode::RMS;

    EmotionLockParams emotionLock;
};

class AuraCompressorEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset() noexcept;
    void updateParameters (const CompressorParams& p);
    void processBlock (juce::AudioBuffer<float>& buffer);

    float getGainReductionDb()  const noexcept { return gainReductionDb.load (std::memory_order_relaxed); }
    float getOutputLevelDb()    const noexcept { return outputLevelDb.load (std::memory_order_relaxed); }

private:
    void processLightningFET (juce::AudioBuffer<float>& buffer);
    void processVelvetOpto   (juce::AudioBuffer<float>& buffer);
    void processCrownMu      (juce::AudioBuffer<float>& buffer);
    void processPunchCell    (juce::AudioBuffer<float>& buffer);
    void processGlueBus      (juce::AudioBuffer<float>& buffer);
    void processModernClean  (juce::AudioBuffer<float>& buffer);

    static float computeGainReductionDb (float inputLevelDb, float thresholdDb,
                                          float ratio, float kneeDb) noexcept;
    float softSaturate (float x, float drive) const noexcept;

    void updateHPFCoeffs (float hpfHz) noexcept;
    float processHPF (float x, int ch) noexcept;

    // Shared per-sample helpers used by all models
    float detectLevel (float scL, float scR, int ch, bool usePeak) noexcept;
    void  smoothEnvelope (float grDb, int ch, float attCoeff, float relCoeff) noexcept;

    double sampleRate  = 44100.0;
    int    numChannels = 2;
    CompressorParams params;
    EmotionLockEngine emotionLock;

    // Envelope state per channel
    std::array<float, 2> envelope  { 0.0f, 0.0f };
    std::array<float, 2> rmsState  { 0.0f, 0.0f }; // for RMS detection

    // VelvetOpto two-stage release
    std::array<float, 2> optoFastEnv { 0.0f, 0.0f };
    std::array<float, 2> optoSlowEnv { 0.0f, 0.0f };

    // PunchCell transient window (in samples)
    std::array<int, 2> punchCountdown { 0, 0 };

    // CrownMu variable ratio smoother
    float muRatio = 2.0f;

    // Aura Level adaptive threshold
    float adaptiveThreshold   = 0.0f;
    float auraLevelRms        = 0.0f;
    float auraLevelAdaptCoeff = 0.0f;

    // Pre-computed time constants (updated in updateParameters)
    float attackCoeff  = 0.0f;
    float releaseCoeff = 0.0f;

    // Sidechain HPF biquad (manual, no allocation)
    float hpfB0 = 1.0f, hpfB1 = 0.0f, hpfB2 = 0.0f, hpfA1 = 0.0f, hpfA2 = 0.0f;
    struct BiquadState { float z1 = 0.0f, z2 = 0.0f; };
    std::array<BiquadState, 2> hpfState {};
    float lastHpfHz = 0.0f;

    // Metering (written on audio thread, read on UI thread)
    std::atomic<float> gainReductionDb { 0.0f };
    std::atomic<float> outputLevelDb   { -60.0f };
};
