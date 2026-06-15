#include "AuraProcessor.h"

namespace
{
constexpr float headroomPad = 0.25118864f; // -12 dB
constexpr float headroomRestore = 3.9810717f;
}

void AuraProcessor::prepare (double sampleRate, int maximumBlockSize, int channels)
{
    currentSampleRate = sampleRate;
    dryBuffer.setSize (channels, juce::jmax (maximumBlockSize, 65536), false, true, false);
    transformer.prepare (sampleRate, channels);
    transformerHigh.prepare (sampleRate * 2.0, channels);
    micCharacter.prepare (sampleRate, channels);
    preamp.prepare (sampleRate, channels);
    widthProcessor.prepare (sampleRate);
    compressor.prepare (sampleRate);
    busCompressor.prepare (sampleRate);
    limiter.prepare (sampleRate, maximumBlockSize, channels);
    bypassDelayBuffer.setSize (channels, limiter.getLatencySamples() + 1, false, true, false);
    initialiseSmoothers (sampleRate);
    reset();
}

void AuraProcessor::reset() noexcept
{
    dryBuffer.clear();
    bypassDelayBuffer.clear();
    transformer.reset();
    transformerHigh.reset();
    micCharacter.reset();
    preamp.reset();
    widthProcessor.reset();
    compressor.reset();
    busCompressor.reset();
    limiter.reset();
    bypassDelayPosition = 0;
    previousColourInput = { 0.0f, 0.0f };
    tubeActivity = 0.0f;
    toneLow = { 0.0f, 0.0f };
    toneCoefficient = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 1800.0f
                                       / static_cast<float> (currentSampleRate));
}

void AuraProcessor::initialiseSmoothers (double sampleRate)
{
    inputGain.reset (sampleRate, 0.020); outputGain.reset (sampleRate, 0.020);
    mix.reset (sampleRate, 0.030); aura.reset (sampleRate, 0.050);
    tubeDrive.reset (sampleRate, 0.030); saturation.reset (sampleRate, 0.030);
    harmonicBias.reset (sampleRate, 0.030); transformerAmount.reset (sampleRate, 0.040);
    summing.reset (sampleRate, 0.040); glue.reset (sampleRate, 0.040);
    tone.reset (sampleRate, 0.040);
    width.reset (sampleRate, 0.050); compressorAmount.reset (sampleRate, 0.020);
    makeupGain.reset (sampleRate, 0.020); ceiling.reset (sampleRate, 0.020);
    bypassFade.reset (sampleRate, 0.007);
    dimGain.reset (sampleRate, 0.020);

    inputGain.setCurrentAndTargetValue (1.0f); outputGain.setCurrentAndTargetValue (1.0f);
    mix.setCurrentAndTargetValue (1.0f); aura.setCurrentAndTargetValue (0.35f);
    tubeDrive.setCurrentAndTargetValue (0.2f); saturation.setCurrentAndTargetValue (0.2f);
    harmonicBias.setCurrentAndTargetValue (0.0f); transformerAmount.setCurrentAndTargetValue (0.15f);
    summing.setCurrentAndTargetValue (0.2f); glue.setCurrentAndTargetValue (0.2f);
    tone.setCurrentAndTargetValue (0.0f);
    width.setCurrentAndTargetValue (1.0f); compressorAmount.setCurrentAndTargetValue (0.0f);
    makeupGain.setCurrentAndTargetValue (1.0f);
    ceiling.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (-0.3f));
    bypassFade.setCurrentAndTargetValue (1.0f);
    dimGain.setCurrentAndTargetValue (1.0f);
}

AuraProcessor::MacroValues AuraProcessor::calculateMacro (float value) noexcept
{
    const auto curved = value * value * (3.0f - 2.0f * value);
    return { 10.0f * curved, 0.55f * curved, 0.65f * curved, 0.60f * curved,
             0.50f * curved, 1.0f + 0.20f * curved, -2.5f * curved };
}

