#pragma once

#include <JuceHeader.h>

class CompressorSection
{
public:
    enum class Mode { fastFet, smoothOpto, tubeLeveler };

    void prepare (double sampleRate);
    void reset() noexcept;
    float processDetector (float detectorInput, Mode mode, float amount,
                           float attackMs, float releaseMs) noexcept;
    float getGainReductionDb() const noexcept { return gainReductionDb; }

private:
    static float modeRatio (Mode mode) noexcept;
    static float modeKneeDb (Mode mode) noexcept;

    double currentSampleRate = 44100.0;
    float envelope = 0.0f;
    float gainReductionDb = 0.0f;
    float currentRatio = 2.0f;
    float currentKneeDb = 10.0f;
};
