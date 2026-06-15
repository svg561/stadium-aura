#pragma once
#include <JuceHeader.h>
#include "../dsp/EQBand.h"

//==============================================================================
// EQ curve frequency response renderer
//==============================================================================
class EQCurveRenderer
{
public:
    static juce::Path createStaticEQPath (const std::array<EQBandState, 24>& bands,
                                          juce::Rectangle<float> bounds,
                                          double sampleRate,
                                          float visibleDbRange);

    static juce::Path createDynamicRangePath (const EQBandState& band,
                                               juce::Rectangle<float> bounds,
                                               double sampleRate,
                                               float visibleDbRange);

    static float frequencyToX (float hz, juce::Rectangle<float> bounds);
    static float xToFrequency (float x,  juce::Rectangle<float> bounds);
    static float gainToY      (float db, juce::Rectangle<float> bounds, float visibleDbRange);
    static float yToGain      (float y,  juce::Rectangle<float> bounds, float visibleDbRange);

    // Made public so external callers (e.g. getEQMagnitudeDb) can compute band response
    static float bandResponseDb (const EQBandState& band, float freqHz, double sr) noexcept;

private:
    struct BiquadCoeffs { float b0=1.f,b1=0.f,b2=0.f,a1=0.f,a2=0.f; };

    static float magnitudeAtFrequency (float freqHz, const BiquadCoeffs& c, double sr) noexcept;

    static BiquadCoeffs calcBell      (float freqHz, float gainDb, float q, double sr) noexcept;
    static BiquadCoeffs calcLowCut    (float freqHz, float q, double sr) noexcept;
    static BiquadCoeffs calcHighCut   (float freqHz, float q, double sr) noexcept;
    static BiquadCoeffs calcLowShelf  (float freqHz, float gainDb, double sr) noexcept;
    static BiquadCoeffs calcHighShelf (float freqHz, float gainDb, double sr) noexcept;
    static BiquadCoeffs calcNotch     (float freqHz, float q, double sr) noexcept;
};

//==============================================================================
// Floating popup for the selected EQ band
//==============================================================================
class EQBandPopup : public juce::Component
{
public:
    EQBandPopup();

    void setBandIndex (int index, juce::AudioProcessorValueTreeState* apvts);
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void rebuildAttachments (juce::AudioProcessorValueTreeState& apvts);

