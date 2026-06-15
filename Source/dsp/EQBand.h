#pragma once
#include <JuceHeader.h>

enum class EQBandType { Bell = 0, LowCut, HighCut, LowShelf, HighShelf, Notch, Tilt };
enum class EQChannelMode { Stereo = 0, Mid, Side, Left, Right };

struct EQBandState
{
    bool enabled         = true;
    EQBandType type      = EQBandType::Bell;
    EQChannelMode channelMode = EQChannelMode::Stereo;
    float frequencyHz    = 1000.0f;
    float gainDb         = 0.0f;
    float q              = 1.0f;
    float slopeDbPerOct  = 12.0f;
    bool dynamicEnabled  = false;
    float dynamicRangeDb = 0.0f;
    float thresholdDb    = -24.0f;
    float attackMs       = 10.0f;
    float releaseMs      = 120.0f;
    bool soloAudition    = false;
};

//==============================================================================
class EQBand
{
public:
    void prepare (double sampleRate) noexcept;
    void reset() noexcept;
    void setState (const EQBandState& newState) noexcept;
    void processBlock (float* leftData, float* rightData, int numSamples) noexcept;

private:
    struct BiquadCoeffs { float b0=1.f, b1=0.f, b2=0.f, a1=0.f, a2=0.f; };

    struct BiquadState
    {
        float z1=0.f, z2=0.f;
        inline float process (float x, const BiquadCoeffs& c) noexcept
        {
            float y = c.b0 * x + z1;
            z1 = c.b1 * x - c.a1 * y + z2;
            z2 = c.b2 * x - c.a2 * y;
            return y;
        }
        void reset() noexcept { z1 = z2 = 0.f; }
    };

    static constexpr int maxCascades = 4;

    std::array<BiquadCoeffs, maxCascades> cascadeCoeffs {};
    std::array<BiquadState,  maxCascades> stateA {};   // left or mid
    std::array<BiquadState,  maxCascades> stateB {};   // right or side
    int numCascades = 1;

    EQBandState currentState;
    double sampleRate = 44100.0;
    bool needsUpdate   = true;

    void updateCoefficients() noexcept;

    void calcBell      (BiquadCoeffs& c, float freqHz, float gainDb, float q) noexcept;
    void calcLowCut    (BiquadCoeffs& c, float freqHz, float q) noexcept;
    void calcHighCut   (BiquadCoeffs& c, float freqHz, float q) noexcept;
    void calcLowShelf  (BiquadCoeffs& c, float freqHz, float gainDb) noexcept;
    void calcHighShelf (BiquadCoeffs& c, float freqHz, float gainDb) noexcept;
    void calcNotch     (BiquadCoeffs& c, float freqHz, float q) noexcept;
    void calcTilt      (BiquadCoeffs& c, float freqHz, float gainDb) noexcept;
};

//==============================================================================
// Standalone spectrum analyser — defined here so both PluginProcessor and EQPanel can use it.
class SpectrumAnalyzer
{
public:
    static constexpr int fftOrder = 12;
    static constexpr int fftSize  = 1 << fftOrder;   // 4096

    void pushSamples (const float* samples, int numSamples) noexcept;
    void processFFT() noexcept;                         // call from UI timer

    const std::array<float, fftSize / 2>& getMagnitudes() const noexcept { return smoothedMagnitudes; }

    // NOTE: createAnalyzerPath is implemented in EQPanel.cpp to avoid UI-type dependency
    // Call createSpectrumPath(analyzer, bounds, sr) from EQPanel instead.

private:
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { static_cast<size_t>(fftSize),
                                                  juce::dsp::WindowingFunction<float>::hann };
    std::array<float, fftSize * 2> fftData {};
    std::array<float, fftSize>     fifo {};
    std::array<float, fftSize / 2> smoothedMagnitudes {};
    int fifoIndex = 0;
    std::atomic<bool> fftDataReady { false };
};
