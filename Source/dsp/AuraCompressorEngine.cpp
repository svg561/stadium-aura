#include "AuraCompressorEngine.h"

namespace
{
static inline float makeCoeff (float timeMs, double sr) noexcept
{
    return 1.0f - std::exp (-1.0f / (0.001f * juce::jmax (0.02f, timeMs) * static_cast<float> (sr) + 1e-9f));
}

static const CompressorProfilePreset kAura2AProfiles[] {
    { 40.0f, 400.0f, 80.0f, 2200.0f, 3.0f, -22.0f, 1.0f, 0.0f, 0.15f, 0.20f, 0.0f, 0.0f, 0.0f },
    { 25.0f, 250.0f, 60.0f, 1200.0f, 3.5f, -20.0f, 1.0f, 0.05f, 0.30f, 0.35f, 0.0f, 0.0f, 0.0f },
    { 55.0f, 500.0f, 90.0f, 3500.0f, 3.2f, -18.0f, 1.0f, 0.0f, 0.20f, 0.55f, 2.0f, 0.0f, 0.0f }
};

static const CompressorProfilePreset kAura76Profiles[] {
    { 0.5f, 80.0f, 50.0f, 400.0f, 4.0f, -18.0f, 1.0f, 0.10f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f },
    { 1.5f, 150.0f, 70.0f, 600.0f, 8.0f, -16.0f, 1.0f, 0.15f, 0.0f, 0.0f, 0.0f, 3.0f, 0.0f },
    { 0.3f, 300.0f, 60.0f, 900.0f, 12.0f, -14.0f, 1.0f, 0.25f, 0.0f, 0.0f, 0.0f, 4.0f, 0.0f },
    { 2.0f, 200.0f, 80.0f, 700.0f, 8.0f, -20.0f, 0.50f, 0.12f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f }
};

static const CompressorProfilePreset kAuraTubeProfiles[] {
    { 8.0f, 180.0f, 60.0f, 900.0f, 3.0f, -20.0f, 1.0f, 0.08f, 0.10f, 0.15f, 0.0f, 80.0f, 0.0f },
    { 5.0f, 120.0f, 50.0f, 650.0f, 4.0f, -18.0f, 1.0f, 0.12f, 0.15f, 0.10f, 0.0f, 150.0f, 1.0f },
    { 3.0f, 90.0f, 45.0f, 500.0f, 5.0f, -16.0f, 1.0f, 0.18f, 0.20f, 0.05f, 0.0f, 150.0f, 2.0f },
    { 12.0f, 240.0f, 70.0f, 1100.0f, 3.5f, -22.0f, 1.0f, 0.10f, 0.25f, 0.30f, 0.0f, 80.0f, 0.0f }
};

static const CompressorProfilePreset kAuraLimiterProfiles[] {
    { 0.2f, 60.0f, 40.0f, 120.0f, 20.0f, -6.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f },
    { 0.1f, 80.0f, 50.0f, 140.0f, 20.0f, -4.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -0.5f, 0.0f },
    { 0.5f, 120.0f, 60.0f, 180.0f, 16.0f, -8.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.5f, 0.0f },
    { 0.15f, 100.0f, 55.0f, 160.0f, 24.0f, -3.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -0.3f, 0.0f }
};

static const CompressorProfilePreset kAuraDensityProfiles[] {
    { 12.0f, 220.0f, 70.0f, 800.0f, 4.0f, -22.0f, 1.0f, 0.35f, 0.45f, 0.20f, 0.0f, 0.0f, 0.0f },
    { 8.0f, 180.0f, 60.0f, 650.0f, 5.0f, -20.0f, 1.0f, 0.55f, 0.35f, 0.10f, 0.0f, 0.0f, 0.0f },
    { 10.0f, 260.0f, 80.0f, 900.0f, 3.5f, -24.0f, 1.0f, 0.25f, 0.55f, 0.35f, 0.0f, 0.0f, 0.0f },
    { 6.0f, 150.0f, 55.0f, 550.0f, 6.0f, -18.0f, 1.0f, 0.65f, 0.50f, 0.15f, 2.0f, 0.0f, 3.0f },
    { 5.0f, 140.0f, 50.0f, 500.0f, 4.5f, -20.0f, 0.45f, 0.40f, 0.40f, 0.20f, 0.0f, 0.0f, 0.0f }
};

static const CompressorProfilePreset kAuraDrumsProfiles[] {
    { 30.0f, 220.0f, 80.0f, 700.0f, 3.0f, -18.0f, 1.0f, 0.15f, 0.10f, 0.0f, 0.0f, 80.0f, 2.0f },
    { 1.5f, 120.0f, 60.0f, 450.0f, 8.0f, -14.0f, 1.0f, 0.45f, 0.25f, 0.0f, 0.0f, 120.0f, 4.0f },
    { 20.0f, 350.0f, 100.0f, 900.0f, 4.0f, -20.0f, 1.0f, 0.20f, 0.15f, 0.0f, 0.0f, 60.0f, 1.0f },
    { 0.8f, 90.0f, 50.0f, 350.0f, 6.0f, -12.0f, 1.0f, 0.35f, 0.20f, 0.0f, 0.0f, 150.0f, 3.0f },
    { 4.0f, 160.0f, 70.0f, 500.0f, 5.0f, -16.0f, 1.0f, 0.30f, 0.30f, 0.0f, 0.0f, 100.0f, 2.0f },
    { 2.0f, 140.0f, 65.0f, 420.0f, 7.0f, -15.0f, 0.50f, 0.40f, 0.20f, 0.0f, 0.0f, 110.0f, 3.0f }
};

static const char* kAura2ANames[]      { "Aura 2A Smooth", "Aura 2A Thick", "Aura 2A Warm Level" };
static const char* kAura76Names[]      { "Aura 76 Fast", "Aura 76 Forward", "Aura 76 Aggressive", "Aura 76 Parallel" };
static const char* kAuraTubeNames[]    { "Aura Tube Smooth", "Aura Tube Forward", "Aura Tube Modern", "Aura Tube Thick" };
static const char* kAuraLimiterNames[] { "Aura Vocal Limiter", "Aura Peak Catcher", "Aura Safety Ceiling", "Aura Maximum Headroom" };
static const char* kAuraDensityNames[] { "Aura Vocal Density", "Aura Vocal Grit", "Aura Thick Air", "Aura Console Push", "Aura Parallel Vocal" };
static const char* kAuraDrumsNames[]   { "Aura Drum Glue", "Aura Drum Smash", "Aura Room Pump", "Aura Snare Crack", "Aura Kick Thick", "Aura Parallel Drums" };
} // namespace