    int bandIndex = -1;
    juce::Slider freqSlider  { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Slider gainSlider  { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
    juce::Slider qSlider     { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
    juce::ComboBox typeBox, channelModeBox, slopeBox;
    juce::ToggleButton dynamicButton { "DYN" };
    juce::TextButton   deleteButton  { "DEL" };

    juce::Label freqLabel  { "", "FREQ" };
    juce::Label gainLabel  { "", "GAIN" };
    juce::Label qLabel     { "", "Q" };

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttach>> sliderAttachments;
    std::vector<std::unique_ptr<ButtonAttach>> buttonAttachments;
    std::vector<std::unique_ptr<ComboAttach>>  comboAttachments;
};

//==============================================================================
// Main EQ Panel — replaces EqSpectrumComponent in PluginEditor
//==============================================================================
class EQPanel : public juce::Component
{
public:
    EQPanel();
    ~EQPanel() override;

    void paint   (juce::Graphics&) override;
    void resized () override;

    void mouseDown        (const juce::MouseEvent&) override;
    void mouseDrag        (const juce::MouseEvent&) override;
    void mouseUp          (const juce::MouseEvent&) override;
    void mouseWheelMove   (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseMove        (const juce::MouseEvent&) override;

    // --- Wire from PluginEditor -----------------------------------------------
    void bindToParameters (juce::AudioProcessorValueTreeState& apvts);
    void updateFromParameters (juce::AudioProcessorValueTreeState& apvts, double sampleRate);

    // Spectrum analyzer feed — call from timerCallback
    void pushSpectrumSamples (const float* samples, int numSamples) noexcept;
    void processSpectrumFFT() noexcept;

    // --- Legacy compat with EqSpectrumComponent API --------------------------
    void setTone           (float value) noexcept;
    void setAnalyzerLevels (const std::array<float, 48>& levels);
    bool isExpanded        () const noexcept { return expanded; }
    void setExpanded       (bool shouldExpand);

    std::function<void (bool)>            onExpandedChanged;
    std::function<void (int,float,float)> onNodeDragged;
    std::function<void()>                 onEmptyAreaClicked; // fires in compact mode when empty area clicked

    // Feed magnitudes from an external SpectrumAnalyzer (e.g. the processor's one)
    void setExternalAnalyzerMagnitudes (const std::array<float, SpectrumAnalyzer::fftSize / 2>& mags) noexcept;

    // Programmatically select a band and show its popup (for band-card clicks in expanded panel)
    void selectBandExternally (int bandIndex);

    // Visible dB range options
    static constexpr float dbRangeOptions[] = { 3.0f, 6.0f, 12.0f, 30.0f };

    void setAuraTraceOn (bool on) noexcept { auraTraceOn = on; repaint(); }
    void setDbRangeIndex (int idx) noexcept
    {
        dbRangeIndex   = juce::jlimit (0, 3, idx);
        visibleDbRange = dbRangeOptions[static_cast<size_t> (dbRangeIndex)];
        repaint();
    }

private:
    // -------------------------------------------------------------------
    juce::Rectangle<float> getPlotBounds() const;
    int hitTestNode (juce::Point<float> pt) const;
    juce::Point<float> nodePosition (int bandIndex) const;

    void showPopupForBand (int index);
    void dismissPopup();
    void setParameterFloat (const juce::String& id, float value);
    void setParameterChoice (const juce::String& id, int index);

    void drawBackground    (juce::Graphics& g, juce::Rectangle<float> plot) const;
    void drawGrid          (juce::Graphics& g, juce::Rectangle<float> plot) const;
    void drawAuraTrace     (juce::Graphics& g, juce::Rectangle<float> plot) const;
    void drawAnalyzer      (juce::Graphics& g, juce::Rectangle<float> plot) const;
    void drawEQCurve       (juce::Graphics& g, juce::Rectangle<float> plot) const;
    void drawNodes         (juce::Graphics& g, juce::Rectangle<float> plot) const;
    void drawTooltip       (juce::Graphics& g) const;
    void drawDbRangeBar    (juce::Graphics& g) const;

    static juce::Colour nodeColour (EQBandType t);

    // -------------------------------------------------------------------
    // Colour palette
    static constexpr uint32_t bgTop        = 0xff09070f;
    static constexpr uint32_t bgBottom     = 0xff14101f;
    static constexpr uint32_t gridLine     = 0x22ffffff;
    static constexpr uint32_t analyzerLine = 0x88b9c2d0;
    static constexpr uint32_t eqCurveGold  = 0xffffd451;
    static constexpr uint32_t bandPurple   = 0xff9b5cff;
    static constexpr uint32_t bandTeal     = 0xff39e6c3;
    static constexpr uint32_t bandOrange   = 0xffff9f43;
    static constexpr uint32_t bandPink     = 0xffff4fd8;

    // -------------------------------------------------------------------
    std::array<EQBandState, 24> bandStates {};
    SpectrumAnalyzer spectrumAnalyzer;
    std::array<float, SpectrumAnalyzer::fftSize / 2> externalMagnitudes {};
    bool hasExternalMagnitudes = false;
    EQBandPopup popup;

    juce::AudioProcessorValueTreeState* apvtsRef = nullptr;
    double currentSampleRate = 44100.0;

    int selectedBand = -1;
    int hoveredBand  = -1;
    bool draggingNode = false;
    juce::Point<float> dragStartPos;
    EQBandState dragStartState;

    float visibleDbRange = 12.0f;
    int   dbRangeIndex   = 2;   // index into dbRangeOptions[]
    bool  auraTraceOn    = false;
    bool  expanded       = false;
    float toneValue      = 0.0f;

    juce::String tooltipText;
    juce::Point<float> tooltipPos;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQPanel)
};
