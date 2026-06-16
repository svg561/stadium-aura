#include <JuceHeader.h>
#include "dsp/AuraProcessor.h"
#include "dsp/AuraMicCharacterEngine.h"

namespace
{
bool allFinite (const juce::AudioBuffer<float>& buffer)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            if (! std::isfinite (buffer.getSample (channel, sample)))
                return false;
    return true;
}

void fillSine (juce::AudioBuffer<float>& buffer, float amplitude)
{
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto value = amplitude * std::sin (juce::MathConstants<float>::twoPi * 997.0f
                                                * static_cast<float> (sample) / 48000.0f);
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.setSample (channel, sample, value);
    }
}
}

int main()
{
    AuraProcessor processor;
    processor.prepare (48000.0, 512, 2);
    AuraParameters parameters;
    parameters.aura = 1.0f;
    parameters.tubeDrive = 1.0f;
    parameters.saturation = 1.0f;
    parameters.transformer = 1.0f;
    parameters.compressorAmount = 0.7f;
    parameters.quality = 2;
    parameters.limiterCeilingDb = -0.3f;
    parameters.micCorrectionAmount = 0.7f;
    parameters.micTargetAmount = 0.7f;
    parameters.badFrequencyTamer = 0.6f;
    parameters.compressorEnabled = true;
    parameters.compressorThresholdDb = -24.0f;
    parameters.compressorRatio = 6.0f;
    parameters.eqEnabled = true;
    parameters.eqHpfHz = 45.0f;
    parameters.eqLowShelfHz = 120.0f;
    parameters.eqLowShelfGainDb = 2.5f;
    parameters.eqBellHz = 900.0f;
    parameters.eqBellGainDb = -3.0f;
    parameters.eqBellQ = 1.2f;
    parameters.eqHighShelfHz = 7500.0f;
    parameters.eqHighShelfGainDb = 1.5f;
    parameters.eqLpfHz = 17500.0f;

    juce::AudioBuffer<float> stereo (2, 512);
    for (int block = 0; block < 20; ++block)
    {
        fillSine (stereo, 2.0f);
        processor.process (stereo, parameters);
        if (! allFinite (stereo)) return 1;
    }

    const auto ceiling = juce::Decibels::decibelsToGain (-0.3f) + 1.0e-4f;
    if (stereo.getMagnitude (0, 0, stereo.getNumSamples()) > ceiling) return 2;
    if (stereo.getMagnitude (1, 0, stereo.getNumSamples()) > ceiling) return 3;

    parameters.micSectionEnabled = false;
    parameters.preampSectionEnabled = false;
    parameters.harmonicsSectionEnabled = false;
    parameters.sumSectionEnabled = false;
    parameters.masterSectionEnabled = false;
    fillSine (stereo, 0.25f);
    processor.process (stereo, parameters);
    if (! allFinite (stereo)) return 30;
    if (stereo.getMagnitude (0, 0, stereo.getNumSamples()) <= 0.0f) return 31;
    parameters.micSectionEnabled = true;
    parameters.preampSectionEnabled = true;
    parameters.harmonicsSectionEnabled = true;
    parameters.sumSectionEnabled = true;
    parameters.masterSectionEnabled = true;

    AuraMicCharacterEngine micEngine;
    for (int profile = 0; profile < kNumAuraMicProfiles; ++profile)
    {
        micEngine.applyProfileDefaults (parameters.micCharacterParams, profile);
        parameters.micCharacterParams.enabled = true;
        parameters.micCharacterParams.bypass = false;
        for (int preampMode = 0; preampMode < 3; ++preampMode)
        {
            parameters.preampMode = static_cast<PreampArchitecture::Mode> (preampMode);
            parameters.tubeSwap = preampMode;
            fillSine (stereo, 0.75f);
            processor.process (stereo, parameters);
            if (! allFinite (stereo)) return 6 + profile * 3 + preampMode;
        }
    }

    parameters.micCharacterParams.bypass = true;
    fillSine (stereo, 0.5f);
    processor.process (stereo, parameters);
    if (! allFinite (stereo)) return 90;
    parameters.micCharacterParams.bypass = false;

    for (int tubeType = 0; tubeType < 6; ++tubeType)
        for (int consoleMode = 0; consoleMode < 4; ++consoleMode)
        {
            parameters.tubeType = static_cast<TubeSaturation::Type> (tubeType);
            parameters.consoleMode = static_cast<SummingGlue::Mode> (consoleMode);
            parameters.consoleDensity = 0.8f;
            fillSine (stereo, 0.75f);
            processor.process (stereo, parameters);
            if (! allFinite (stereo)) return 60 + tubeType * 4 + consoleMode;
        }

    AuraProcessor monoProcessor;
    monoProcessor.prepare (48000.0, 512, 1);
    juce::AudioBuffer<float> mono (1, 512);
    fillSine (mono, 0.8f);
    parameters.monoCheck = true;
    monoProcessor.process (mono, parameters);
    if (! allFinite (mono)) return 4;

    parameters.bypassed = true;
    for (int block = 0; block < 4; ++block)
    {
        fillSine (mono, 0.5f);
        monoProcessor.process (mono, parameters);
        if (! allFinite (mono)) return 5;
    }

    return 0;
}