int getAuraCompressorProfileCount (AuraCompressorModel model) noexcept
{
    switch (model)
    {
        case AuraCompressorModel::Aura2A:      return 3;
        case AuraCompressorModel::Aura76:      return 4;
        case AuraCompressorModel::AuraTube:    return 4;
        case AuraCompressorModel::AuraLimiter: return 4;
        case AuraCompressorModel::AuraDensity: return 5;
        case AuraCompressorModel::AuraDrums:   return 6;
        default: return 1;
    }
}

juce::String getAuraCompressorProfileName (AuraCompressorModel model, int profileIndex) noexcept
{
    switch (model)
    {
        case AuraCompressorModel::Aura2A:
            return kAura2ANames[juce::jlimit (0, 2, profileIndex)];
        case AuraCompressorModel::Aura76:
            return kAura76Names[juce::jlimit (0, 3, profileIndex)];
        case AuraCompressorModel::AuraTube:
            return kAuraTubeNames[juce::jlimit (0, 3, profileIndex)];
        case AuraCompressorModel::AuraLimiter:
            return kAuraLimiterNames[juce::jlimit (0, 3, profileIndex)];
        case AuraCompressorModel::AuraDensity:
            return kAuraDensityNames[juce::jlimit (0, 4, profileIndex)];
        case AuraCompressorModel::AuraDrums:
            return kAuraDrumsNames[juce::jlimit (0, 5, profileIndex)];
        default: return "Profile";
    }
}

CompressorProfilePreset getAuraCompressorProfilePreset (AuraCompressorModel model, int profileIndex) noexcept
{
    switch (model)
    {
        case AuraCompressorModel::Aura2A:
            return kAura2AProfiles[juce::jlimit (0, 2, profileIndex)];
        case AuraCompressorModel::Aura76:
            return kAura76Profiles[juce::jlimit (0, 3, profileIndex)];
        case AuraCompressorModel::AuraTube:
            return kAuraTubeProfiles[juce::jlimit (0, 3, profileIndex)];
        case AuraCompressorModel::AuraLimiter:
            return kAuraLimiterProfiles[juce::jlimit (0, 3, profileIndex)];
        case AuraCompressorModel::AuraDensity:
            return kAuraDensityProfiles[juce::jlimit (0, 4, profileIndex)];
        case AuraCompressorModel::AuraDrums:
            return kAuraDrumsProfiles[juce::jlimit (0, 5, profileIndex)];
        default: return {};
    }
}

float AuraCompressorEngine::computeGainReductionDb (float inputLevelDb, float thresholdDb,
                                                     float ratio, float kneeDb) noexcept
{
    const float overDb = inputLevelDb - thresholdDb;
    if (kneeDb <= 0.0f)
    {
        if (overDb <= 0.0f) return 0.0f;
        return -(overDb - (overDb / ratio));
    }
    const float halfKnee = kneeDb * 0.5f;
    if (overDb <= -halfKnee) return 0.0f;
    if (overDb >= halfKnee)  return -(overDb - (overDb / ratio));
    const float x = overDb + halfKnee;
    return -((x * x) / (2.0f * kneeDb)) * (1.0f - 1.0f / ratio);
}

float AuraCompressorEngine::softSaturate (float x, float drive) const noexcept
{
    if (drive < 0.001f) return x;
    const float d = 1.0f + drive * 4.0f;
    return std::tanh (x * d) / d;
}

float AuraCompressorEngine::densitySaturate (float x, float drive, float density, float warmth) const noexcept
{
    const float d = 1.0f + drive * 5.0f + density * 3.0f;
    const float bias = warmth * 0.12f;
    const float odd = std::tanh ((x + bias) * d) / d;
    const float even = warmth * 0.08f * x * x * (x > 0.0f ? 1.0f : -1.0f);
    return odd + even * density;
}

