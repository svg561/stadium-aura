#pragma once
#include <JuceHeader.h>
#include "EQBand.h"
#include <array>

//==============================================================================
class EQProcessor
{
public:
    void prepare (double sampleRate, int maximumBlockSize) noexcept;
    void reset() noexcept;

    // bands array must have exactly 24 elements
    void process (juce::AudioBuffer<float>& buffer,
                  const std::array<EQBandState, 24>& bands) noexcept;

    void updateBand (int index, const EQBandState& state) noexcept;

private:
    std::array<EQBand, 24> bands;
    double sampleRate = 44100.0;
};
