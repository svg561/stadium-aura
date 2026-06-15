#include "AuraCompressorEngine.h"

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static inline float makeCoeff (float timeMs, double sr) noexcept
{
    return 1.0f - std::exp (-1.0f / (0.001f * timeMs * static_cast<float> (sr) + 1e-9f));
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

void AuraCompressorEngine::updateHPFCoeffs (float hpfHz) noexcept
{
    if (std::abs (hpfHz - lastHpfHz) < 0.5f) return;
    lastHpfHz = hpfHz;
    const float sr    = static_cast<float> (sampleRate);
    const float omega = juce::MathConstants<float>::twoPi
                       * juce::jlimit (10.0f, 18000.0f, hpfHz) / sr;
    const float sinw  = std::sin (omega);
    const float cosw  = std::cos (omega);
    const float alpha = sinw / 1.41421356f;  // Q=0.7071 Butterworth
    const float a0inv = 1.0f / (1.0f + alpha);
    hpfB0 =  (1.0f + cosw) * 0.5f * a0inv;
    hpfB1 = -(1.0f + cosw)        * a0inv;
    hpfB2 = hpfB0;
    hpfA1 = -2.0f * cosw          * a0inv;
    hpfA2 = (1.0f - alpha)        * a0inv;
}

float AuraCompressorEngine::processHPF (float x, int ch) noexcept
{
    auto& s = hpfState[static_cast<size_t> (ch)];
    const float y = hpfB0 * x + s.z1;
    s.z1 = hpfB1 * x - hpfA1 * y + s.z2;
    s.z2 = hpfB2 * x - hpfA2 * y;
    return y;
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────────────────────

void AuraCompressorEngine::prepare (double sr, int /*maxBlockSize*/, int numCh)
{
    sampleRate  = sr;
    numChannels = numCh;
    auraLevelAdaptCoeff = 1.0f - std::exp (-1.0f / (2.0f * static_cast<float> (sr)));
    emotionLock.prepare (sr);
    updateHPFCoeffs (params.sidechainHPF);
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
    adaptiveThreshold   = 0.0f;
    auraLevelRms        = 0.0f;
    hpfState            = {};
    gainReductionDb.store (0.0f,   std::memory_order_relaxed);
    outputLevelDb.store   (-60.0f, std::memory_order_relaxed);
    emotionLock.reset();
}

void AuraCompressorEngine::updateParameters (const CompressorParams& p)
{
    params = p;
    updateHPFCoeffs (params.sidechainHPF);
    attackCoeff  = makeCoeff (params.attackMs,  sampleRate);
    releaseCoeff = makeCoeff (params.releaseMs, sampleRate);
    emotionLock.setParams (params.emotionLock);
}

// ─────────────────────────────────────────────────────────────────────────────
// processBlock dispatcher
// ─────────────────────────────────────────────────────────────────────────────

void AuraCompressorEngine::processBlock (juce::AudioBuffer<float>& buffer)
{
    if (!params.enabled || buffer.getNumSamples() == 0) return;

    switch (params.model)
    {
        case AuraCompressorModel::LightningFET: processLightningFET (buffer); break;
        case AuraCompressorModel::VelvetOpto:   processVelvetOpto   (buffer); break;
        case AuraCompressorModel::CrownMu:      processCrownMu      (buffer); break;
        case AuraCompressorModel::PunchCell:    processPunchCell    (buffer); break;
        case AuraCompressorModel::GlueBus:      processGlueBus      (buffer); break;
        case AuraCompressorModel::ModernClean:  processModernClean  (buffer); break;
        default: break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Shared per-sample helpers
// ─────────────────────────────────────────────────────────────────────────────

// Peak or weighted-RMS detector for one channel (uses rmsState for RMS)
float AuraCompressorEngine::detectLevel (float scL, float scR, int ch, bool usePeak) noexcept
{
    const float sc = params.linkedStereo ? juce::jmax (std::abs (scL), std::abs (scR))
                                         : (ch == 0 ? std::abs (scL) : std::abs (scR));
    if (usePeak) return sc;
    // Leaky-peak RMS approximation (fast rise, slow decay)
    const float rmsCoeff = 1.0f - std::exp (-1.0f / (0.050f * static_cast<float> (sampleRate)));
    auto& rs = rmsState[static_cast<size_t> (ch)];
    rs += (sc * sc - rs) * rmsCoeff;
    return std::sqrt (juce::jmax (0.0f, rs));
}

// Standard envelope follower (negative dB convention: more negative = more compression)
void AuraCompressorEngine::smoothEnvelope (float grDb, int ch, float attCoeff_, float relCoeff_) noexcept
{
    auto& env = envelope[static_cast<size_t> (ch)];
    const bool attack = grDb < env;
    env += (grDb - env) * (attack ? attCoeff_ : relCoeff_);
}

// ─────────────────────────────────────────────────────────────────────────────
// MODEL 1 — Lightning FET  (fast FET, hard knee, odd harmonics)
// ─────────────────────────────────────────────────────────────────────────────

void AuraCompressorEngine::processLightningFET (juce::AudioBuffer<float>& buffer)
{
    constexpr float kneeDb    = 1.0f;
    const float inputGain     = juce::Decibels::decibelsToGain (params.inputDb);
    const float makeupGain    = juce::Decibels::decibelsToGain (params.makeupDb);
    const float relMod        = emotionLock.getReleaseModifier();
    const float satMod        = emotionLock.getSaturationModifier();
    // FET: very fast attack, moderate release
    const float attCoeff      = makeCoeff (params.attackMs * 0.08f, sampleRate);  // ~0.8 ms equivalent
    const float relCoeff      = makeCoeff (params.releaseMs * relMod, sampleRate);
    const float effectiveThr  = params.thresholdDb - params.amount * 12.0f;
    const float satDrive      = params.saturation * satMod;

    const int numSamples = buffer.getNumSamples();
    const int numCh      = juce::jmin (numChannels, buffer.getNumChannels());
    float peakGR = 0.0f, peakOut = 0.0f;

    for (int s = 0; s < numSamples; ++s)
    {
        float inL = buffer.getSample (0, s) * inputGain;
        float inR = numCh > 1 ? buffer.getSample (1, s) * inputGain : inL;

        // Sidechain HPF
        const float scL = processHPF (inL, 0);
        const float scR = numCh > 1 ? processHPF (inR, 1) : scL;

        // Peak detection (FET responds to peaks)
        const float detL = params.linkedStereo ? juce::jmax (std::abs (scL), std::abs (scR)) : std::abs (scL);
        const float detR = params.linkedStereo ? detL : std::abs (scR);

        // Emotion lock analysis
        emotionLock.analyzeSample ((detL + detR) * 0.5f);

        for (int ch = 0; ch < numCh; ++ch)
        {
            const float det   = (ch == 0) ? detL : detR;
            const float detDb = juce::Decibels::gainToDecibels (det + 1e-9f);
            float grDb        = computeGainReductionDb (detDb, effectiveThr, params.ratio, kneeDb);
            grDb              = emotionLock.processGainReductionDb (grDb);
            grDb              = juce::jlimit (-40.0f, 0.0f, grDb);
            smoothEnvelope (grDb, ch, attCoeff, relCoeff);
            peakGR = juce::jmin (peakGR, envelope[static_cast<size_t> (ch)]);
        }

        const float gainL = juce::Decibels::decibelsToGain (envelope[0]);
        const float gainR = numCh > 1 ? juce::Decibels::decibelsToGain (envelope[1]) : gainL;

        // Odd-harmonic FET saturation
        const float wetL = softSaturate (inL * gainL, satDrive) * makeupGain;
        const float wetR = softSaturate (inR * gainR, satDrive) * makeupGain;

        const float outL = juce::jmap (params.mix, inL * makeupGain, wetL);
        const float outR = juce::jmap (params.mix, inR * makeupGain, wetR);

        buffer.setSample (0, s, outL);
        if (numCh > 1) buffer.setSample (1, s, outR);
        peakOut = juce::jmax (peakOut, std::abs (outL), std::abs (outR));
    }

    gainReductionDb.store (peakGR, std::memory_order_relaxed);
    outputLevelDb.store   (juce::Decibels::gainToDecibels (peakOut + 1e-9f), std::memory_order_relaxed);
}

// ─────────────────────────────────────────────────────────────────────────────
// MODEL 2 — Velvet Opto  (slow opto, soft knee, two-stage release)
// ─────────────────────────────────────────────────────────────────────────────

void AuraCompressorEngine::processVelvetOpto (juce::AudioBuffer<float>& buffer)
{
    constexpr float kneeDb  = 8.0f;
    const float inputGain   = juce::Decibels::decibelsToGain (params.inputDb);
    const float makeupGain  = juce::Decibels::decibelsToGain (params.makeupDb);
    const float relMod      = emotionLock.getReleaseModifier();
    const float satMod      = emotionLock.getSaturationModifier();
    const float attCoeff    = makeCoeff (params.attackMs, sampleRate);
    const float fastRel     = makeCoeff (60.0f * relMod, sampleRate);
    const float slowRel     = makeCoeff (params.releaseMs * relMod, sampleRate);
    const float effectiveThr = params.thresholdDb - params.amount * 12.0f;
    const float satDrive    = params.saturation * satMod * 0.5f;  // even harmonics, gentler

    const int numSamples = buffer.getNumSamples();
    const int numCh      = juce::jmin (numChannels, buffer.getNumChannels());
    float peakGR = 0.0f, peakOut = 0.0f;

    for (int s = 0; s < numSamples; ++s)
    {
        float inL = buffer.getSample (0, s) * inputGain;
        float inR = numCh > 1 ? buffer.getSample (1, s) * inputGain : inL;

        const float scL = processHPF (inL, 0);
        const float scR = numCh > 1 ? processHPF (inR, 1) : scL;

        // RMS detection with slow window (~50ms)
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
            float grDb        = computeGainReductionDb (detDb, effectiveThr, params.ratio, kneeDb);
            grDb              = emotionLock.processGainReductionDb (grDb);
            grDb              = juce::jlimit (-40.0f, 0.0f, grDb);

            // Two-stage envelope (opto program-dependent release)
            auto& fast = optoFastEnv[static_cast<size_t> (ch)];
            auto& slow = optoSlowEnv[static_cast<size_t> (ch)];
            fast += (grDb - fast) * (grDb < fast ? attCoeff : fastRel);
            slow += (grDb - slow) * (grDb < slow ? attCoeff : slowRel);
            envelope[static_cast<size_t> (ch)] = fast * 0.5f + slow * 0.5f;
            peakGR = juce::jmin (peakGR, envelope[static_cast<size_t> (ch)]);
        }

        const float gainL = juce::Decibels::decibelsToGain (envelope[0]);
        const float gainR = numCh > 1 ? juce::Decibels::decibelsToGain (envelope[1]) : gainL;

        // Slight even-order coloration via asymmetric saturation
        auto evenSat = [&] (float x, float d) -> float
        {
            if (d < 0.001f) return x;
            const float bias = 0.08f * d;
            return softSaturate (x + bias, d * 0.6f) - bias * (1.0f - d);
        };
        const float wetL = evenSat (inL * gainL, satDrive) * makeupGain;
        const float wetR = evenSat (inR * gainR, satDrive) * makeupGain;

        const float outL = juce::jmap (params.mix, inL * makeupGain, wetL);
        const float outR = juce::jmap (params.mix, inR * makeupGain, wetR);
        buffer.setSample (0, s, outL);
        if (numCh > 1) buffer.setSample (1, s, outR);
        peakOut = juce::jmax (peakOut, std::abs (outL), std::abs (outR));
    }

    gainReductionDb.store (peakGR, std::memory_order_relaxed);
    outputLevelDb.store   (juce::Decibels::gainToDecibels (peakOut + 1e-9f), std::memory_order_relaxed);
}

// ─────────────────────────────────────────────────────────────────────────────
// MODEL 3 — Crown Mu  (vari-mu, variable ratio, very soft knee, stereo link)
// ─────────────────────────────────────────────────────────────────────────────

void AuraCompressorEngine::processCrownMu (juce::AudioBuffer<float>& buffer)
{
    constexpr float kneeDb  = 10.0f;
    const float inputGain   = juce::Decibels::decibelsToGain (params.inputDb);
    const float makeupGain  = juce::Decibels::decibelsToGain (params.makeupDb);
    const float relMod      = emotionLock.getReleaseModifier();
    const float satMod      = emotionLock.getSaturationModifier();
    // Vari-mu: very slow attack, moderate release
    const float attCoeff    = makeCoeff (params.attackMs * 2.5f, sampleRate);
    const float relCoeff    = makeCoeff (params.releaseMs * relMod, sampleRate);
    const float effectiveThr = params.thresholdDb - params.amount * 12.0f;
    const float satDrive    = params.saturation * satMod * 1.2f;  // high tube saturation
    const float maxRatio    = params.ratio * 2.0f;

    const int numSamples = buffer.getNumSamples();
    const int numCh      = juce::jmin (numChannels, buffer.getNumChannels());
    float peakGR = 0.0f, peakOut = 0.0f;

    // Vari-mu: always stereo linked
    const float rmsCoeff = makeCoeff (40.0f, sampleRate);

    for (int s = 0; s < numSamples; ++s)
    {
        float inL = buffer.getSample (0, s) * inputGain;
        float inR = numCh > 1 ? buffer.getSample (1, s) * inputGain : inL;

        const float scL = processHPF (inL, 0);
        const float scR = numCh > 1 ? processHPF (inR, 1) : scL;

        // Linked stereo RMS
        const float power = (scL * scL + scR * scR) * 0.5f;
        rmsState[0] += (power - rmsState[0]) * rmsCoeff;
        rmsState[1]  = rmsState[0];
        const float det = std::sqrt (juce::jmax (0.0f, rmsState[0]) + 1e-18f);

        emotionLock.analyzeSample (det);

        const float detDb = juce::Decibels::gainToDecibels (det + 1e-9f);
        const float overDb = juce::jmax (0.0f, detDb - effectiveThr);

        // Variable ratio increases with level above threshold
        const float varRatio = juce::jlimit (params.ratio, maxRatio,
                                              params.ratio + overDb * 0.25f);
        // Smooth ratio changes
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

        // Tube-like mixed harmonics saturation
        auto tubeSat = [&] (float x, float d) -> float
        {
            if (d < 0.001f) return x;
            const float drive = 1.0f + d * 6.0f;
            // Mix tanh (odd) with x^2 bias (even) for tube character
            const float tanhPart = std::tanh (x * drive) / drive;
            const float evenBias = 0.15f * d * x * x * (x > 0.0f ? 1.0f : -1.0f);
            return tanhPart + evenBias;
        };
        const float wetL = tubeSat (inL * gainL, satDrive) * makeupGain;
        const float wetR = tubeSat (inR * gainR, satDrive) * makeupGain;

        const float outL = juce::jmap (params.mix, inL * makeupGain, wetL);
        const float outR = juce::jmap (params.mix, inR * makeupGain, wetR);
        buffer.setSample (0, s, outL);
        if (numCh > 1) buffer.setSample (1, s, outR);
        peakOut = juce::jmax (peakOut, std::abs (outL), std::abs (outR));
    }

    gainReductionDb.store (peakGR, std::memory_order_relaxed);
    outputLevelDb.store   (juce::Decibels::gainToDecibels (peakOut + 1e-9f), std::memory_order_relaxed);
}

// ─────────────────────────────────────────────────────────────────────────────
// MODEL 4 — Punch Cell  (VCA punch, transient window, medium knee)
// ─────────────────────────────────────────────────────────────────────────────

void AuraCompressorEngine::processPunchCell (juce::AudioBuffer<float>& buffer)
{
    constexpr float kneeDb  = 3.0f;
    const float inputGain   = juce::Decibels::decibelsToGain (params.inputDb);
    const float makeupGain  = juce::Decibels::decibelsToGain (params.makeupDb);
    const float relMod      = emotionLock.getReleaseModifier();
    const float satMod      = emotionLock.getSaturationModifier();
    const float attCoeff    = makeCoeff (params.attackMs, sampleRate);
    const float relCoeff    = makeCoeff (params.releaseMs * relMod, sampleRate);
    const float effectiveThr = params.thresholdDb - params.amount * 12.0f;
    const float satDrive    = params.saturation * satMod * 0.3f;
    const int   punchSamples = static_cast<int> (0.006f * static_cast<float> (sampleRate)); // 6 ms

    const int numSamples = buffer.getNumSamples();
    const int numCh      = juce::jmin (numChannels, buffer.getNumChannels());
    float peakGR = 0.0f, peakOut = 0.0f;

    const float rmsCoeff = makeCoeff (20.0f, sampleRate);
    const float thrGain  = juce::Decibels::decibelsToGain (effectiveThr);

    for (int s = 0; s < numSamples; ++s)
    {
        float inL = buffer.getSample (0, s) * inputGain;
        float inR = numCh > 1 ? buffer.getSample (1, s) * inputGain : inL;

        const float scL = processHPF (inL, 0);
        const float scR = numCh > 1 ? processHPF (inR, 1) : scL;

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
            float grDb        = computeGainReductionDb (detDb, effectiveThr, params.ratio, kneeDb);
            grDb              = emotionLock.processGainReductionDb (grDb);
            grDb              = juce::jlimit (-40.0f, 0.0f, grDb);

            // Punch window: when signal crosses threshold, allow transient through
            auto& pc = punchCountdown[static_cast<size_t> (ch)];
            if (det > thrGain && pc == 0)
                pc = punchSamples;
            if (pc > 0)
            {
                // During punch window: reduce GR by 50% to let transient through
                grDb *= 0.5f;
                --pc;
            }
            smoothEnvelope (grDb, ch, attCoeff, relCoeff);
            peakGR = juce::jmin (peakGR, envelope[static_cast<size_t> (ch)]);
        }

        const float gainL = juce::Decibels::decibelsToGain (envelope[0]);
        const float gainR = numCh > 1 ? juce::Decibels::decibelsToGain (envelope[1]) : gainL;

        const float wetL = softSaturate (inL * gainL, satDrive) * makeupGain;
        const float wetR = softSaturate (inR * gainR, satDrive) * makeupGain;

        const float outL = juce::jmap (params.mix, inL * makeupGain, wetL);
        const float outR = juce::jmap (params.mix, inR * makeupGain, wetR);
        buffer.setSample (0, s, outL);
        if (numCh > 1) buffer.setSample (1, s, outR);
        peakOut = juce::jmax (peakOut, std::abs (outL), std::abs (outR));
    }

    gainReductionDb.store (peakGR, std::memory_order_relaxed);
    outputLevelDb.store   (juce::Decibels::gainToDecibels (peakOut + 1e-9f), std::memory_order_relaxed);
}

// ─────────────────────────────────────────────────────────────────────────────
// MODEL 5 — Glue Bus  (bus glue, slow attack, stereo linked, minimal color)
// ─────────────────────────────────────────────────────────────────────────────

void AuraCompressorEngine::processGlueBus (juce::AudioBuffer<float>& buffer)
{
    constexpr float kneeDb  = 4.0f;
    const float inputGain   = juce::Decibels::decibelsToGain (params.inputDb);
    const float makeupGain  = juce::Decibels::decibelsToGain (params.makeupDb);
    const float relMod      = emotionLock.getReleaseModifier();
    const float satMod      = emotionLock.getSaturationModifier();
    // Glue bus: slow attack prevents pumping, medium release for consistency
    const float attCoeff    = makeCoeff (juce::jmax (params.attackMs, 20.0f), sampleRate);
    const float relCoeff    = makeCoeff (params.releaseMs * relMod, sampleRate);
    const float effectiveThr = params.thresholdDb - params.amount * 12.0f;
    const float satDrive    = params.saturation * satMod * 0.2f;  // nearly transparent

    const int numSamples = buffer.getNumSamples();
    const int numCh      = juce::jmin (numChannels, buffer.getNumChannels());
    float peakGR = 0.0f, peakOut = 0.0f;

    const float rmsCoeff = makeCoeff (30.0f, sampleRate);

    for (int s = 0; s < numSamples; ++s)
    {
        float inL = buffer.getSample (0, s) * inputGain;
        float inR = numCh > 1 ? buffer.getSample (1, s) * inputGain : inL;

        const float scL = processHPF (inL, 0);
        const float scR = numCh > 1 ? processHPF (inR, 1) : scL;

        // Always stereo-linked RMS for bus processing
        const float power = (scL * scL + scR * scR) * 0.5f;
        rmsState[0] += (power - rmsState[0]) * rmsCoeff;
        rmsState[1]  = rmsState[0];
        const float det = std::sqrt (juce::jmax (0.0f, rmsState[0]) + 1e-18f);

        emotionLock.analyzeSample (det);

        const float detDb = juce::Decibels::gainToDecibels (det + 1e-9f);

        for (int ch = 0; ch < numCh; ++ch)
        {
            float grDb = computeGainReductionDb (detDb, effectiveThr, params.ratio, kneeDb);
            grDb       = emotionLock.processGainReductionDb (grDb);
            grDb       = juce::jlimit (-40.0f, 0.0f, grDb);
            smoothEnvelope (grDb, ch, attCoeff, relCoeff);
            peakGR = juce::jmin (peakGR, envelope[static_cast<size_t> (ch)]);
        }

        // Linked gain (both channels same for tight stereo bus)
        const float sharedGain = juce::Decibels::decibelsToGain (
            (envelope[0] + (numCh > 1 ? envelope[1] : envelope[0])) * 0.5f);

        const float wetL = softSaturate (inL * sharedGain, satDrive) * makeupGain;
        const float wetR = softSaturate (inR * sharedGain, satDrive) * makeupGain;

        const float outL = juce::jmap (params.mix, inL * makeupGain, wetL);
        const float outR = juce::jmap (params.mix, inR * makeupGain, wetR);
        buffer.setSample (0, s, outL);
        if (numCh > 1) buffer.setSample (1, s, outR);
        peakOut = juce::jmax (peakOut, std::abs (outL), std::abs (outR));
    }

    gainReductionDb.store (peakGR, std::memory_order_relaxed);
    outputLevelDb.store   (juce::Decibels::gainToDecibels (peakOut + 1e-9f), std::memory_order_relaxed);
}

// ─────────────────────────────────────────────────────────────────────────────
// MODEL 6 — Modern Clean  (transparent digital, no saturation, perfect math)
// ─────────────────────────────────────────────────────────────────────────────

void AuraCompressorEngine::processModernClean (juce::AudioBuffer<float>& buffer)
{
    constexpr float kneeDb  = 6.0f;
    const float inputGain   = juce::Decibels::decibelsToGain (params.inputDb);
    const float makeupGain  = juce::Decibels::decibelsToGain (params.makeupDb);
    const float relMod      = emotionLock.getReleaseModifier();
    const float attCoeff    = makeCoeff (params.attackMs, sampleRate);
    const float relCoeff    = makeCoeff (params.releaseMs * relMod, sampleRate);
    const float effectiveThr = params.thresholdDb - params.amount * 12.0f;

    const int numSamples = buffer.getNumSamples();
    const int numCh      = juce::jmin (numChannels, buffer.getNumChannels());
    float peakGR = 0.0f, peakOut = 0.0f;

    const float rmsCoeff = makeCoeff (25.0f, sampleRate);

    for (int s = 0; s < numSamples; ++s)
    {
        float inL = buffer.getSample (0, s) * inputGain;
        float inR = numCh > 1 ? buffer.getSample (1, s) * inputGain : inL;

        const float scL = processHPF (inL, 0);
        const float scR = numCh > 1 ? processHPF (inR, 1) : scL;

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
            float grDb        = computeGainReductionDb (detDb, effectiveThr, params.ratio, kneeDb);
            grDb              = emotionLock.processGainReductionDb (grDb);
            grDb              = juce::jlimit (-40.0f, 0.0f, grDb);
            smoothEnvelope (grDb, ch, attCoeff, relCoeff);
            peakGR = juce::jmin (peakGR, envelope[static_cast<size_t> (ch)]);
        }

        const float gainL = juce::Decibels::decibelsToGain (envelope[0]);
        const float gainR = numCh > 1 ? juce::Decibels::decibelsToGain (envelope[1]) : gainL;

        // No saturation — perfect linear compression
        const float wetL = inL * gainL * makeupGain;
        const float wetR = inR * gainR * makeupGain;

        const float outL = juce::jmap (params.mix, inL * makeupGain, wetL);
        const float outR = juce::jmap (params.mix, inR * makeupGain, wetR);
        buffer.setSample (0, s, outL);
        if (numCh > 1) buffer.setSample (1, s, outR);
        peakOut = juce::jmax (peakOut, std::abs (outL), std::abs (outR));
    }

    gainReductionDb.store (peakGR, std::memory_order_relaxed);
    outputLevelDb.store   (juce::Decibels::gainToDecibels (peakOut + 1e-9f), std::memory_order_relaxed);
}