void AuraCompressorEngine::updateHPFCoeffs (float hpfHz) noexcept
{
    if (hpfHz <= 1.0f)
    {
        lastHpfHz = 0.0f;
        hpfB0 = 1.0f; hpfB1 = 0.0f; hpfB2 = 0.0f; hpfA1 = 0.0f; hpfA2 = 0.0f;
        return;
    }
    if (std::abs (hpfHz - lastHpfHz) < 0.5f) return;
    lastHpfHz = hpfHz;
    const float sr    = static_cast<float> (sampleRate);
    const float omega = juce::MathConstants<float>::twoPi
                       * juce::jlimit (10.0f, 18000.0f, hpfHz) / sr;
    const float sinw  = std::sin (omega);
    const float cosw  = std::cos (omega);
    const float alpha = sinw / 1.41421356f;
    const float a0inv = 1.0f / (1.0f + alpha);
    hpfB0 =  (1.0f + cosw) * 0.5f * a0inv;
    hpfB1 = -(1.0f + cosw)        * a0inv;
    hpfB2 = hpfB0;
    hpfA1 = -2.0f * cosw          * a0inv;
    hpfA2 = (1.0f - alpha)        * a0inv;
}

float AuraCompressorEngine::processHPF (float x, int ch) noexcept
{
    if (lastHpfHz <= 1.0f) return x;
    auto& s = hpfState[static_cast<size_t> (ch)];
    const float y = hpfB0 * x + s.z1;
    s.z1 = hpfB1 * x - hpfA1 * y + s.z2;
    s.z2 = hpfB2 * x - hpfA2 * y;
    return y;
}

void AuraCompressorEngine::initSmoothers (double sr) noexcept
{
    const double ramp = 0.050;
    smInputGain.reset (sr, ramp);
    smThreshold.reset (sr, ramp);
    smRatio.reset (sr, ramp);
    smAttack.reset (sr, ramp);
    smRelease.reset (sr, ramp);
    smMix.reset (sr, ramp);
    smOutputGain.reset (sr, ramp);
    smDrive.reset (sr, ramp);
    smDensity.reset (sr, ramp);
    smWarmth.reset (sr, ramp);
    smCeiling.reset (sr, ramp);
    smSidechainHpf.reset (sr, ramp);
    smBypass.reset (sr, 0.080);
}

void AuraCompressorEngine::updateSmootherTargets (const CompressorParams& p) noexcept
{
    smInputGain.setTargetValue (juce::Decibels::decibelsToGain (p.inputGainDb));
    smThreshold.setTargetValue (p.thresholdDb);
    smRatio.setTargetValue (juce::jmax (1.0f, p.ratio));
    smAttack.setTargetValue (p.attackMs);
    smRelease.setTargetValue (p.releaseMs);
    smMix.setTargetValue (juce::jlimit (0.0f, 1.0f, p.mix));
    smOutputGain.setTargetValue (juce::Decibels::decibelsToGain (p.outputGainDb));
    smDrive.setTargetValue (juce::jlimit (0.0f, 1.0f, p.drive));
    smDensity.setTargetValue (juce::jlimit (0.0f, 1.0f, p.density));
    smWarmth.setTargetValue (juce::jlimit (0.0f, 1.0f, p.warmth));
    smCeiling.setTargetValue (juce::Decibels::decibelsToGain (p.ceilingDb));
    smSidechainHpf.setTargetValue (p.sidechainHpfHz);
    smBypass.setTargetValue (p.bypass ? 0.0f : 1.0f);
}

float AuraCompressorEngine::getSmoothedInputGain() noexcept { return smInputGain.getNextValue(); }
float AuraCompressorEngine::getSmoothedThreshold() noexcept { return smThreshold.getNextValue(); }
float AuraCompressorEngine::getSmoothedRatio() noexcept { return smRatio.getNextValue(); }
float AuraCompressorEngine::getSmoothedAttackMs() noexcept { return smAttack.getNextValue(); }
float AuraCompressorEngine::getSmoothedReleaseMs() noexcept { return smRelease.getNextValue(); }
float AuraCompressorEngine::getSmoothedMix() noexcept { return smMix.getNextValue(); }
float AuraCompressorEngine::getSmoothedOutputGain() noexcept { return smOutputGain.getNextValue(); }
float AuraCompressorEngine::getSmoothedDrive() noexcept { return smDrive.getNextValue(); }
float AuraCompressorEngine::getSmoothedDensity() noexcept { return smDensity.getNextValue(); }
float AuraCompressorEngine::getSmoothedWarmth() noexcept { return smWarmth.getNextValue(); }
float AuraCompressorEngine::getSmoothedCeiling() noexcept { return smCeiling.getNextValue(); }
float AuraCompressorEngine::getSmoothedSidechainHpf() noexcept { return smSidechainHpf.getNextValue(); }
float AuraCompressorEngine::getBypassMix() noexcept { return smBypass.getNextValue(); }

void AuraCompressorEngine::applyProfilePreset (const CompressorParams& p) noexcept
{
    if (p.model == lastModel && p.profile == lastProfile)
        return;

    lastModel   = p.model;
    lastProfile = p.profile;
    activePreset = getAuraCompressorProfilePreset (p.model, p.profile);
}

void AuraCompressorEngine::prepare (double sr, int /*maxBlockSize*/, int numCh)
{
    sampleRate  = sr;
    numChannels = numCh;
    auraLevelAdaptCoeff = 1.0f - std::exp (-1.0f / (2.0f * static_cast<float> (sr)));
    emotionLock.prepare (sr);
    initSmoothers (sr);
    updateHPFCoeffs (params.sidechainHpfHz);
    attackCoeff  = makeCoeff (params.attackMs,  sr);
    releaseCoeff = makeCoeff (params.releaseMs, sr);
    reset();
}

