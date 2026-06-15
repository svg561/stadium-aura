#include "EQBand.h"

//==============================================================================
// EQBand
//==============================================================================

void EQBand::prepare (double sr) noexcept
{
    sampleRate = sr;
    needsUpdate = true;
    reset();
}

void EQBand::reset() noexcept
{
    for (auto& s : stateA) s.reset();
    for (auto& s : stateB) s.reset();
}

void EQBand::setState (const EQBandState& newState) noexcept
{
    currentState = newState;
    needsUpdate  = true;
}

void EQBand::processBlock (float* leftData, float* rightData, int numSamples) noexcept
{
    if (!currentState.enabled)
        return;

    if (needsUpdate)
    {
        updateCoefficients();
        needsUpdate = false;
    }

    const auto mode = currentState.channelMode;

    for (int i = 0; i < numSamples; ++i)
    {
        float l = leftData[i];
        float r = rightData[i];

        if (mode == EQChannelMode::Stereo)
        {
            for (int c = 0; c < numCascades; ++c)
            {
                l = stateA[static_cast<size_t>(c)].process (l, cascadeCoeffs[static_cast<size_t>(c)]);
                r = stateB[static_cast<size_t>(c)].process (r, cascadeCoeffs[static_cast<size_t>(c)]);
            }
        }
        else if (mode == EQChannelMode::Left)
        {
            for (int c = 0; c < numCascades; ++c)
                l = stateA[static_cast<size_t>(c)].process (l, cascadeCoeffs[static_cast<size_t>(c)]);
        }
        else if (mode == EQChannelMode::Right)
        {
            for (int c = 0; c < numCascades; ++c)
                r = stateB[static_cast<size_t>(c)].process (r, cascadeCoeffs[static_cast<size_t>(c)]);
        }
        else if (mode == EQChannelMode::Mid)
        {
            float mid  = (l + r) * 0.5f;
            float side = (l - r) * 0.5f;
            for (int c = 0; c < numCascades; ++c)
                mid = stateA[static_cast<size_t>(c)].process (mid, cascadeCoeffs[static_cast<size_t>(c)]);
            l = mid + side;
            r = mid - side;
        }
        else // Side
        {
            float mid  = (l + r) * 0.5f;
            float side = (l - r) * 0.5f;
            for (int c = 0; c < numCascades; ++c)
                side = stateB[static_cast<size_t>(c)].process (side, cascadeCoeffs[static_cast<size_t>(c)]);
            l = mid + side;
            r = mid - side;
        }

        leftData[i]  = l;
        rightData[i] = r;
    }
}

//==============================================================================
void EQBand::updateCoefficients() noexcept
{
    const float freq  = juce::jlimit (20.0f, 20000.0f, currentState.frequencyHz);
    const float gain  = juce::jlimit (-30.0f, 30.0f,   currentState.gainDb);
    const float q     = juce::jlimit (0.025f, 40.0f,   currentState.q);
    const float slope = currentState.slopeDbPerOct;

    // Determine cascade count for cut filters
    // Each 2nd-order biquad gives 12 dB/oct; cascade for steeper slopes.
    numCascades = 1;
    if (currentState.type == EQBandType::LowCut || currentState.type == EQBandType::HighCut)
    {
        if (slope <= 6.0f)       numCascades = 1;
        else if (slope <= 12.0f) numCascades = 1;
        else if (slope <= 24.0f) numCascades = 2;
        else if (slope <= 36.0f) numCascades = 3;
        else                     numCascades = 4;
    }
    numCascades = juce::jlimit (1, maxCascades, numCascades);

    BiquadCoeffs c;
    switch (currentState.type)
    {
        case EQBandType::Bell:      calcBell      (c, freq, gain, q);  break;
        case EQBandType::LowCut:    calcLowCut    (c, freq, (slope <= 6.0f) ? 0.5f : 0.7071f); break;
        case EQBandType::HighCut:   calcHighCut   (c, freq, (slope <= 6.0f) ? 0.5f : 0.7071f); break;
        case EQBandType::LowShelf:  calcLowShelf  (c, freq, gain);     break;
        case EQBandType::HighShelf: calcHighShelf (c, freq, gain);     break;
        case EQBandType::Notch:     calcNotch     (c, freq, q);        break;
        case EQBandType::Tilt:      calcTilt      (c, freq, gain);     break;
        default:                    break;
    }

    for (int i = 0; i < numCascades; ++i)
        cascadeCoeffs[static_cast<size_t>(i)] = c;
}

//==============================================================================
void EQBand::calcBell (BiquadCoeffs& c, float freqHz, float gainDb, float q) noexcept
{
    const float omega = juce::MathConstants<float>::twoPi * freqHz / static_cast<float> (sampleRate);
    const float cosw  = std::cos (omega);
    const float sinw  = std::sin (omega);
    const float alpha = sinw / (2.0f * q);
    const float A     = std::pow (10.0f, gainDb / 40.0f);

    const float a0    = 1.0f + alpha / A;
    c.b0 = (1.0f + alpha * A) / a0;
    c.b1 = (-2.0f * cosw)     / a0;
    c.b2 = (1.0f - alpha * A) / a0;
    c.a1 = (-2.0f * cosw)     / a0;
    c.a2 = (1.0f - alpha / A) / a0;
}

void EQBand::calcLowCut (BiquadCoeffs& c, float freqHz, float q) noexcept
{
    const float omega = juce::MathConstants<float>::twoPi * freqHz / static_cast<float> (sampleRate);
    const float cosw  = std::cos (omega);
    const float sinw  = std::sin (omega);
    const float alpha = sinw / (2.0f * q);

    const float a0    = 1.0f + alpha;
    c.b0 =  (1.0f + cosw) * 0.5f / a0;
    c.b1 = -(1.0f + cosw)        / a0;
    c.b2 =  (1.0f + cosw) * 0.5f / a0;
    c.a1 = -2.0f * cosw          / a0;
    c.a2 = (1.0f - alpha)        / a0;
}

