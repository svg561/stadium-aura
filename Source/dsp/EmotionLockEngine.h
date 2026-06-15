#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>

struct EmotionLockParams
{
    bool  enabled          = false;
    float amount           = 0.5f;
    float breathProtect    = 0.35f;
    float presenceProtect  = 0.5f;
    float airProtect       = 0.35f;
    float harshTame        = 0.3f;
    float pocketLock       = 0.4f;
    float intensity        = 0.5f;
};

// Header-only emotion-lock engine — inline, no dynamic allocation.
class EmotionLockEngine
{
public:
    void prepare (double sr) noexcept
    {
        sampleRate = sr;
        updateBPF (breathBPF,   1800.0f, 4.0f);
        updateBPF (presenceBPF, 3000.0f, 2.0f);
        updateBPF (harshBPF,    5500.0f, 2.5f);
        updateBPF (airBPF,     12000.0f, 3.0f);
        reset();
    }

    void reset() noexcept
    {
        breathZ   = { 0.0f, 0.0f };
        presenceZ = { 0.0f, 0.0f };
        harshZ    = { 0.0f, 0.0f };
        airZ      = { 0.0f, 0.0f };
        breathEnv = presenceEnv = harshEnv = airEnv = 0.0f;
        releaseModifier = 1.0f;
        satModifier     = 1.0f;
    }

    void setParams (const EmotionLockParams& p) noexcept { params = p; }

    // Call once per sidechain sample (mono mix of detector signal).
    void analyzeSample (float sample) noexcept
    {
        if (!params.enabled) return;

        const float breath   = processBPF (sample, breathBPF,   breathZ[0],   breathZ[1]);
        const float presence = processBPF (sample, presenceBPF, presenceZ[0], presenceZ[1]);
        const float harsh    = processBPF (sample, harshBPF,    harshZ[0],    harshZ[1]);
        const float air      = processBPF (sample, airBPF,      airZ[0],      airZ[1]);

        auto smooth = [] (float& env, float v)
        {
            const float fast = 0.05f, slow = 0.002f;
            env += (v - env) * (v > env ? fast : slow);
        };
        smooth (breathEnv,   std::abs (breath));
        smooth (presenceEnv, std::abs (presence));
        smooth (harshEnv,    std::abs (harsh));
        smooth (airEnv,      std::abs (air));

        const float amt = params.amount * params.intensity;

        // Release modifier: more breath / presence → longer release
        const float targetRel = 1.0f + juce::jlimit (0.0f, 2.0f,
            breathEnv * params.breathProtect * amt * 4.0f
            + presenceEnv * params.presenceProtect * amt * 3.0f);
        releaseModifier += (targetRel - releaseModifier) * 0.001f;

        // Saturation modifier: more air → cleaner (less saturation)
        const float targetSat = 1.0f - juce::jlimit (0.0f, 0.65f, airEnv * params.airProtect * amt * 2.0f);
        satModifier += (targetSat - satModifier) * 0.001f;
    }

    // Modify gain-reduction dB based on emotional content (negative = compression).
    float processGainReductionDb (float grDb) const noexcept
    {
        if (!params.enabled) return grDb;
        const float amt = params.amount * params.intensity;
        // Presence protection: reduce compression for presence frequencies
        const float presGain = presenceEnv * params.presenceProtect * amt * 6.0f;
        // Harsh tame: slightly more compression in harsh region
        const float harshBias = harshEnv * params.harshTame * amt * 3.0f;
        // Pocket lock: subtle consistency bias
        const float pocket = (grDb < -3.0f ? 1.0f : 0.0f) * params.pocketLock * amt * 0.1f;
        const float modifier = 1.0f - juce::jlimit (-0.4f, 0.4f, presGain - harshBias + pocket);
        return grDb * modifier;
    }

    float getReleaseModifier()    const noexcept { return params.enabled ? juce::jlimit (0.5f, 3.0f, releaseModifier) : 1.0f; }
    float getSaturationModifier() const noexcept { return params.enabled ? juce::jlimit (0.1f, 1.0f, satModifier)     : 1.0f; }

private:
    struct BPFCoeffs { float b0 = 0.f, b2 = 0.f, a1 = 0.f, a2 = 0.f; };

    void updateBPF (BPFCoeffs& c, float freq, float Q) noexcept
    {
        const float sr = static_cast<float> (juce::jmax (1.0, sampleRate));
        const float omega = juce::MathConstants<float>::twoPi * juce::jlimit (20.0f, 20000.0f, freq) / sr;
        const float sinw  = std::sin (omega);
        const float cosw  = std::cos (omega);
        const float alpha = sinw / (2.0f * juce::jmax (0.1f, Q));
        const float a0    = 1.0f + alpha;
        c.b0 = alpha / a0;
        c.b2 = -alpha / a0;
        c.a1 = -2.0f * cosw / a0;
        c.a2 = (1.0f - alpha) / a0;
    }

    // Transposed-II biquad (b1 = 0 for band-pass)
    static float processBPF (float x, const BPFCoeffs& c, float& z1, float& z2) noexcept
    {
        const float y = c.b0 * x + z1;
        z1 = -c.a1 * y + z2;
        z2 =  c.b2 * x - c.a2 * y;
        return y;
    }

    double sampleRate = 44100.0;
    EmotionLockParams params;

    BPFCoeffs breathBPF, presenceBPF, harshBPF, airBPF;
    std::array<float, 2> breathZ   { 0.0f, 0.0f };
    std::array<float, 2> presenceZ { 0.0f, 0.0f };
    std::array<float, 2> harshZ    { 0.0f, 0.0f };
    std::array<float, 2> airZ      { 0.0f, 0.0f };

    float breathEnv   = 0.0f;
    float presenceEnv = 0.0f;
    float harshEnv    = 0.0f;
    float airEnv      = 0.0f;
    float releaseModifier = 1.0f;
    float satModifier     = 1.0f;
};