void AuraCompressorEngine::reset() noexcept
{
    envelope      = { 0.0f, 0.0f };
    rmsState      = { 0.0f, 0.0f };
    optoFastEnv   = { 0.0f, 0.0f };
    optoSlowEnv   = { 0.0f, 0.0f };
    punchCountdown= { 0, 0 };
    muRatio       = params.ratio;
    tubeProgramMemory = 0.0f;
    adaptiveThreshold   = 0.0f;
    auraLevelRms        = 0.0f;
    hpfState            = {};
    lastProfile         = -1;
    gainReductionDb.store (0.0f,   std::memory_order_relaxed);
    outputLevelDb.store   (-60.0f, std::memory_order_relaxed);
    targetGrDbAtomic.store (0.0f,  std::memory_order_relaxed);
    emotionLock.reset();
}

void AuraCompressorEngine::updateParameters (const CompressorParams& p)
{
    params = p;
    params.inputGainDb = p.inputGainDb != 0.0f || p.inputDb == 0.0f ? p.inputGainDb : p.inputDb;
    params.outputGainDb = p.outputGainDb != 0.0f || p.makeupDb == 0.0f ? p.outputGainDb : p.makeupDb;
    if (params.sidechainHpfHz <= 1.0f && p.sidechainHPF > 1.0f)
        params.sidechainHpfHz = p.sidechainHPF;

    applyProfilePreset (params);
    updateSmootherTargets (params);
    updateHPFCoeffs (params.sidechainHpfHz);
    attackCoeff  = makeCoeff (params.attackMs,  sampleRate);
    releaseCoeff = makeCoeff (params.releaseMs, sampleRate);
    emotionLock.setParams (params.emotionLock);

    const float guideThr = params.thresholdDb - params.amount * 12.0f;
    targetGrDbAtomic.store (juce::jlimit (0.0f, 18.0f, -guideThr * 0.15f + params.ratio * 0.5f),
                            std::memory_order_relaxed);
}

void AuraCompressorEngine::processBlock (juce::AudioBuffer<float>& buffer)
{
    if (!params.enabled || buffer.getNumSamples() == 0)
        return;

    juce::ScopedNoDenormals noDenormals;

    if (params.bypass)
    {
        const int numSamples = buffer.getNumSamples();
        const int numCh = juce::jmin (numChannels, buffer.getNumChannels());
        for (int s = 0; s < numSamples; ++s)
        {
            const float wetMix = getBypassMix();
            for (int ch = 0; ch < numCh; ++ch)
            {
                const float dry = buffer.getSample (ch, s);
                buffer.setSample (ch, s, dry * wetMix + dry * (1.0f - wetMix));
            }
        }
        gainReductionDb.store (0.0f, std::memory_order_relaxed);
        return;
    }

    switch (params.model)
    {
        case AuraCompressorModel::Aura2A:      processAura2A (buffer); break;
        case AuraCompressorModel::Aura76:      processAura76 (buffer); break;
        case AuraCompressorModel::AuraTube:    processAuraTube (buffer); break;
        case AuraCompressorModel::AuraLimiter: processAuraLimiter (buffer); break;
        case AuraCompressorModel::AuraDensity: processAuraDensity (buffer); break;
        case AuraCompressorModel::AuraDrums:   processAuraDrums (buffer); break;
        default: break;
    }
}

float AuraCompressorEngine::detectLevel (float scL, float scR, int ch, bool usePeak) noexcept
{
    const float sc = params.linkedStereo ? juce::jmax (std::abs (scL), std::abs (scR))
                                         : (ch == 0 ? std::abs (scL) : std::abs (scR));
    if (usePeak) return sc;
    const float rmsCoeff = 1.0f - std::exp (-1.0f / (0.050f * static_cast<float> (sampleRate)));
    auto& rs = rmsState[static_cast<size_t> (ch)];
    rs += (sc * sc - rs) * rmsCoeff;
    return std::sqrt (juce::jmax (0.0f, rs));
}

void AuraCompressorEngine::smoothEnvelope (float grDb, int ch, float attCoeff_, float relCoeff_) noexcept
{
    auto& env = envelope[static_cast<size_t> (ch)];
    const bool attack = grDb < env;
    env += (grDb - env) * (attack ? attCoeff_ : relCoeff_);
}