void EQBand::calcHighCut (BiquadCoeffs& c, float freqHz, float q) noexcept
{
    const float omega = juce::MathConstants<float>::twoPi * freqHz / static_cast<float> (sampleRate);
    const float cosw  = std::cos (omega);
    const float sinw  = std::sin (omega);
    const float alpha = sinw / (2.0f * q);

    const float a0    = 1.0f + alpha;
    c.b0 = (1.0f - cosw) * 0.5f / a0;
    c.b1 = (1.0f - cosw)        / a0;
    c.b2 = (1.0f - cosw) * 0.5f / a0;
    c.a1 = -2.0f * cosw         / a0;
    c.a2 = (1.0f - alpha)       / a0;
}

void EQBand::calcLowShelf (BiquadCoeffs& c, float freqHz, float gainDb) noexcept
{
    const float omega = juce::MathConstants<float>::twoPi * freqHz / static_cast<float> (sampleRate);
    const float A     = std::pow (10.0f, gainDb / 40.0f);
    const float cosw  = std::cos (omega);
    const float sinw  = std::sin (omega);
    const float beta  = std::sqrt (A) / 0.70710678f;

    const float b0 = A * ((A + 1.0f) - (A - 1.0f) * cosw + beta * sinw);
    const float b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw);
    const float b2 = A * ((A + 1.0f) - (A - 1.0f) * cosw - beta * sinw);
    const float a0 = (A + 1.0f) + (A - 1.0f) * cosw + beta * sinw;
    const float a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosw);
    const float a2 = (A + 1.0f) + (A - 1.0f) * cosw - beta * sinw;

    c.b0 = b0 / a0; c.b1 = b1 / a0; c.b2 = b2 / a0;
    c.a1 = a1 / a0; c.a2 = a2 / a0;
}

void EQBand::calcHighShelf (BiquadCoeffs& c, float freqHz, float gainDb) noexcept
{
    const float omega = juce::MathConstants<float>::twoPi * freqHz / static_cast<float> (sampleRate);
    const float A     = std::pow (10.0f, gainDb / 40.0f);
    const float cosw  = std::cos (omega);
    const float sinw  = std::sin (omega);
    const float beta  = std::sqrt (A) / 0.70710678f;

    const float b0 =  A * ((A + 1.0f) + (A - 1.0f) * cosw + beta * sinw);
    const float b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw);
    const float b2 =  A * ((A + 1.0f) + (A - 1.0f) * cosw - beta * sinw);
    const float a0 = (A + 1.0f) - (A - 1.0f) * cosw + beta * sinw;
    const float a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosw);
    const float a2 = (A + 1.0f) - (A - 1.0f) * cosw - beta * sinw;

    c.b0 = b0 / a0; c.b1 = b1 / a0; c.b2 = b2 / a0;
    c.a1 = a1 / a0; c.a2 = a2 / a0;
}

void EQBand::calcNotch (BiquadCoeffs& c, float freqHz, float q) noexcept
{
    const float omega = juce::MathConstants<float>::twoPi * freqHz / static_cast<float> (sampleRate);
    const float cosw  = std::cos (omega);
    const float sinw  = std::sin (omega);
    const float alpha = sinw / (2.0f * q);
    const float a0    = 1.0f + alpha;

    c.b0 =  1.0f         / a0;
    c.b1 = -2.0f * cosw  / a0;
    c.b2 =  1.0f         / a0;
    c.a1 = -2.0f * cosw  / a0;
    c.a2 = (1.0f - alpha)/ a0;
}

void EQBand::calcTilt (BiquadCoeffs& c, float freqHz, float gainDb) noexcept
{
    // Tilt as a low shelf at 2x the specified frequency with gain/2,
    // approximated by a single wide bell at the tilt frequency.
    // For audio display correctness, we use a low-shelf at the given frequency.
    calcLowShelf (c, freqHz, gainDb);
}

//==============================================================================
// SpectrumAnalyzer
//==============================================================================

void SpectrumAnalyzer::pushSamples (const float* samples, int numSamples) noexcept
{
    for (int i = 0; i < numSamples; ++i)
    {
        fifo[static_cast<size_t> (fifoIndex)] = samples[i];
        ++fifoIndex;
        if (fifoIndex >= fftSize)
        {
            if (!fftDataReady.load (std::memory_order_relaxed))
            {
                std::copy (fifo.begin(), fifo.end(), fftData.begin());
                fftDataReady.store (true, std::memory_order_release);
            }
            fifoIndex = 0;
        }
    }
}

void SpectrumAnalyzer::processFFT() noexcept
{
    if (!fftDataReady.load (std::memory_order_acquire))
        return;

    std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);
    window.multiplyWithWindowingTable (fftData.data(), static_cast<size_t> (fftSize));
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    const float invSize    = 1.0f / static_cast<float> (fftSize);
    const float attackCoef = 0.85f;
    const float decayCoef  = 0.93f;
    for (size_t i = 0; i < static_cast<size_t> (fftSize / 2); ++i)
    {
        const float mag = fftData[i] * invSize;
        smoothedMagnitudes[i] = mag > smoothedMagnitudes[i]
            ? attackCoef * smoothedMagnitudes[i] + (1.0f - attackCoef) * mag
            : decayCoef  * smoothedMagnitudes[i] + (1.0f - decayCoef)  * mag;
    }

    fftDataReady.store (false, std::memory_order_release);
}

