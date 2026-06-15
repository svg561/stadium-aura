#include "EQProcessor.h"

void EQProcessor::prepare (double sr, int /*maximumBlockSize*/) noexcept
{
    sampleRate = sr;
    for (auto& band : bands)
        band.prepare (sr);
}

void EQProcessor::reset() noexcept
{
    for (auto& band : bands)
        band.reset();
}

void EQProcessor::process (juce::AudioBuffer<float>& buffer,
                            const std::array<EQBandState, 24>& bandStates) noexcept
{
    if (buffer.getNumChannels() < 1 || buffer.getNumSamples() < 1)
        return;

    // Update band states
    for (int i = 0; i < 24; ++i)
        bands[static_cast<size_t> (i)].setState (bandStates[static_cast<size_t> (i)]);

    const int numSamples  = buffer.getNumSamples();
    float* leftData  = buffer.getWritePointer (0);
    float* rightData = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : leftData;

    for (int i = 0; i < 24; ++i)
        bands[static_cast<size_t> (i)].processBlock (leftData, rightData, numSamples);
}

void EQProcessor::updateBand (int index, const EQBandState& state) noexcept
{
    if (juce::isPositiveAndBelow (index, 24))
        bands[static_cast<size_t> (index)].setState (state);
}