void AuraCompressorEngine::processAura76 (juce::AudioBuffer<float>& buffer)
{
    constexpr float kneeDb = 1.0f;
    const float relMod   = emotionLock.getReleaseModifier();
    const float satMod   = emotionLock.getSaturationModifier();

    const int numSamples = buffer.getNumSamples();
    const int numCh      = juce::jmin (numChannels, buffer.getNumChannels());
    float peakGR = 0.0f, peakOut = 0.0f;

    for (int s = 0; s < numSamples; ++s)
    {
        const float inputGain  = getSmoothedInputGain();
        const float makeupGain = getSmoothedOutputGain();
        const float mix        = getSmoothedMix();
        const float ratio      = getSmoothedRatio();
        const float effectiveThr = getSmoothedThreshold() - params.amount * 12.0f;
        const float attCoeff   = makeCoeff (getSmoothedAttackMs() * 0.08f, sampleRate);
        const float relCoeff   = makeCoeff (getSmoothedReleaseMs() * relMod, sampleRate);
        const float satDrive     = getSmoothedDrive() * satMod + params.saturation * satMod;
        updateHPFCoeffs (getSmoothedSidechainHpf());

        float inL = buffer.getSample (0, s) * inputGain;
        float inR = numCh > 1 ? buffer.getSample (1, s) * inputGain : inL;

        const float scL = processHPF (inL, 0);
        const float scR = numCh > 1 ? processHPF (inR, 1) : scL;

        const float detL = params.linkedStereo ? juce::jmax (std::abs (scL), std::abs (scR)) : std::abs (scL);
        const float detR = params.linkedStereo ? detL : std::abs (scR);

        emotionLock.analyzeSample ((detL + detR) * 0.5f);

        for (int ch = 0; ch < numCh; ++ch)
        {
            const float det   = (ch == 0) ? detL : detR;
            const float detDb = juce::Decibels::gainToDecibels (det + 1e-9f);
            float grDb        = computeGainReductionDb (detDb, effectiveThr, ratio, kneeDb);
            grDb              = emotionLock.processGainReductionDb (grDb);
            grDb              = juce::jlimit (-40.0f, 0.0f, grDb);
            smoothEnvelope (grDb, ch, attCoeff, relCoeff);
            peakGR = juce::jmin (peakGR, envelope[static_cast<size_t> (ch)]);
        }

        const float gainL = juce::Decibels::decibelsToGain (envelope[0]);
        const float gainR = numCh > 1 ? juce::Decibels::decibelsToGain (envelope[1]) : gainL;

        const float dryL = inL * makeupGain;
        const float dryR = inR * makeupGain;
        const float wetL = softSaturate (inL * gainL, satDrive) * makeupGain;
        const float wetR = softSaturate (inR * gainR, satDrive) * makeupGain;

        buffer.setSample (0, s, juce::jmap (mix, dryL, wetL));
        if (numCh > 1) buffer.setSample (1, s, juce::jmap (mix, dryR, wetR));
        peakOut = juce::jmax (peakOut, std::abs (buffer.getSample (0, s)), numCh > 1 ? std::abs (buffer.getSample (1, s)) : 0.0f);
    }

    gainReductionDb.store (peakGR, std::memory_order_relaxed);
    outputLevelDb.store   (juce::Decibels::gainToDecibels (peakOut + 1e-9f), std::memory_order_relaxed);
}

void AuraCompressorEngine::processAura2A (juce::AudioBuffer<float>& buffer)
{
    constexpr float kneeDb = 8.0f;
    const float relMod     = emotionLock.getReleaseModifier();
    const float satMod     = emotionLock.getSaturationModifier();

    const int numSamples = buffer.getNumSamples();
    const int numCh      = juce::jmin (numChannels, buffer.getNumChannels());
    float peakGR = 0.0f, peakOut = 0.0f;

    for (int s = 0; s < numSamples; ++s)
    {
        const float inputGain    = getSmoothedInputGain();
        const float makeupGain   = getSmoothedOutputGain();
        const float mix          = getSmoothedMix();
        const float ratio        = getSmoothedRatio();
        const float warmth       = getSmoothedWarmth();
        const float peakReduction = params.amount;
        const float effectiveThr = getSmoothedThreshold() - peakReduction * 18.0f;
        const float attCoeff     = makeCoeff (juce::jlimit (10.0f, 80.0f, getSmoothedAttackMs()), sampleRate);
        const float fastRel      = makeCoeff (juce::jmax (60.0f, activePreset.fastReleaseMs) * relMod, sampleRate);
        const float slowRel      = makeCoeff (juce::jmax (500.0f, getSmoothedReleaseMs()) * relMod, sampleRate);
        const float satDrive     = (warmth * 0.5f + params.saturation) * satMod;

        float inL = buffer.getSample (0, s) * inputGain;
        float inR = numCh > 1 ? buffer.getSample (1, s) * inputGain : inL;

        const float scL = inL;
        const float scR = inR;

        const float rmsCoeff = makeCoeff (50.0f, sampleRate);
        const float powerL   = params.linkedStereo ? (scL * scL + scR * scR) * 0.5f : scL * scL;
        const float powerR   = params.linkedStereo ? powerL : scR * scR;
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto& rs = rmsState[static_cast<size_t> (ch)];
            rs += ((ch == 0 ? powerL : powerR) - rs) * rmsCoeff;
        }

        const float detL = std::sqrt (juce::jmax (0.0f, rmsState[0]) + 1e-18f);
        const float detR = numCh > 1 ? std::sqrt (juce::jmax (0.0f, rmsState[1]) + 1e-18f) : detL;

        emotionLock.analyzeSample ((detL + detR) * 0.5f);

        for (int ch = 0; ch < numCh; ++ch)
        {
            const float det   = (ch == 0) ? detL : detR;
            const float detDb = juce::Decibels::gainToDecibels (det + 1e-9f);
            float grDb        = computeGainReductionDb (detDb, effectiveThr, ratio, kneeDb);
            grDb              = emotionLock.processGainReductionDb (grDb);
            grDb              = juce::jlimit (-40.0f, 0.0f, grDb);

            auto& fast = optoFastEnv[static_cast<size_t> (ch)];
            auto& slow = optoSlowEnv[static_cast<size_t> (ch)];
            fast += (grDb - fast) * (grDb < fast ? attCoeff : fastRel);
            slow += (grDb - slow) * (grDb < slow ? attCoeff : slowRel);
            envelope[static_cast<size_t> (ch)] = fast * 0.5f + slow * 0.5f;
            peakGR = juce::jmin (peakGR, envelope[static_cast<size_t> (ch)]);
        }

        const float gainL = juce::Decibels::decibelsToGain (envelope[0]);
        const float gainR = numCh > 1 ? juce::Decibels::decibelsToGain (envelope[1]) : gainL;

        auto evenSat = [&] (float x, float d, float w) -> float
        {
            if (d < 0.001f && w < 0.001f) return x;
            const float bias = 0.08f * d + 0.12f * w;
            return softSaturate (x + bias, d * 0.6f + w * 0.4f) - bias * (1.0f - d);
        };

        const float dryL = inL * makeupGain;
        const float dryR = inR * makeupGain;
        const float wetL = evenSat (inL * gainL, satDrive, warmth) * makeupGain;
        const float wetR = evenSat (inR * gainR, satDrive, warmth) * makeupGain;

        buffer.setSample (0, s, juce::jmap (mix, dryL, wetL));
        if (numCh > 1) buffer.setSample (1, s, juce::jmap (mix, dryR, wetR));
        peakOut = juce::jmax (peakOut, std::abs (buffer.getSample (0, s)));
    }

    gainReductionDb.store (peakGR, std::memory_order_relaxed);
    outputLevelDb.store   (juce::Decibels::gainToDecibels (peakOut + 1e-9f), std::memory_order_relaxed);
}

