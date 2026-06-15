#include "CompressorSection.h"

void CompressorSection::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    reset();
}

void CompressorSection::reset() noexcept
{
    envelope = 0.0f;
    gainReductionDb = 0.0f;
    currentRatio = 2.0f;
    currentKneeDb = 10.0f;
    detectorLow = 0.0f;
}

float CompressorSection::processDetector (float detectorInput, Mode mode, float amount,
                                          float attackMs, float releaseMs) noexcept
{
    return processDetectorDetailed (detectorInput, mode, juce::jmap (amount, -6.0f, -30.0f),
                                    modeRatio (mode), attackMs, releaseMs, 20.0f);
}

float CompressorSection::processDetectorDetailed (float detectorInput, Mode mode, float thresholdDb,
                                                   float ratio, float attackMs, float releaseMs,
                                                   float sidechainHpfHz) noexcept
{
    const auto hpfCoefficient = 1.0f - std::exp (-juce::MathConstants<float>::twoPi
                                                 * juce::jlimit (20.0f, 300.0f, sidechainHpfHz)
                                                 / static_cast<float> (currentSampleRate));
    detectorLow += hpfCoefficient * (detectorInput - detectorLow);
    const auto detector = std::abs (detectorInput - detectorLow);
    const auto timeMs = detector > envelope ? attackMs : releaseMs;
    const auto coefficient = std::exp (-1.0f / (0.001f * timeMs * static_cast<float> (currentSampleRate)));
    envelope = coefficient * envelope + (1.0f - coefficient) * detector;

    const auto levelDb = juce::Decibels::gainToDecibels (envelope, -100.0f);
    const auto modeSmoothing = 1.0f - std::exp (-1.0f / (0.025f * static_cast<float> (currentSampleRate)));
    const auto programRatio = mode == Mode::kid670
        ? juce::jlimit (1.2f, ratio, 1.5f + juce::jmax (0.0f, levelDb - thresholdDb) * 0.22f)
        : ratio;
    currentRatio += (juce::jmax (1.0f, programRatio) - currentRatio) * modeSmoothing;
    currentKneeDb += (modeKneeDb (mode) - currentKneeDb) * modeSmoothing;
    const auto kneeDb = currentKneeDb;
    const auto overDb = levelDb - thresholdDb;
    float compressedOverDb = 0.0f;

    if (overDb > kneeDb * 0.5f)
        compressedOverDb = overDb;
    else if (overDb > -kneeDb * 0.5f)
    {
        const auto kneePosition = overDb + kneeDb * 0.5f;
        compressedOverDb = kneePosition * kneePosition / (2.0f * kneeDb);
    }

    gainReductionDb = -compressedOverDb * (1.0f - 1.0f / currentRatio);
    return juce::Decibels::decibelsToGain (gainReductionDb);
}

float CompressorSection::modeRatio (Mode mode) noexcept
{
    switch (mode)
    {
        case Mode::fast76: return 4.0f;
        case Mode::smoothOpto: return 2.0f;
        case Mode::kid670: return 3.0f;
    }
    return 2.0f;
}

float CompressorSection::modeKneeDb (Mode mode) noexcept
{
    return mode == Mode::fast76 ? 4.0f : (mode == Mode::kid670 ? 14.0f : 10.0f);
}
