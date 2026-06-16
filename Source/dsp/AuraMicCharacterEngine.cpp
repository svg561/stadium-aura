#include "AuraMicCharacterEngine.h"

const AuraMicProfile AuraMicCharacterEngine::auraMicProfiles[kNumAuraMicProfiles] =
{
    { "aura_vintage_87",   "Aura Vintage 87",   0.0f, 75.0f, 0.5f,  1.0f, -1.0f,  1.5f,  1.0f,  0.25f, 0.20f, 0.12f, 0.0f },
    { "aura_silk_tube",    "Aura Silk Tube",    0.0f, 80.0f, 0.0f,  0.5f, -0.8f,  1.0f,  3.0f,  0.35f, 0.30f, 0.22f, -0.5f },
    { "aura_golden_251",   "Aura Golden 251",   0.0f, 70.0f, 0.5f,  1.2f, -0.7f,  2.0f,  2.2f,  0.25f, 0.25f, 0.18f, -0.3f },
    { "aura_crystal_12",   "Aura Crystal 12",   0.0f, 90.0f, -0.5f, -0.5f, -1.2f,  2.0f,  3.8f,  0.40f, 0.40f, 0.10f, -0.8f },
    { "aura_broadcast_7",  "Aura Broadcast 7",  0.0f, 55.0f, 1.5f,  2.0f, -0.5f, -0.8f, -1.5f, 0.15f, 0.10f, 0.16f, 0.5f },
    { "aura_modern_pop",   "Aura Modern Pop",   0.0f, 85.0f, 0.0f,  0.2f, -1.5f,  2.5f,  3.0f,  0.45f, 0.45f, 0.14f, -0.6f },
    { "aura_warm_rap",     "Aura Warm Rap",     0.0f, 70.0f, 1.0f,  1.8f, -1.0f,  1.2f,  1.2f,  0.35f, 0.30f, 0.20f, 0.0f },
    { "aura_female_air",   "Aura Female Air",   0.0f, 95.0f, -0.3f, 0.0f, -1.0f,  1.5f,  3.5f,  0.50f, 0.50f, 0.12f, -0.8f },
    { "aura_male_body",    "Aura Male Body",    0.0f, 65.0f, 1.2f,  2.0f, -1.2f,  0.8f,  1.0f,  0.35f, 0.25f, 0.16f, 0.0f },
    { "aura_clean_capture","Aura Clean Capture",0.0f, 80.0f, 0.0f,  0.0f, -0.5f,  0.5f,  0.5f,  0.15f, 0.15f, 0.04f, 0.0f }
};

void AuraMicCharacterEngine::prepare (double sampleRate, int maximumBlockSize, int channels)
{
    juce::ignoreUnused (maximumBlockSize);
    currentSampleRate = sampleRate;
    states.assign (static_cast<size_t> (juce::jmax (1, channels)), {});
    coeffSmoothing = 1.0f - std::exp (-1.0f / (0.035f * static_cast<float> (sampleRate)));
    inputTrimGain.reset (sampleRate, 0.020);
    outputTrimGain.reset (sampleRate, 0.020);
    colorMix.reset (sampleRate, 0.030);
    deHarshAmt.reset (sampleRate, 0.040);
    sibilanceAmt.reset (sampleRate, 0.040);
    reset();
}

void AuraMicCharacterEngine::reset() noexcept
{
    for (auto& state : states)
        state = ChannelState {};
    smoothBodyDb = 0.0f;
    smoothMudDb = 0.0f;
    smoothPresDb = 0.0f;
    smoothAirDb = 0.0f;
    smoothLowCutHz = 80.0f;
}

void AuraMicCharacterEngine::applyProfileDefaults (MicCharacterParams& params, int profileIndex) noexcept
{
    profileIndex = juce::jlimit (0, kNumAuraMicProfiles - 1, profileIndex);
    const auto& profile = auraMicProfiles[profileIndex];
    params.profileIndex = profileIndex;
    params.inputTrimDb = profile.inputTrimDb;
    params.proximity = 0.0f;
    params.bodyDb = profile.bodyDb;
    params.mudDb = profile.mudDb;
    params.presenceDb = profile.presenceDb;
    params.airDb = profile.airDb;
    params.deHarsh = profile.deHarshAmount;
    params.sibilanceGuard = profile.sibilanceAmount;
    params.colorAmount = profile.colorAmount;
    params.outputTrimDb = profile.outputTrimDb;
    params.lowCutHz = profile.lowCutHz;
}