void AuraCompressorEngine::processAuraTube (juce::AudioBuffer<float>& buffer)
{
    constexpr float kneeDb = 10.0f;
    const float relMod     = emotionLock.getReleaseModifier();
    const float satMod     = emotionLock.getSaturationModifier();

    const int numSamples = buffer.getNumSamples();
    const int numCh      = juce::jmin (numChannels, buffer.getNumChannels());
    float peakGR = 0.0f, peakOut = 0.0f;
    const float rmsCoeff = makeCoeff (40.0f, sampleRate);

    for (int s = 0; s < numSamples; ++s)
    {
        const float inputGain  = getSmoothedInputGain();
        const float makeupGain = getSmoothedOutputGain();
        const float mix        = getSmoothedMix();
        const float ratio      = getSmoothedRatio();
        const float effectiveThr = getSmoothedThreshold() - params.amount * 12.0f;
        const float satDrive   = (getSmoothedDrive() + params.saturation) * satMod * 1.2f;
        updateHPFCoeffs (getSmoothedSidechainHpf());

        float manualAttack = getSmoothedAttackMs();
        float manualRelease = getSmoothedReleaseMs();
        if (params.timingMode == AuraTimingMode::Fixed)
        {
            manualAttack  = activePreset.attackMs;
            manualRelease = activePreset.releaseMs;
        }
        else if (params.timingMode == AuraTimingMode::Hybrid)
        {
            manualRelease = juce::jmap (tubeProgramMemory, 0.0f, 1.0f,
                                        activePreset.releaseMs, activePreset.slowReleaseMs);
        }

        const float attCoeff = makeCoeff (manualAttack * (params.timingMode == AuraTimingMode::Manual ? 1.0f : 2.5f), sampleRate);
        const float relCoeff = makeCoeff (manualRelease * relMod, sampleRate);
        const float maxRatio = ratio * 2.0f;

        float inL = buffer.getSample (0, s) * inputGain;
        float inR = numCh > 1 ? buffer.getSample (1, s) * inputGain : inL;

        const float scL = processHPF (inL, 0);
        const float scR = numCh > 1 ? processHPF (inR, 1) : scL;

        const float power = (scL * scL + scR * scR) * 0.5f;
        rmsState[0] += (power - rmsState[0]) * rmsCoeff;
        rmsState[1]  = rmsState[0];
        const float det = std::sqrt (juce::jmax (0.0f, rmsState[0]) + 1e-18f);

        emotionLock.analyzeSample (det);
        tubeProgramMemory += (det - tubeProgramMemory) * 0.0015f;

        const float detDb = juce::Decibels::gainToDecibels (det + 1e-9f);
        const float overDb = juce::jmax (0.0f, detDb - effectiveThr);
        const float varRatio = juce::jlimit (ratio, maxRatio, ratio + overDb * 0.25f);
        muRatio += (varRatio - muRatio) * 0.002f;

        for (int ch = 0; ch < numCh; ++ch)
        {
            float grDb = computeGainReductionDb (detDb, effectiveThr, muRatio, kneeDb);
            grDb       = emotionLock.processGainReductionDb (grDb);
            grDb       = juce::jlimit (-40.0f, 0.0f, grDb);
            smoothEnvelope (grDb, ch, attCoeff, relCoeff);
            peakGR = juce::jmin (peakGR, envelope[static_cast<size_t> (ch)]);
        }

        const float gainL = juce::Decibels::decibelsToGain (envelope[0]);
        const float gainR = numCh > 1 ? juce::Decibels::decibelsToGain (envelope[1]) : gainL;

        auto tubeSat = [&] (float x, float d) -> float
        {
            if (d < 0.001f) return x;
            const float drive = 1.0f + d * 6.0f;
            const float tanhPart = std::tanh (x * drive) / drive;
            const float evenBias = 0.15f * d * x * x * (x > 0.0f ? 1.0f : -1.0f);
            return tanhPart + evenBias;
        };

        const float dryL = inL * makeupGain;
        const float dryR = inR * makeupGain;
        const float wetL = tubeSat (inL * gainL, satDrive) * makeupGain;
        const float wetR = tubeSat (inR * gainR, satDrive) * makeupGain;

        buffer.setSample (0, s, juce::jmap (mix, dryL, wetL));
        if (numCh > 1) buffer.setSample (1, s, juce::jmap (mix, dryR, wetR));
        peakOut = juce::jmax (peakOut, std::abs (buffer.getSample (0, s)));
    }

    gainReductionDb.store (peakGR, std::memory_order_relaxed);
    outputLevelDb.store   (juce::Decibels::gainToDecibels (peakOut + 1e-9f), std::memory_order_relaxed);
}

