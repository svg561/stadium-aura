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
    micCorrection.reset (sampleRate, 0.050); micTarget.reset (sampleRate, 0.050);
    badFrequency.reset (sampleRate, 0.050); airProtect.reset (sampleRate, 0.050); bodyProtect.reset (sampleRate, 0.050);
    preampDrive.reset (sampleRate, 0.035); tubeOutput.reset (sampleRate, 0.020); consoleDensity.reset (sampleRate, 0.040);
    width.reset (sampleRate, 0.050); compressorAmount.reset (sampleRate, 0.020);
    compThreshold.reset (sampleRate, 0.020); compRatio.reset (sampleRate, 0.030);
    compAttack.reset (sampleRate, 0.040); compRelease.reset (sampleRate, 0.050);
    compMakeup.reset (sampleRate, 0.020); compBleed.reset (sampleRate, 0.020); compScHpf.reset (sampleRate, 0.050);
    makeupGain.reset (sampleRate, 0.020); ceiling.reset (sampleRate, 0.020);
    bypassFade.reset (sampleRate, 0.080);
    dimGain.reset (sampleRate, 0.020);
    monoAmount.reset (sampleRate, 0.050);
    micEnable.reset (sampleRate, 0.020);
    preampEnable.reset (sampleRate, 0.020);
    harmonicsEnable.reset (sampleRate, 0.020);
    sumEnable.reset (sampleRate, 0.020);
    masterEnable.reset (sampleRate, 0.030);

    inputGain.setCurrentAndTargetValue (1.0f); outputGain.setCurrentAndTargetValue (1.0f);
    mix.setCurrentAndTargetValue (1.0f); aura.setCurrentAndTargetValue (0.50f);
    tubeDrive.setCurrentAndTargetValue (0.25f); saturation.setCurrentAndTargetValue (0.2f);
    harmonicBias.setCurrentAndTargetValue (0.0f); transformerAmount.setCurrentAndTargetValue (0.20f);
    summing.setCurrentAndTargetValue (0.2f); glue.setCurrentAndTargetValue (0.2f);
    tone.setCurrentAndTargetValue (0.0f);
    micCorrection.setCurrentAndTargetValue (0.35f); micTarget.setCurrentAndTargetValue (0.50f);
    badFrequency.setCurrentAndTargetValue (0.35f); airProtect.setCurrentAndTargetValue (0.50f); bodyProtect.setCurrentAndTargetValue (0.50f);
    preampDrive.setCurrentAndTargetValue (0.25f); tubeOutput.setCurrentAndTargetValue (1.0f); consoleDensity.setCurrentAndTargetValue (0.20f);
    width.setCurrentAndTargetValue (1.0f); compressorAmount.setCurrentAndTargetValue (0.0f);
    compThreshold.setCurrentAndTargetValue (-18.0f); compRatio.setCurrentAndTargetValue (4.0f);
    compAttack.setCurrentAndTargetValue (20.0f); compRelease.setCurrentAndTargetValue (400.0f);
    compMakeup.setCurrentAndTargetValue (1.0f); compBleed.setCurrentAndTargetValue (0.0f); compScHpf.setCurrentAndTargetValue (80.0f);
    makeupGain.setCurrentAndTargetValue (1.0f);
    ceiling.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (-1.0f));
    bypassFade.setCurrentAndTargetValue (1.0f);
    dimGain.setCurrentAndTargetValue (1.0f);
    monoAmount.setCurrentAndTargetValue (0.0f);
    micEnable.setCurrentAndTargetValue (1.0f);
    preampEnable.setCurrentAndTargetValue (1.0f);
    harmonicsEnable.setCurrentAndTargetValue (1.0f);
    sumEnable.setCurrentAndTargetValue (1.0f);
    masterEnable.setCurrentAndTargetValue (1.0f);
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
    micCorrection.setTargetValue (p.micCorrectionAmount); micTarget.setTargetValue (p.micTargetAmount);
    badFrequency.setTargetValue (p.badFrequencyTamer); airProtect.setTargetValue (p.airProtection); bodyProtect.setTargetValue (p.bodyProtection);
    preampDrive.setTargetValue (p.preampDrive); tubeOutput.setTargetValue (juce::Decibels::decibelsToGain (p.tubeOutputDb));
    consoleDensity.setTargetValue (p.consoleDensity);
    width.setTargetValue (p.width); compressorAmount.setTargetValue (p.compressorAmount);
    compThreshold.setTargetValue (p.compressorThresholdDb); compRatio.setTargetValue (p.compressorRatio);
    compAttack.setTargetValue (p.compressorAttackMs); compRelease.setTargetValue (p.compressorReleaseMs);
    compMakeup.setTargetValue (juce::Decibels::decibelsToGain (p.compressorMakeupDb));
    compBleed.setTargetValue (p.compressorBleed); compScHpf.setTargetValue (p.compressorSidechainHpfHz);
    makeupGain.setTargetValue (juce::Decibels::decibelsToGain (p.makeupGainDb));
    ceiling.setTargetValue (juce::Decibels::decibelsToGain (p.limiterCeilingDb));
    bypassFade.setTargetValue (p.bypassed ? 0.0f : 1.0f);
    dimGain.setTargetValue (p.dimmed ? juce::Decibels::decibelsToGain (-18.0f) : 1.0f);
    monoAmount.setTargetValue (p.monoCheck ? 1.0f : 0.0f);
    micEnable.setTargetValue (p.micSectionEnabled ? 1.0f : 0.0f);
    preampEnable.setTargetValue (p.preampSectionEnabled ? 1.0f : 0.0f);
    harmonicsEnable.setTargetValue (p.harmonicsSectionEnabled ? 1.0f : 0.0f);
    sumEnable.setTargetValue (p.sumSectionEnabled ? 1.0f : 0.0f);
    masterEnable.setTargetValue (p.masterSectionEnabled ? 1.0f : 0.0f);

    float blockTubePeak = 0.0f;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto inGain = inputGain.getNextValue();
        const auto outGain = outputGain.getNextValue();
        const auto wetMix = mix.getNextValue();
        const auto masterBlend = masterEnable.getNextValue();
        const auto auraValue = aura.getNextValue() * masterBlend;
        const auto macro = calculateMacro (auraValue);
        const auto micBlend = micEnable.getNextValue();
        const auto preampBlend = preampEnable.getNextValue();
        const auto harmonicsBlend = harmonicsEnable.getNextValue();
        const auto sumBlend = sumEnable.getNextValue();
        const auto tubeDriveValue = tubeDrive.getNextValue();
        const auto driveDb = tubeDriveValue * 30.0f + macro.driveDb;
        const auto sat = juce::jlimit (0.0f, 1.0f, (saturation.getNextValue() + macro.saturation + tubeDriveValue * 0.68f) * harmonicsBlend);
        const auto bias = harmonicBias.getNextValue();
        const auto transformerMix = juce::jlimit (0.0f, 1.0f, (transformerAmount.getNextValue() + macro.transformer) * harmonicsBlend);
        const auto summingMix = summing.getNextValue() * sumBlend;
        const auto glueMix = juce::jlimit (0.0f, 1.0f, (glue.getNextValue() + macro.glue) * sumBlend);
        const auto compAmount = juce::jlimit (0.0f, 1.0f, compressorAmount.getNextValue() + macro.compression);
        const auto correctionValue = micCorrection.getNextValue();
        const auto targetValue = micTarget.getNextValue();
        const auto badFrequencyValue = badFrequency.getNextValue();
        const auto airProtectionValue = airProtect.getNextValue();
        const auto bodyProtectionValue = bodyProtect.getNextValue();
        const auto preampDriveValue = preampDrive.getNextValue();
        const auto tubeOutputValue = tubeOutput.getNextValue();
        const auto consoleDensityValue = consoleDensity.getNextValue();
        const auto thresholdValue = compThreshold.getNextValue() - compAmount * 8.0f;
        const auto ratioValue = compRatio.getNextValue();
        auto attackValue = compAttack.getNextValue();
        auto releaseValue = compRelease.getNextValue();
        const auto compMakeupValue = compMakeup.getNextValue();
        const auto compBleedValue = compBleed.getNextValue();
        const auto sidechainHpfValue = compScHpf.getNextValue();
        const auto toneValue = tone.getNextValue();
        const auto makeup = makeupGain.getNextValue() * juce::Decibels::decibelsToGain (macro.autoGainDb);
        const auto widthValue = juce::jlimit (0.0f, 1.5f, width.getNextValue() * macro.width);
        const auto monoBlend = monoAmount.getNextValue();
        const auto dim = dimGain.getNextValue();

        float left = buffer.getSample (0, sample) * inGain * headroomPad;
        float right = channels > 1 ? buffer.getSample (1, sample) * inGain * headroomPad : left;
        const auto preMicLeft = left;
        const auto preMicRight = right;
        left = micCharacter.processRelativeSample (0, left, p.sourceMicMode, p.targetMicMode,
                                                   correctionValue * micBlend, targetValue * micBlend,
                                                   badFrequencyValue * micBlend,
                                                   airProtectionValue, bodyProtectionValue, p.hardwareSafeMode);
        right = micCharacter.processRelativeSample (juce::jmin (1, channels - 1), right,
                                                    p.sourceMicMode, p.targetMicMode, correctionValue * micBlend,
                                                    targetValue * micBlend, badFrequencyValue * micBlend,
                                                    airProtectionValue, bodyProtectionValue, p.hardwareSafeMode);
        left = micCharacter.processSample (0, left, p.micCharacter);
        right = micCharacter.processSample (juce::jmin (1, channels - 1), right, p.micCharacter);
        left = juce::jmap (micBlend, preMicLeft, left);
        right = juce::jmap (micBlend, preMicRight, right);

        const auto prePreampLeft = left;
        const auto prePreampRight = right;
        left = preamp.processSample (0, left, p.preampMode, preampDriveValue * preampBlend);
        right = preamp.processSample (juce::jmin (1, channels - 1), right, p.preampMode, preampDriveValue * preampBlend);
        left = juce::jmap (preampBlend, prePreampLeft, left);
        right = juce::jmap (preampBlend, prePreampRight, right);

        const auto detector = juce::jmax (std::abs (left), std::abs (right));
        if (p.compressorTimingMode != 0)
        {
            float fixedAttack = 20.0f, fixedRelease = 400.0f;
            if (p.compressorMode == CompressorSection::Mode::fast76) { fixedAttack = 0.25f; fixedRelease = 120.0f; }
            if (p.compressorMode == CompressorSection::Mode::kid670) { fixedAttack = 10.0f; fixedRelease = 650.0f; }
            const auto fixedBlend = p.compressorTimingMode == 1 ? 1.0f : 0.5f;
            attackValue = juce::jmap (fixedBlend, attackValue, fixedAttack);
            releaseValue = juce::jmap (fixedBlend, releaseValue, fixedRelease);
        }
        const auto compressorGain = p.compressorEnabled
            ? compressor.processDetectorDetailed (detector, p.compressorMode, thresholdValue,
                                                  ratioValue, attackValue, releaseValue, sidechainHpfValue)
            : 1.0f;
        left = juce::jmap (compBleedValue, left * compressorGain * compMakeupValue, left);
        right = juce::jmap (compBleedValue, right * compressorGain * compMakeupValue, right);
        const auto colourInputLeft = left;
        const auto colourInputRight = right;
        const auto tubeDriveOffset = p.tubeSwap == 0 ? 0.0f : (p.tubeSwap == 1 ? 1.5f : -1.0f);
        const auto tubeBiasOffset = p.tubeSwap == 0 ? 0.0f : (p.tubeSwap == 1 ? 0.10f : -0.08f);
        const auto preHarmonicsLeft = left;
        const auto preHarmonicsRight = right;

        if (p.quality >= 2)
        {
            const auto midLeft = (previousColourInput[0] + left) * 0.5f;
            const auto midRight = (previousColourInput[1] + right) * 0.5f;
            const auto highMidLeft = transformerHigh.processSample (0, TubeSaturation::processSample (midLeft, driveDb + tubeDriveOffset, sat, bias + tubeBiasOffset, p.tubeType), transformerMix);
            const auto highMidRight = transformerHigh.processSample (juce::jmin (1, channels - 1), TubeSaturation::processSample (midRight, driveDb + tubeDriveOffset, sat, bias + tubeBiasOffset, p.tubeType), transformerMix);
            const auto highLeft = transformerHigh.processSample (0, TubeSaturation::processSample (left, driveDb + tubeDriveOffset, sat, bias + tubeBiasOffset, p.tubeType), transformerMix);
            const auto highRight = transformerHigh.processSample (juce::jmin (1, channels - 1), TubeSaturation::processSample (right, driveDb + tubeDriveOffset, sat, bias + tubeBiasOffset, p.tubeType), transformerMix);
            left = (highMidLeft + highLeft) * 0.5f;
            right = (highMidRight + highRight) * 0.5f;
        }
        else
        {
            left = TubeSaturation::processSample (left, driveDb + tubeDriveOffset, sat, bias + tubeBiasOffset, p.tubeType);
            right = TubeSaturation::processSample (right, driveDb + tubeDriveOffset, sat, bias + tubeBiasOffset, p.tubeType);
            left = transformer.processSample (0, left, transformerMix);
            right = transformer.processSample (juce::jmin (1, channels - 1), right, transformerMix);
        }
        left *= tubeOutputValue;
        right *= tubeOutputValue;
        left = juce::jmap (harmonicsBlend, preHarmonicsLeft, left);
        right = juce::jmap (harmonicsBlend, preHarmonicsRight, right);
        blockTubePeak = juce::jmax (blockTubePeak, std::abs (left), std::abs (right));
        previousColourInput = { colourInputLeft, colourInputRight };
        const auto preSumLeft = left;
        const auto preSumRight = right;
        SummingGlue::processStereo (left, right, juce::jlimit (0.0f, 1.0f, summingMix + consoleDensityValue * sumBlend),
                                    p.trackCount, p.consoleMode);
        const auto busGain = busCompressor.processDetector (juce::jmax (std::abs (left), std::abs (right)),
                                                            CompressorSection::Mode::smoothOpto,
                                                            glueMix * 0.55f, 25.0f, 240.0f);
        left *= busGain;
        right *= busGain;
        left = juce::jmap (sumBlend, preSumLeft, left);
        right = juce::jmap (sumBlend, preSumRight, right);

        toneLow[0] += toneCoefficient * (left - toneLow[0]);
        toneLow[1] += toneCoefficient * (right - toneLow[1]);
        const auto tilt = toneValue * 0.18f;
        left += (left - toneLow[0]) * tilt - toneLow[0] * tilt;
        right += (right - toneLow[1]) * tilt - toneLow[1] * tilt;
        widthProcessor.processStereo (left, right, widthValue, monoBlend);

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
