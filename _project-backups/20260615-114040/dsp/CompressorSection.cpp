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
}

float CompressorSection::processDetector (float detectorInput, Mode mode, float amount,
                                          float attackMs, float releaseMs) noexcept
{
    const auto detector = std::abs (detectorInput);
    const auto timeMs = detector > envelope ? attackMs : releaseMs;
    const auto coefficient = std::exp (-1.0f / (0.001f * timeMs * static_cast<float> (currentSampleRate)));
    envelope = coefficient * envelope + (1.0f - coefficient) * detector;

    const auto levelDb = juce::Decibels::gainToDecibels (envelope, -100.0f);
    const auto thresholdDb = juce::jmap (amount, -6.0f, -30.0f);
    const auto modeSmoothing = 1.0f - std::exp (-1.0f / (0.025f * static_cast<float> (currentSampleRate)));
    currentRatio += (modeRatio (mode) - currentRatio) * modeSmoothing;
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
        case Mode::fastFet: return 4.0f;
        case Mode::smoothOpto: return 2.0f;
        case Mode::tubeLeveler: return 3.0f;
    }
    return 2.0f;
}

float CompressorSection::modeKneeDb (Mode mode) noexcept
{
    return mode == Mode::fastFet ? 5.0f : 10.0f;
}