void AuraCompressorEngine::processAuraLimiter (juce::AudioBuffer<float>& buffer)
{
    constexpr float kneeDb = 4.0f;
    const float relMod = emotionLock.getReleaseModifier();

    const int numSamples = buffer.getNumSamples();
    const int numCh      = juce::jmin (numChannels, buffer.getNumChannels());
    float peakGR = 0.0f, peakOut = 0.0f;

    for (int s = 0; s < numSamples; ++s)
    {
        const float inputGain  = getSmoothedInputGain();
        const float makeupGain = getSmoothedOutputGain();
        const float ceiling    = getSmoothedCeiling();
        const float effectiveThr = getSmoothedThreshold();
        const float attCoeff = makeCoeff (juce::jlimit (0.1f, 2.0f, getSmoothedAttackMs()), sampleRate);
        const float relCoeff = makeCoeff (juce::jlimit (40.0f, 180.0f, getSmoothedReleaseMs()) * relMod, sampleRate);
        const float ratio = juce::jmax (12.0f, getSmoothedRatio());

        float inL = buffer.getSample (0, s) * inputGain;
        float inR = numCh > 1 ? buffer.getSample (1, s) * inputGain : inL;

        const float detL = params.linkedStereo ? juce::jmax (std::abs (inL), std::abs (inR)) : std::abs (inL);
        const float detR = params.linkedStereo ? detL : std::abs (inR);

        for (int ch = 0; ch < numCh; ++ch)
        {
            const float det   = (ch == 0) ? detL : detR;
            const float detDb = juce::Decibels::gainToDecibels (det + 1e-9f);
            float grDb        = computeGainReductionDb (detDb, effectiveThr, ratio, kneeDb);
            grDb              = juce::jlimit (-24.0f, 0.0f, grDb);
            smoothEnvelope (grDb, ch, attCoeff, relCoeff);
            peakGR = juce::jmin (peakGR, envelope[static_cast<size_t> (ch)]);
        }

        const float gainL = juce::Decibels::decibelsToGain (envelope[0]);
        const float gainR = numCh > 1 ? juce::Decibels::decibelsToGain (envelope[1]) : gainL;

        float outL = inL * gainL * makeupGain;
        float outR = inR * gainR * makeupGain;

        auto softCeiling = [ceiling] (float x)
        {
            const float absX = std::abs (x);
            if (absX <= ceiling) return x;
            const float over = absX - ceiling;
            const float softened = ceiling + std::tanh (over * 4.0f) * (1.0f - ceiling) * 0.25f;
            return softened * (x >= 0.0f ? 1.0f : -1.0f);
        };

        outL = softCeiling (outL);
        outR = softCeiling (outR);

        buffer.setSample (0, s, outL);
        if (numCh > 1) buffer.setSample (1, s, outR);
        peakOut = juce::jmax (peakOut, std::abs (outL), std::abs (outR));
    }

    gainReductionDb.store (peakGR, std::memory_order_relaxed);
    outputLevelDb.store   (juce::Decibels::gainToDecibels (peakOut + 1e-9f), std::memory_order_relaxed);
}