float AuraMicCharacterEngine::processBiquad (float input, float& z1, float& z2, const BiquadCoeffs& c) noexcept
{
    const auto output = c.b0 * input + z1;
    z1 = c.b1 * input - c.a1 * output + z2;
    z2 = c.b2 * input - c.a2 * output;
    return output;
}

AuraMicCharacterEngine::BiquadCoeffs AuraMicCharacterEngine::makeHighPass (double sampleRate, float frequency) noexcept
{
    const auto omega = juce::MathConstants<float>::twoPi * juce::jlimit (20.0f, 20000.0f, frequency)
                     / static_cast<float> (sampleRate);
    const auto alpha = std::sin (omega) / 1.41421356f;
    const auto cosw = std::cos (omega);
    const auto a0 = 1.0f + alpha;
    BiquadCoeffs c;
    c.b0 = (1.0f + cosw) * 0.5f / a0;
    c.b1 = -(1.0f + cosw) / a0;
    c.b2 = c.b0;
    c.a1 = -2.0f * cosw / a0;
    c.a2 = (1.0f - alpha) / a0;
    return c;
}

AuraMicCharacterEngine::BiquadCoeffs AuraMicCharacterEngine::makePeak (double sampleRate, float frequency, float gainDb, float q) noexcept
{
    const auto omega = juce::MathConstants<float>::twoPi * juce::jlimit (20.0f, 20000.0f, frequency)
                     / static_cast<float> (sampleRate);
    const auto alpha = std::sin (omega) / (2.0f * juce::jlimit (0.2f, 8.0f, q));
    const auto a = std::pow (10.0f, gainDb / 40.0f);
    const auto cosw = std::cos (omega);
    const auto a0 = 1.0f + alpha / a;
    BiquadCoeffs c;
    c.b0 = (1.0f + alpha * a) / a0;
    c.b1 = (-2.0f * cosw) / a0;
    c.b2 = (1.0f - alpha * a) / a0;
    c.a1 = (-2.0f * cosw) / a0;
    c.a2 = (1.0f - alpha / a) / a0;
    return c;
}

AuraMicCharacterEngine::BiquadCoeffs AuraMicCharacterEngine::makeHighShelf (double sampleRate, float frequency, float gainDb) noexcept
{
    const auto omega = juce::MathConstants<float>::twoPi * juce::jlimit (20.0f, 20000.0f, frequency)
                     / static_cast<float> (sampleRate);
    const auto a = std::pow (10.0f, gainDb / 40.0f);
    const auto cosw = std::cos (omega);
    const auto beta = std::sqrt (a) / 0.70710678f;
    const auto sinw = std::sin (omega);
  BiquadCoeffs c;
    const auto b0 = a * ((a + 1.0f) + (a - 1.0f) * cosw + beta * sinw);
    const auto b1 = -2.0f * a * ((a - 1.0f) + (a + 1.0f) * cosw);
    const auto b2 = a * ((a + 1.0f) + (a - 1.0f) * cosw - beta * sinw);
    const auto a0 = (a + 1.0f) - (a - 1.0f) * cosw + beta * sinw;
    const auto a1 = -2.0f * ((a - 1.0f) - (a + 1.0f) * cosw);
    const auto a2 = (a + 1.0f) - (a - 1.0f) * cosw - beta * sinw;
    c.b0 = b0 / a0;
    c.b1 = b1 / a0;
    c.b2 = b2 / a0;
    c.a1 = a1 / a0;
    c.a2 = a2 / a0;
    return c;
}