void AuraProcessor::process (juce::AudioBuffer<float>& buffer, const AuraParameters& p) noexcept
{
    jassert (buffer.getNumSamples() <= dryBuffer.getNumSamples());
    const auto channels = juce::jmin (buffer.getNumChannels(), dryBuffer.getNumChannels());
    for (int channel = 0; channel < channels; ++channel)
        dryBuffer.copyFrom (channel, 0, buffer, channel, 0, buffer.getNumSamples());

    inputGain.setTargetValue (juce::Decibels::decibelsToGain (p.inputGainDb));
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (p.outputGainDb));
    mix.setTargetValue (p.mix); aura.setTargetValue (p.aura);
    tubeDrive.setTargetValue (p.tubeDrive); saturation.setTargetValue (p.saturation);
    harmonicBias.setTargetValue (p.harmonicBias); transformerAmount.setTargetValue (p.transformer);
    summing.setTargetValue (p.summing); glue.setTargetValue (p.glue);
    tone.setTargetValue (p.tone);
    width.setTargetValue (p.width); compressorAmount.setTargetValue (p.compressorAmount);
    makeupGain.setTargetValue (juce::Decibels::decibelsToGain (p.makeupGainDb));
    ceiling.setTargetValue (juce::Decibels::decibelsToGain (p.limiterCeilingDb));
    bypassFade.setTargetValue (p.bypassed ? 0.0f : 1.0f);
    dimGain.setTargetValue (p.dimmed ? juce::Decibels::decibelsToGain (-18.0f) : 1.0f);

    float blockTubePeak = 0.0f;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto inGain = inputGain.getNextValue();
        const auto outGain = outputGain.getNextValue();
        const auto wetMix = mix.getNextValue();
        const auto auraValue = aura.getNextValue();
        const auto macro = calculateMacro (auraValue);
        const auto driveDb = tubeDrive.getNextValue() * 24.0f + macro.driveDb;
        const auto sat = juce::jlimit (0.0f, 1.0f, saturation.getNextValue() + macro.saturation);
        const auto bias = harmonicBias.getNextValue();
        const auto transformerMix = juce::jlimit (0.0f, 1.0f, transformerAmount.getNextValue() + macro.transformer);
        const auto summingMix = summing.getNextValue();
        const auto glueMix = juce::jlimit (0.0f, 1.0f, glue.getNextValue() + macro.glue);
        const auto compAmount = juce::jlimit (0.0f, 1.0f, compressorAmount.getNextValue() + macro.compression);
        const auto toneValue = tone.getNextValue();
        const auto makeup = makeupGain.getNextValue() * juce::Decibels::decibelsToGain (macro.autoGainDb);
        const auto widthValue = juce::jlimit (0.0f, 1.5f, width.getNextValue() * macro.width);
        const auto dim = dimGain.getNextValue();

        float left = buffer.getSample (0, sample) * inGain * headroomPad;
        float right = channels > 1 ? buffer.getSample (1, sample) * inGain * headroomPad : left;
        left = micCharacter.processSample (0, left, p.micCharacter);
        right = micCharacter.processSample (juce::jmin (1, channels - 1), right, p.micCharacter);
        left = preamp.processSample (0, left, p.preampMode);
        right = preamp.processSample (juce::jmin (1, channels - 1), right, p.preampMode);

        const auto detector = juce::jmax (std::abs (left), std::abs (right));
        float attackMs = 20.0f, releaseMs = 400.0f;
        if (p.compressorMode == CompressorSection::Mode::fastFet) { attackMs = 1.0f; releaseMs = 80.0f; }
        if (p.compressorMode == CompressorSection::Mode::tubeLeveler) { attackMs = 10.0f; releaseMs = 160.0f; }
        const auto compressorGain = compressor.processDetector (detector, p.compressorMode,
                                                                compAmount, attackMs, releaseMs);
        left *= compressorGain;
        right *= compressorGain;
        const auto colourInputLeft = left;
        const auto colourInputRight = right;
        const auto tubeDriveOffset = p.tubeSwap == 0 ? 0.0f : (p.tubeSwap == 1 ? 1.5f : -1.0f);
        const auto tubeBiasOffset = p.tubeSwap == 0 ? 0.0f : (p.tubeSwap == 1 ? 0.10f : -0.08f);

        if (p.quality >= 2)
        {
            const auto midLeft = (previousColourInput[0] + left) * 0.5f;
            const auto midRight = (previousColourInput[1] + right) * 0.5f;
            const auto highMidLeft = transformerHigh.processSample (0, TubeSaturation::processSample (midLeft, driveDb + tubeDriveOffset, sat, bias + tubeBiasOffset), transformerMix);
            const auto highMidRight = transformerHigh.processSample (juce::jmin (1, channels - 1), TubeSaturation::processSample (midRight, driveDb + tubeDriveOffset, sat, bias + tubeBiasOffset), transformerMix);
            const auto highLeft = transformerHigh.processSample (0, TubeSaturation::processSample (left, driveDb + tubeDriveOffset, sat, bias + tubeBiasOffset), transformerMix);
            const auto highRight = transformerHigh.processSample (juce::jmin (1, channels - 1), TubeSaturation::processSample (right, driveDb + tubeDriveOffset, sat, bias + tubeBiasOffset), transformerMix);
            left = (highMidLeft + highLeft) * 0.5f;
            right = (highMidRight + highRight) * 0.5f;
        }
        else
        {
            left = TubeSaturation::processSample (left, driveDb + tubeDriveOffset, sat, bias + tubeBiasOffset);
            right = TubeSaturation::processSample (right, driveDb + tubeDriveOffset, sat, bias + tubeBiasOffset);
            left = transformer.processSample (0, left, transformerMix);
            right = transformer.processSample (juce::jmin (1, channels - 1), right, transformerMix);
        }
        blockTubePeak = juce::jmax (blockTubePeak, std::abs (left), std::abs (right));
        previousColourInput = { colourInputLeft, colourInputRight };
        SummingGlue::processStereo (left, right, summingMix, p.trackCount);
        const auto busGain = busCompressor.processDetector (juce::jmax (std::abs (left), std::abs (right)),
                                                            CompressorSection::Mode::smoothOpto,
                                                            glueMix * 0.55f, 25.0f, 240.0f);
        left *= busGain;
        right *= busGain;

        toneLow[0] += toneCoefficient * (left - toneLow[0]);
        toneLow[1] += toneCoefficient * (right - toneLow[1]);
        const auto tilt = toneValue * 0.18f;
        left += (left - toneLow[0]) * tilt - toneLow[0] * tilt;
        right += (right - toneLow[1]) * tilt - toneLow[1] * tilt;
        widthProcessor.processStereo (left, right, widthValue, p.monoCheck);

        const auto wetLeft = left * headroomRestore * makeup * outGain * dim;
        const auto wetRight = right * headroomRestore * makeup * outGain * dim;
        const auto dryLeft = dryBuffer.getSample (0, sample) * outGain * dim;
        const auto dryRight = channels > 1 ? dryBuffer.getSample (1, sample) * outGain * dim : dryLeft;
        buffer.setSample (0, sample, juce::jmap (wetMix, dryLeft, wetLeft));
        if (channels > 1)
            buffer.setSample (1, sample, juce::jmap (wetMix, dryRight, wetRight));
    }

    limiter.process (buffer, ceiling, p.limiterEnabled);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto wet = bypassFade.getNextValue();
        const auto delayReadPosition = (bypassDelayPosition + 1) % bypassDelayBuffer.getNumSamples();
        for (int channel = 0; channel < channels; ++channel)
        {
            const auto processed = buffer.getSample (channel, sample);
            bypassDelayBuffer.setSample (channel, bypassDelayPosition, dryBuffer.getSample (channel, sample));
            const auto dry = bypassDelayBuffer.getSample (channel, delayReadPosition);
            buffer.setSample (channel, sample, dry + (processed - dry) * wet);
        }
        bypassDelayPosition = delayReadPosition;
    }
    tubeActivity = juce::jmax (blockTubePeak, tubeActivity * 0.92f);
}