void AuraCompressorEngine::processAuraDensity (juce::AudioBuffer<float>& buffer)
{
    constexpr float kneeDb = 3.0f;
    const float relMod = emotionLock.getReleaseModifier();
    const float satMod = emotionLock.getSaturationModifier();

    const int numSamples = buffer.getNumSamples();
    const int numCh      = juce::jmin (numChannels, buffer.getNumChannels());
    float peakGR = 0.0f, peakOut = 0.0f;
    const float rmsCoeff = makeCoeff (20.0f, sampleRate);

    for (int s = 0; s < numSamples; ++s)
    {
        const float inputGain  = getSmoothedInputGain();
        const float makeupGain = getSmoothedOutputGain();
        const float mix        = getSmoothedMix();
        const float ratio      = getSmoothedRatio();
        const float drive      = getSmoothedDrive() * satMod + params.saturation * satMod;
        const float density    = getSmoothedDensity();
        const float warmth     = getSmoothedWarmth();
        const float effectiveThr = getSmoothedThreshold() - params.amount * 12.0f;
        const float attCoeff = makeCoeff (getSmoothedAttackMs(), sampleRate);
        const float relCoeff = makeCoeff (getSmoothedReleaseMs() * relMod, sampleRate);

        float inL = buffer.getSample (0, s) * inputGain;
        float inR = numCh > 1 ? buffer.getSample (1, s) * inputGain : inL;

        const float scL = inL;
        const float scR = inR;
        const float powerL = params.linkedStereo ? (scL * scL + scR * scR) * 0.5f : scL * scL;
        const float powerR = params.linkedStereo ? powerL : scR * scR;
        rmsState[0] += (powerL - rmsState[0]) * rmsCoeff;
        rmsState[1] += (powerR - rmsState[1]) * rmsCoeff;

        const float detL = std::sqrt (juce::jmax (0.0f, rmsState[0]) + 1e-18f);
        const float detR = std::sqrt (juce::jmax (0.0f, rmsState[1]) + 1e-18f);

        emotionLock.analyzeSample ((detL + detR) * 0.5f);

        for (int ch = 0; ch < numCh; ++ch)
        {
            const float det   = (ch == 0) ? detL : detR;
            const float detDb = juce::Decibels::gainToDecibels (det + 1e-9f);
            float grDb        = computeGainReductionDb (detDb, effectiveThr, ratio, kneeDb);
            grDb              = emotionLock.processGainReductionDb (grDb);
            grDb              = juce::jlimit (-40.0f, 0.0f, grDb);
            smoothEnvelope (grDb, ch, attCoeff, relCoeff);
            peakGR = juce::jmin (peakGR, envelope[static_cast<size_t> (ch)]);
        }

        const float gainL = juce::Decibels::decibelsToGain (envelope[0]);
        const float gainR = numCh > 1 ? juce::Decibels::decibelsToGain (envelope[1]) : gainL;

        const float dryL = inL * makeupGain;
        const float dryR = inR * makeupGain;
        const float wetL = densitySaturate (inL * gainL, drive, density, warmth) * makeupGain;
        const float wetR = densitySaturate (inR * gainR, drive, density, warmth) * makeupGain;

        buffer.setSample (0, s, juce::jmap (mix, dryL, wetL));
        if (numCh > 1) buffer.setSample (1, s, juce::jmap (mix, dryR, wetR));
        peakOut = juce::jmax (peakOut, std::abs (buffer.getSample (0, s)));
    }

    gainReductionDb.store (peakGR, std::memory_order_relaxed);
    outputLevelDb.store   (juce::Decibels::gainToDecibels (peakOut + 1e-9f), std::memory_order_relaxed);
}

void AuraCompressorEngine::processAuraDrums (juce::AudioBuffer<float>& buffer)
{
    constexpr float kneeDb = 4.0f;
    const float relMod = emotionLock.getReleaseModifier();
    const float satMod = emotionLock.getSaturationModifier();

    const int numSamples = buffer.getNumSamples();
    const int numCh      = juce::jmin (numChannels, buffer.getNumChannels());
    float peakGR = 0.0f, peakOut = 0.0f;
    const float rmsCoeff = makeCoeff (30.0f, sampleRate);

    for (int s = 0; s < numSamples; ++s)
    {
        const float inputGain  = getSmoothedInputGain();
        const float makeupGain = getSmoothedOutputGain();
        const float mix        = getSmoothedMix();
        const float ratio      = getSmoothedRatio();
        const float drive      = getSmoothedDrive() * satMod;
        const float effectiveThr = getSmoothedThreshold() - params.amount * 12.0f;
        const float attCoeff = makeCoeff (juce::jmax (0.5f, getSmoothedAttackMs()), sampleRate);
        const float relCoeff = makeCoeff (getSmoothedReleaseMs() * relMod, sampleRate);
        updateHPFCoeffs (getSmoothedSidechainHpf());

        float inL = buffer.getSample (0, s) * inputGain;
        float inR = numCh > 1 ? buffer.getSample (1, s) * inputGain : inL;

        const float scL = processHPF (inL, 0);
        const float scR = numCh > 1 ? processHPF (inR, 1) : scL;

        const float power = (scL * scL + scR * scR) * 0.5f;
        rmsState[0] += (power - rmsState[0]) * rmsCoeff;
        rmsState[1]  = rmsState[0];
        const float det = std::sqrt (juce::jmax (0.0f, rmsState[0]) + 1e-18f);

        emotionLock.analyzeSample (det);
        const float detDb = juce::Decibels::gainToDecibels (det + 1e-9f);

        for (int ch = 0; ch < numCh; ++ch)
        {
            float grDb = computeGainReductionDb (detDb, effectiveThr, ratio, kneeDb);
            grDb       = emotionLock.processGainReductionDb (grDb);
            grDb       = juce::jlimit (-40.0f, 0.0f, grDb);
            smoothEnvelope (grDb, ch, attCoeff, relCoeff);
            peakGR = juce::jmin (peakGR, envelope[static_cast<size_t> (ch)]);
        }

        const float sharedGain = juce::Decibels::decibelsToGain (
            (envelope[0] + (numCh > 1 ? envelope[1] : envelope[0])) * 0.5f);

        const float dryL = inL * makeupGain;
        const float dryR = inR * makeupGain;
        const float wetL = softSaturate (inL * sharedGain, drive) * makeupGain;
        const float wetR = softSaturate (inR * sharedGain, drive) * makeupGain;

        buffer.setSample (0, s, juce::jmap (mix, dryL, wetL));
        if (numCh > 1) buffer.setSample (1, s, juce::jmap (mix, dryR, wetR));
        peakOut = juce::jmax (peakOut, std::abs (buffer.getSample (0, s)));
    }

    gainReductionDb.store (peakGR, std::memory_order_relaxed);
    outputLevelDb.store   (juce::Decibels::gainToDecibels (peakOut + 1e-9f), std::memory_order_relaxed);
}
