#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <array>
#include "EmotionLockEngine.h"

enum class AuraCompressorModel
{
    Aura2A     = 0,  // smooth optical vocal leveler
    Aura76     = 1,  // fast FET peak controller
    AuraTube   = 2,  // modern polished lead vocal compressor
    AuraLimiter = 3, // vocal peak catcher / safety ceiling
    AuraDensity = 4, // body, grit, console push
    AuraDrums  = 5   // drum glue / smash / snare / kick
};

enum class AuraTimingMode
{
    Fixed  = 0,
    Manual = 1,
    Hybrid = 2
};

struct CompressorProfilePreset
{
    float attackMs      = 10.0f;
    float releaseMs     = 300.0f;
    float fastReleaseMs = 60.0f;
    float slowReleaseMs = 800.0f;
    float ratio         = 3.0f;
    float thresholdDb   = -24.0f;
    float mix           = 1.0f;
    float drive         = 0.0f;
    float density       = 0.0f;
    float warmth        = 0.0f;
    float outputGainDb  = 0.0f;
    float sidechainHpfHz = 0.0f;
    float inputGainDb   = 0.0f;
};

struct CompressorParams
{
    bool enabled  = true;
    bool bypass   = false;
    AuraCompressorModel model = AuraCompressorModel::Aura2A;
    int profile   = 0;
    AuraTimingMode timingMode = AuraTimingMode::Fixed;

    float inputGainDb   = 0.0f;
    float thresholdDb   = -24.0f;
    float ratio         = 3.0f;
    float attackMs      = 10.0f;
    float releaseMs     = 300.0f;
    float sidechainHpfHz = 0.0f;
    float mix           = 1.0f;
    float drive         = 0.0f;
    float density       = 0.0f;
    float warmth        = 0.0f;
    float outputGainDb  = 0.0f;
    float ceilingDb     = -0.5f;
    float targetGrDb    = 0.0f;

    // Legacy / shared fields kept for host compatibility
    float inputDb       = 0.0f;
    float amount        = 0.5f;
    float makeupDb      = 0.0f;
    float sidechainHPF  = 90.0f;
    float saturation    = 0.0f;
    bool autoGain       = false;
    bool auraLevel      = false;
    bool linkedStereo   = true;

    enum class DetectorMode { Peak, RMS, VocalFocus, Mid, Side } detectorMode = DetectorMode::RMS;
    EmotionLockParams emotionLock;
};

int getAuraCompressorProfileCount (AuraCompressorModel model) noexcept;
juce::String getAuraCompressorProfileName (AuraCompressorModel model, int profileIndex) noexcept;
CompressorProfilePreset getAuraCompressorProfilePreset (AuraCompressorModel model, int profileIndex) noexcept;

class AuraCompressorEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset() noexcept;
    void updateParameters (const CompressorParams& p);
    void processBlock (juce::AudioBuffer<float>& buffer);

    float getGainReductionDb()  const noexcept { return gainReductionDb.load (std::memory_order_relaxed); }
    float getOutputLevelDb()    const noexcept { return outputLevelDb.load (std::memory_order_relaxed); }
    float getTargetGrDb()       const noexcept { return targetGrDbAtomic.load (std::memory_order_relaxed); }

private:
    void applyProfilePreset (const CompressorParams& p) noexcept;
    void initSmoothers (double sr) noexcept;
    void updateSmootherTargets (const CompressorParams& p) noexcept;

    void processAura2A     (juce::AudioBuffer<float>& buffer);
    void processAura76     (juce::AudioBuffer<float>& buffer);
    void processAuraTube   (juce::AudioBuffer<float>& buffer);
    void processAuraLimiter (juce::AudioBuffer<float>& buffer);
    void processAuraDensity (juce::AudioBuffer<float>& buffer);
    void processAuraDrums  (juce::AudioBuffer<float>& buffer);

    static float computeGainReductionDb (float inputLevelDb, float thresholdDb,
                                          float ratio, float kneeDb) noexcept;
    float softSaturate (float x, float drive) const noexcept;
    float densitySaturate (float x, float drive, float density, float warmth) const noexcept;

    void updateHPFCoeffs (float hpfHz) noexcept;
    float processHPF (float x, int ch) noexcept;

    float detectLevel (float scL, float scR, int ch, bool usePeak) noexcept;
    void  smoothEnvelope (float grDb, int ch, float attCoeff, float relCoeff) noexcept;
    float getSmoothedInputGain() noexcept;
    float getSmoothedThreshold() noexcept;
    float getSmoothedRatio() noexcept;
    float getSmoothedAttackMs() noexcept;
    float getSmoothedReleaseMs() noexcept;
    float getSmoothedMix() noexcept;
    float getSmoothedOutputGain() noexcept;
    float getSmoothedDrive() noexcept;
    float getSmoothedDensity() noexcept;
    float getSmoothedWarmth() noexcept;
    float getSmoothedCeiling() noexcept;
    float getSmoothedSidechainHpf() noexcept;
    float getBypassMix() noexcept;

    double sampleRate  = 44100.0;
    int    numChannels = 2;
    CompressorParams params;
    CompressorProfilePreset activePreset {};
    EmotionLockEngine emotionLock;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smInputGain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smThreshold;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smRatio;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smAttack;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smRelease;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smOutputGain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smDrive;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smDensity;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smWarmth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smCeiling;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smSidechainHpf;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smBypass;

    std::array<float, 2> envelope  { 0.0f, 0.0f };
    std::array<float, 2> rmsState  { 0.0f, 0.0f };

    std::array<float, 2> optoFastEnv { 0.0f, 0.0f };
    std::array<float, 2> optoSlowEnv { 0.0f, 0.0f };

    std::array<int, 2> punchCountdown { 0, 0 };

    float muRatio = 2.0f;
    float tubeProgramMemory = 0.0f;

    float adaptiveThreshold   = 0.0f;
    float auraLevelRms        = 0.0f;
    float auraLevelAdaptCoeff = 0.0f;

    float attackCoeff  = 0.0f;
    float releaseCoeff = 0.0f;

    float hpfB0 = 1.0f, hpfB1 = 0.0f, hpfB2 = 0.0f, hpfA1 = 0.0f, hpfA2 = 0.0f;
    struct BiquadState { float z1 = 0.0f, z2 = 0.0f; };
    std::array<BiquadState, 2> hpfState {};
    float lastHpfHz = 0.0f;

    int lastProfile = -1;
    AuraCompressorModel lastModel = AuraCompressorModel::Aura2A;

    std::atomic<float> gainReductionDb { 0.0f };
    std::atomic<float> outputLevelDb   { -60.0f };
    std::atomic<float> targetGrDbAtomic { 0.0f };
};