void AuraMicCharacterEngine::updateCoefficients (float lowCutHz, float bodyDb, float mudDb,
                                                 float presenceDb, float airDb) noexcept
{
    auto smooth = [this] (float& current, float target)
    {
        current += (target - current) * coeffSmoothing;
    };

    smooth (smoothLowCutHz, lowCutHz);
    smooth (smoothBodyDb, bodyDb);
    smooth (smoothMudDb, mudDb);
    smooth (smoothPresDb, presenceDb);
    smooth (smoothAirDb, airDb);

    hpfCoeffs = makeHighPass (currentSampleRate, smoothLowCutHz);
    bodyCoeffs = makePeak (currentSampleRate, 200.0f, smoothBodyDb, 0.85f);
    mudCoeffs = makePeak (currentSampleRate, 350.0f, smoothMudDb, 1.1f);
    presCoeffs = makePeak (currentSampleRate, 3500.0f, smoothPresDb, 1.0f);
    airCoeffs = makeHighShelf (currentSampleRate, 10000.0f, smoothAirDb);
}

void AuraMicCharacterEngine::process (juce::AudioBuffer<float>& buffer, const MicCharacterParams& params) noexcept
{
    if (! params.enabled || params.bypass)
        return;

    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = juce::jmin (buffer.getNumChannels(), static_cast<int> (states.size()));
    if (numSamples <= 0 || numChannels <= 0)
        return;

    inputTrimGain.setTargetValue (juce::Decibels::decibelsToGain (params.inputTrimDb));
    outputTrimGain.setTargetValue (juce::Decibels::decibelsToGain (params.outputTrimDb));
    colorMix.setTargetValue (juce::jlimit (0.0f, 1.0f, params.colorAmount));
    deHarshAmt.setTargetValue (juce::jlimit (0.0f, 1.0f, params.deHarsh));
    sibilanceAmt.setTargetValue (juce::jlimit (0.0f, 1.0f, params.sibilanceGuard));

    const auto harshCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 4500.0f
                                            / static_cast<float> (currentSampleRate));
    const auto sibCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 7500.0f
                                          / static_cast<float> (currentSampleRate));

    for (int sample = 0; sample < numSamples; ++sample)
    {
        updateCoefficients (params.lowCutHz, params.bodyDb, params.mudDb, params.presenceDb, params.airDb);
        const auto inGain = inputTrimGain.getNextValue();
        const auto outGain = outputTrimGain.getNextValue();
        const auto color = colorMix.getNextValue();
        const auto deHarsh = deHarshAmt.getNextValue();
        const auto sibilance = sibilanceAmt.getNextValue();

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto& state = states[static_cast<size_t> (channel)];
            auto x = buffer.getSample (channel, sample) * inGain;

            x = processBiquad (x, state.hpfZ1, state.hpfZ2, hpfCoeffs);
            x = processBiquad (x, state.bodyZ1, state.bodyZ2, bodyCoeffs);
            x = processBiquad (x, state.mudZ1, state.mudZ2, mudCoeffs);
            x = processBiquad (x, state.presZ1, state.presZ2, presCoeffs);
            x = processBiquad (x, state.airZ1, state.airZ2, airCoeffs);

            state.harshLow += harshCoeff * (x - state.harshLow);
            state.harshBand = x - state.harshLow;
            const auto harshEnergy = std::abs (state.harshBand);
            const auto harshAttack = harshEnergy > state.harshEnv ? 0.12f : 0.004f;
            state.harshEnv += (harshEnergy - state.harshEnv) * harshAttack;
            const auto harshCut = juce::jlimit (0.0f, 0.55f, (state.harshEnv - 0.02f) * deHarsh * 5.0f);
            x -= state.harshBand * harshCut;

            state.sibLow += sibCoeff * (x - state.sibLow);
            state.sibBand = x - state.sibLow;
            const auto sibEnergy = std::abs (state.sibBand);
            const auto sibAttack = sibEnergy > state.sibEnv ? 0.15f : 0.005f;
            state.sibEnv += (sibEnergy - state.sibEnv) * sibAttack;
            const auto sibCut = juce::jlimit (0.0f, 0.48f, (state.sibEnv - 0.015f) * sibilance * 4.5f);
            x -= state.sibBand * sibCut;

            if (color > 0.001f)
            {
                const auto driven = std::tanh (x * (1.0f + color * 1.4f));
                x = juce::jmap (color, x, driven);
            }

            buffer.setSample (channel, sample, x * outGain);
        }
    }
}
