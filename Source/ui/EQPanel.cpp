#include "EQPanel.h"
#include <cmath>

//==============================================================================
// Standalone analyzer path builder (UI-thread only, lives in EQPanel.cpp)
//==============================================================================
static juce::Path createSpectrumPath (const SpectrumAnalyzer& sa, juce::Rectangle<float> bounds, double sr)
{
    if (sr < 1.0) return {};
    juce::Path path;
    const float sampleRate = static_cast<float> (sr);
    bool started = false;
    const auto& mags = sa.getMagnitudes();

    for (int i = 1; i < SpectrumAnalyzer::fftSize / 2; ++i)
    {
        const float freq = static_cast<float> (i) * sampleRate / static_cast<float> (SpectrumAnalyzer::fftSize);
        if (freq < 20.0f || freq > 20000.0f) continue;

        const float norm = std::log10 (freq / 20.0f) / std::log10 (20000.0f / 20.0f);
        const float x    = bounds.getX() + norm * bounds.getWidth();
        const float mag  = mags[static_cast<size_t> (i)];
        const float db   = juce::Decibels::gainToDecibels (mag + 1e-9f, -120.0f);
        const float y    = juce::jmap (db, -80.0f, 0.0f, bounds.getBottom(),
                                       bounds.getY() + bounds.getHeight() * 0.1f);

        if (!started) { path.startNewSubPath (x, y); started = true; }
        else          { path.lineTo (x, y); }
    }
    return path;
}

//==============================================================================
// EQCurveRenderer helpers
//==============================================================================

static float twoPiOverSr (double sr) { return juce::MathConstants<float>::twoPi / static_cast<float> (sr); }

EQCurveRenderer::BiquadCoeffs EQCurveRenderer::calcBell (float freqHz, float gainDb, float q, double sr) noexcept
{
    BiquadCoeffs c;
    const float omega = twoPiOverSr (sr) * juce::jlimit (20.0f, 20000.0f, freqHz);
    const float cosw  = std::cos (omega);
    const float sinw  = std::sin (omega);
    const float alpha = sinw / (2.0f * juce::jlimit (0.025f, 40.0f, q));
    const float A     = std::pow (10.0f, juce::jlimit (-30.0f, 30.0f, gainDb) / 40.0f);
    const float a0    = 1.0f + alpha / A;
    c.b0 = (1.0f + alpha * A) / a0;
    c.b1 = -2.0f * cosw / a0;
    c.b2 = (1.0f - alpha * A) / a0;
    c.a1 = -2.0f * cosw / a0;
    c.a2 = (1.0f - alpha / A) / a0;
    return c;
}

EQCurveRenderer::BiquadCoeffs EQCurveRenderer::calcLowCut (float freqHz, float q, double sr) noexcept
{
    BiquadCoeffs c;
    const float omega = twoPiOverSr (sr) * juce::jlimit (20.0f, 20000.0f, freqHz);
    const float cosw  = std::cos (omega);
    const float sinw  = std::sin (omega);
    const float alpha = sinw / (2.0f * q);
    const float a0    = 1.0f + alpha;
    c.b0 =  (1.0f + cosw) * 0.5f / a0;
    c.b1 = -(1.0f + cosw) / a0;
    c.b2 =  (1.0f + cosw) * 0.5f / a0;
    c.a1 = -2.0f * cosw / a0;
    c.a2 = (1.0f - alpha) / a0;
    return c;
}

EQCurveRenderer::BiquadCoeffs EQCurveRenderer::calcHighCut (float freqHz, float q, double sr) noexcept
{
    BiquadCoeffs c;
    const float omega = twoPiOverSr (sr) * juce::jlimit (20.0f, 20000.0f, freqHz);
    const float cosw  = std::cos (omega);
    const float sinw  = std::sin (omega);
    const float alpha = sinw / (2.0f * q);
    const float a0    = 1.0f + alpha;
    c.b0 = (1.0f - cosw) * 0.5f / a0;
    c.b1 = (1.0f - cosw) / a0;
    c.b2 = (1.0f - cosw) * 0.5f / a0;
    c.a1 = -2.0f * cosw / a0;
    c.a2 = (1.0f - alpha) / a0;
    return c;
}

EQCurveRenderer::BiquadCoeffs EQCurveRenderer::calcLowShelf (float freqHz, float gainDb, double sr) noexcept
{
    BiquadCoeffs c;
    const float omega = twoPiOverSr (sr) * juce::jlimit (20.0f, 20000.0f, freqHz);
    const float A     = std::pow (10.0f, juce::jlimit (-30.0f, 30.0f, gainDb) / 40.0f);
    const float cosw  = std::cos (omega);
    const float sinw  = std::sin (omega);
    const float beta  = std::sqrt (A) / 0.70710678f;
    const float b0_ = A * ((A+1.0f) - (A-1.0f)*cosw + beta*sinw);
    const float b1_ = 2.0f*A*((A-1.0f) - (A+1.0f)*cosw);
    const float b2_ = A*((A+1.0f) - (A-1.0f)*cosw - beta*sinw);
    const float a0  = (A+1.0f) + (A-1.0f)*cosw + beta*sinw;
    const float a1_ = -2.0f*((A-1.0f) + (A+1.0f)*cosw);
    const float a2_ = (A+1.0f) + (A-1.0f)*cosw - beta*sinw;
    c.b0=b0_/a0; c.b1=b1_/a0; c.b2=b2_/a0; c.a1=a1_/a0; c.a2=a2_/a0;
    return c;
}

EQCurveRenderer::BiquadCoeffs EQCurveRenderer::calcHighShelf (float freqHz, float gainDb, double sr) noexcept
{
    BiquadCoeffs c;
    const float omega = twoPiOverSr (sr) * juce::jlimit (20.0f, 20000.0f, freqHz);
    const float A     = std::pow (10.0f, juce::jlimit (-30.0f, 30.0f, gainDb) / 40.0f);
    const float cosw  = std::cos (omega);
    const float sinw  = std::sin (omega);
    const float beta  = std::sqrt (A) / 0.70710678f;
    const float b0_ = A*((A+1.0f) + (A-1.0f)*cosw + beta*sinw);
    const float b1_ = -2.0f*A*((A-1.0f) + (A+1.0f)*cosw);
    const float b2_ = A*((A+1.0f) + (A-1.0f)*cosw - beta*sinw);
    const float a0  = (A+1.0f) - (A-1.0f)*cosw + beta*sinw;
    const float a1_ = 2.0f*((A-1.0f) - (A+1.0f)*cosw);
    const float a2_ = (A+1.0f) - (A-1.0f)*cosw - beta*sinw;
    c.b0=b0_/a0; c.b1=b1_/a0; c.b2=b2_/a0; c.a1=a1_/a0; c.a2=a2_/a0;
    return c;
}

EQCurveRenderer::BiquadCoeffs EQCurveRenderer::calcNotch (float freqHz, float q, double sr) noexcept
{
    BiquadCoeffs c;
    const float omega = twoPiOverSr (sr) * juce::jlimit (20.0f, 20000.0f, freqHz);
    const float cosw  = std::cos (omega);
    const float sinw  = std::sin (omega);
    const float alpha = sinw / (2.0f * juce::jlimit (0.025f, 40.0f, q));
    const float a0    = 1.0f + alpha;
    c.b0 = 1.0f/a0; c.b1 = -2.0f*cosw/a0; c.b2 = 1.0f/a0;
    c.a1 = -2.0f*cosw/a0; c.a2 = (1.0f-alpha)/a0;
    return c;
}

float EQCurveRenderer::magnitudeAtFrequency (float freqHz, const BiquadCoeffs& c, double sr) noexcept
{
    const float omega = twoPiOverSr (sr) * juce::jlimit (20.0f, 20000.0f, freqHz);
    const float cosw = std::cos (omega);
    const float sinw = std::sin (omega);
    const float cos2w = std::cos (2.0f * omega);
    const float sin2w = std::sin (2.0f * omega);

    const float numR = c.b0 + c.b1*cosw + c.b2*cos2w;
    const float numI = -(c.b1*sinw + c.b2*sin2w);
    const float denR = 1.0f + c.a1*cosw + c.a2*cos2w;
    const float denI = -(c.a1*sinw + c.a2*sin2w);

    const float numMag2 = numR*numR + numI*numI;
    const float denMag2 = denR*denR + denI*denI;
    if (denMag2 < 1e-15f) return 0.0f;
    return std::sqrt (numMag2 / denMag2);
}

float EQCurveRenderer::bandResponseDb (const EQBandState& band, float freqHz, double sr) noexcept
{
    if (!band.enabled) return 0.0f;

    // Determine cascade count (cut filters only)
    int cascades = 1;
    if (band.type == EQBandType::LowCut || band.type == EQBandType::HighCut)
    {
        const float s = band.slopeDbPerOct;
        if      (s <= 12.0f) cascades = 1;
        else if (s <= 24.0f) cascades = 2;
        else if (s <= 36.0f) cascades = 3;
        else                  cascades = 4;
    }

    BiquadCoeffs c;
    switch (band.type)
    {
        case EQBandType::Bell:      c = calcBell      (band.frequencyHz, band.gainDb, band.q, sr); break;
        case EQBandType::LowCut:    c = calcLowCut    (band.frequencyHz, 0.7071f, sr); break;
        case EQBandType::HighCut:   c = calcHighCut   (band.frequencyHz, 0.7071f, sr); break;
        case EQBandType::LowShelf:  c = calcLowShelf  (band.frequencyHz, band.gainDb, sr); break;
        case EQBandType::HighShelf: c = calcHighShelf (band.frequencyHz, band.gainDb, sr); break;
        case EQBandType::Notch:     c = calcNotch     (band.frequencyHz, band.q, sr); break;
        case EQBandType::Tilt:      c = calcLowShelf  (band.frequencyHz, band.gainDb, sr); break;
        default:                    return 0.0f;
    }

    float mag = 1.0f;
    for (int i = 0; i < cascades; ++i)
        mag *= magnitudeAtFrequency (freqHz, c, sr);

    return juce::Decibels::gainToDecibels (mag + 1e-10f, -120.0f);
}

float EQCurveRenderer::frequencyToX (float hz, juce::Rectangle<float> bounds)
{
    const float norm = std::log10 (juce::jlimit (20.0f, 20000.0f, hz) / 20.0f)
                       / std::log10 (20000.0f / 20.0f);
    return bounds.getX() + norm * bounds.getWidth();
}

float EQCurveRenderer::xToFrequency (float x, juce::Rectangle<float> bounds)
{
    const float norm = juce::jlimit (0.0f, 1.0f, (x - bounds.getX()) / bounds.getWidth());
    return 20.0f * std::pow (20000.0f / 20.0f, norm);
}

float EQCurveRenderer::gainToY (float db, juce::Rectangle<float> bounds, float visibleDbRange)
{
    const float norm = juce::jlimit (0.0f, 1.0f, (db / visibleDbRange) * 0.5f + 0.5f);
    return bounds.getBottom() - norm * bounds.getHeight();
}

float EQCurveRenderer::yToGain (float y, juce::Rectangle<float> bounds, float visibleDbRange)
{
    const float norm = juce::jlimit (0.0f, 1.0f, (bounds.getBottom() - y) / bounds.getHeight());
    return (norm - 0.5f) * 2.0f * visibleDbRange;
}

juce::Path EQCurveRenderer::createStaticEQPath (const std::array<EQBandState, 24>& bands,
                                                  juce::Rectangle<float> bounds,
                                                  double sampleRate,
                                                  float visibleDbRange)
{
    if (sampleRate < 1.0) return {};
    juce::Path path;
    constexpr int numPoints = 512;
    bool started = false;

    for (int i = 0; i < numPoints; ++i)
    {
        const float norm = static_cast<float> (i) / static_cast<float> (numPoints - 1);
        const float freq = 20.0f * std::pow (20000.0f / 20.0f, norm);
        const float x = bounds.getX() + norm * bounds.getWidth();

        float totalDb = 0.0f;
        for (const auto& band : bands)
            totalDb += bandResponseDb (band, freq, sampleRate);

        totalDb = juce::jlimit (-visibleDbRange, visibleDbRange, totalDb);
        const float y = gainToY (totalDb, bounds, visibleDbRange);

        if (!started) { path.startNewSubPath (x, y); started = true; }
        else          { path.lineTo (x, y); }
    }
    return path;
}

juce::Path EQCurveRenderer::createDynamicRangePath (const EQBandState& band,
                                                      juce::Rectangle<float> bounds,
                                                      double sampleRate,
                                                      float visibleDbRange)
{
    if (!band.dynamicEnabled || sampleRate < 1.0) return {};
    // Draw a shaded region showing the dynamic range around the static curve
    juce::Path path;
    constexpr int numPoints = 256;

    for (int i = 0; i < numPoints; ++i)
    {
        const float norm = static_cast<float> (i) / static_cast<float> (numPoints - 1);
        const float freq = 20.0f * std::pow (20000.0f / 20.0f, norm);
        const float x    = bounds.getX() + norm * bounds.getWidth();
        const float baseDb = bandResponseDb (band, freq, sampleRate);
        const float topY   = gainToY (juce::jlimit (-visibleDbRange, visibleDbRange, baseDb + band.dynamicRangeDb), bounds, visibleDbRange);

        if (i == 0) path.startNewSubPath (x, topY);
        else        path.lineTo (x, topY);
    }
    return path;
}

//==============================================================================
// EQBandPopup
//==============================================================================

EQBandPopup::EQBandPopup()
{
    for (auto* label : { &freqLabel, &gainLabel, &qLabel })
    {
        label->setFont (juce::FontOptions (9.5f, juce::Font::bold));
        label->setColour (juce::Label::textColourId, juce::Colour (0xffb0a090));
        label->setJustificationType (juce::Justification::centred);
        addAndMakeVisible (*label);
    }
    for (auto* s : { &freqSlider, &gainSlider, &qSlider })
    {
        s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
        addAndMakeVisible (*s);
    }
    typeBox.addItemList ({"Bell","Low Cut","High Cut","Low Shelf","High Shelf","Notch","Tilt"}, 1);
    channelModeBox.addItemList ({"Stereo","Mid","Side","Left","Right"}, 1);
    slopeBox.addItemList ({"6 dB","12 dB","18 dB","24 dB","36 dB","48 dB","72 dB","96 dB"}, 1);
    for (auto* c : { &typeBox, &channelModeBox, &slopeBox })
        addAndMakeVisible (*c);
    addAndMakeVisible (dynamicButton);
    addAndMakeVisible (deleteButton);
    setInterceptsMouseClicks (true, true);
    setVisible (false);
}

void EQBandPopup::setBandIndex (int index, juce::AudioProcessorValueTreeState* apvts)
{
    bandIndex = index;
    sliderAttachments.clear();
    buttonAttachments.clear();
    comboAttachments.clear();

    if (apvts == nullptr || !juce::isPositiveAndBelow (index, 24))
    {
        setVisible (false);
        return;
    }
    rebuildAttachments (*apvts);
    setVisible (true);
    repaint();
}

void EQBandPopup::rebuildAttachments (juce::AudioProcessorValueTreeState& apvts)
{
    const auto pfx = "EQ_BAND_" + juce::String (bandIndex + 1).paddedLeft ('0', 2) + "_";

    sliderAttachments.push_back (std::make_unique<SliderAttach> (apvts, pfx + "FREQ", freqSlider));
    sliderAttachments.push_back (std::make_unique<SliderAttach> (apvts, pfx + "GAIN", gainSlider));
    sliderAttachments.push_back (std::make_unique<SliderAttach> (apvts, pfx + "Q",    qSlider));
    buttonAttachments.push_back (std::make_unique<ButtonAttach> (apvts, pfx + "DYNAMIC_ENABLED", dynamicButton));
    comboAttachments .push_back (std::make_unique<ComboAttach>  (apvts, pfx + "TYPE",         typeBox));
    comboAttachments .push_back (std::make_unique<ComboAttach>  (apvts, pfx + "CHANNEL_MODE", channelModeBox));
    comboAttachments .push_back (std::make_unique<ComboAttach>  (apvts, pfx + "SLOPE",        slopeBox));
}

void EQBandPopup::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (juce::Colour (0xd8101018));
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (juce::Colour (0x449b5cff));
    g.drawRoundedRectangle (bounds, 8.0f, 1.5f);

    if (bandIndex >= 0)
    {
        g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        g.setColour (juce::Colour (0xff9b5cff));
        g.drawText ("BAND " + juce::String (bandIndex + 1),
                    getLocalBounds().removeFromTop (16),
                    juce::Justification::centred);
    }
}

void EQBandPopup::resized()
{
    auto area = getLocalBounds().reduced (6, 20);

    auto comboRow = area.removeFromTop (22);
    typeBox.setBounds        (comboRow.removeFromLeft (90).reduced (2, 0));
    channelModeBox.setBounds (comboRow.removeFromLeft (70).reduced (2, 0));
    slopeBox.setBounds       (comboRow.reduced (2, 0));

    area.removeFromTop (4);

    auto sliderRow = area.removeFromTop (80);
    const int sliderW = sliderRow.getWidth() / 3;
    freqLabel .setBounds (sliderRow.removeFromLeft (sliderW).removeFromTop (14));
    // (reset so we can do the sliders again)
    sliderRow = juce::Rectangle<int>(area.getX() - sliderW * 3 + 6, area.getY() - 80 + 14,
                                      sliderW * 3 - 12, 66); // adjust without going negative
    // Simpler approach: use absolute positioning
    const int sy = getLocalBounds().getHeight() - 96;
    const int sw = (getWidth() - 12) / 3;
    freqLabel .setBounds (6, sy, sw, 12);
    gainLabel .setBounds (6 + sw, sy, sw, 12);
    qLabel    .setBounds (6 + sw*2, sy, sw, 12);
    freqSlider.setBounds (6, sy + 12, sw, 64);
    gainSlider.setBounds (6 + sw, sy + 12, sw, 64);
    qSlider   .setBounds (6 + sw*2, sy + 12, sw, 64);

    auto btRow = getLocalBounds().removeFromBottom (26).reduced (6, 3);
    deleteButton .setBounds (btRow.removeFromRight (40));
    dynamicButton.setBounds (btRow.removeFromLeft  (50));
}

//==============================================================================
// EQPanel
//==============================================================================

constexpr float EQPanel::dbRangeOptions[];

EQPanel::EQPanel()
{
    addChildComponent (popup);
    setOpaque (false);
}

EQPanel::~EQPanel() {}

//------------------------------------------------------------------------------
void EQPanel::bindToParameters (juce::AudioProcessorValueTreeState& apvts)
{
    apvtsRef = &apvts;
}

void EQPanel::updateFromParameters (juce::AudioProcessorValueTreeState& apvts, double sampleRate)
{
    currentSampleRate = sampleRate;
    auto get = [&apvts] (const juce::String& id) -> float
    {
        if (auto* p = apvts.getRawParameterValue (id))
            return p->load();
        return 0.0f;
    };

    static const float slopeTable[] = { 6.f,12.f,18.f,24.f,36.f,48.f,72.f,96.f };

    for (int b = 0; b < 24; ++b)
    {
        const auto pfx = "EQ_BAND_" + juce::String (b + 1).paddedLeft ('0', 2) + "_";
        auto& s = bandStates[static_cast<size_t> (b)];
        s.enabled       = get (pfx + "ENABLED") > 0.5f;
        s.type          = static_cast<EQBandType> (juce::jlimit (0, 6, (int) get (pfx + "TYPE")));
        s.frequencyHz   = get (pfx + "FREQ");
        s.gainDb        = get (pfx + "GAIN");
        s.q             = get (pfx + "Q");
        const int si    = juce::jlimit (0, 7, (int) get (pfx + "SLOPE"));
        s.slopeDbPerOct = slopeTable[si];
        s.channelMode   = static_cast<EQChannelMode> (juce::jlimit (0, 4, (int) get (pfx + "CHANNEL_MODE")));
        s.dynamicEnabled  = get (pfx + "DYNAMIC_ENABLED") > 0.5f;
        s.dynamicRangeDb  = get (pfx + "DYNAMIC_RANGE");
        s.thresholdDb     = get (pfx + "THRESHOLD");
        s.attackMs        = get (pfx + "ATTACK");
        s.releaseMs       = get (pfx + "RELEASE");
    }
    repaint();
}

void EQPanel::setTone (float value) noexcept { toneValue = value; repaint(); }
void EQPanel::setAnalyzerLevels (const std::array<float, 48>&) { repaint(); }

void EQPanel::setExpanded (bool shouldExpand)
{
    if (expanded == shouldExpand) return;
    expanded = shouldExpand;
    if (onExpandedChanged) onExpandedChanged (expanded);
}

void EQPanel::pushSpectrumSamples (const float* samples, int numSamples) noexcept
{
    spectrumAnalyzer.pushSamples (samples, numSamples);
}

void EQPanel::processSpectrumFFT() noexcept
{
    spectrumAnalyzer.processFFT();
}

//------------------------------------------------------------------------------
void EQPanel::paint (juce::Graphics& g)
{
    auto plot = getPlotBounds();

    drawBackground (g, plot);
    drawGrid       (g, plot);
    if (auraTraceOn) drawAuraTrace (g, plot);
    drawAnalyzer   (g, plot);
    drawEQCurve    (g, plot);
    drawNodes      (g, plot);
    drawTooltip    (g);
    drawDbRangeBar (g);
}

void EQPanel::resized()
{
    popup.setBounds (getLocalBounds().reduced (6).removeFromBottom (160));
}

//------------------------------------------------------------------------------
juce::Rectangle<float> EQPanel::getPlotBounds() const
{
    return getLocalBounds().toFloat().reduced (4.0f, 2.0f).withTrimmedBottom (24.0f);
}

//------------------------------------------------------------------------------
void EQPanel::drawBackground (juce::Graphics& g, juce::Rectangle<float> plot) const
{
    juce::ColourGradient grad (juce::Colour (bgTop), plot.getTopLeft(),
                               juce::Colour (bgBottom), plot.getBottomLeft(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (plot, 6.0f);
    g.setColour (juce::Colour (0x33ffffff));
    g.drawRoundedRectangle (plot, 6.0f, 0.5f);
}

void EQPanel::drawGrid (juce::Graphics& g, juce::Rectangle<float> plot) const
{
    g.setColour (juce::Colour (gridLine));

    static const float freqLines[] = { 20.f, 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f, 20000.f };
    static const char* freqLabels[]= { "20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };

    for (int i = 0; i < 10; ++i)
    {
        const float x = EQCurveRenderer::frequencyToX (freqLines[i], plot);
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());

        g.setFont (juce::FontOptions (8.5f));
        g.setColour (juce::Colour (0x66ffffff));
        g.drawText (freqLabels[i],
                    juce::Rectangle<float> (x - 16.0f, plot.getBottom() - 14.0f, 32.0f, 12.0f),
                    juce::Justification::centred, false);
        g.setColour (juce::Colour (gridLine));
    }

    // Horizontal dB lines
    const float dbStep = visibleDbRange <= 6.0f ? 3.0f : (visibleDbRange <= 12.0f ? 6.0f : 12.0f);
    for (float db = -visibleDbRange; db <= visibleDbRange; db += dbStep)
    {
        const float y = EQCurveRenderer::gainToY (db, plot, visibleDbRange);
        if (db == 0.0f)
        {
            g.setColour (juce::Colour (0x44ffffff));
            g.drawLine (plot.getX(), y, plot.getRight(), y, 1.0f);
        }
        else
        {
            g.setColour (juce::Colour (gridLine));
            g.drawLine (plot.getX(), y, plot.getRight(), y, 0.5f);
        }

        g.setFont (juce::FontOptions (8.0f));
        g.setColour (juce::Colour (0x66ffffff));
        g.drawText ((db > 0 ? "+" : "") + juce::String (db, 0) + " dB",
                    juce::Rectangle<float> (plot.getX() + 2.0f, y - 8.0f, 36.0f, 14.0f),
                    juce::Justification::left, false);
    }
}

void EQPanel::drawAuraTrace (juce::Graphics& g, juce::Rectangle<float> plot) const
{
    struct Zone { float lo, hi; uint32_t colour; const char* name; };
    static const Zone zones[] = {
        {  120.0f,  350.0f, 0x10ff9000, "MUD"      },
        {  350.0f,  700.0f, 0x10ffff00, "BOX"      },
        { 1500.0f, 4000.0f, 0x10a0c8ff, "PRESENCE" },
        { 4000.0f, 7000.0f, 0x1000ffff, "BITE"     },
        {10000.0f,18000.0f, 0x1040a0ff, "AIR"      }
    };
    for (const auto& z : zones)
    {
        const float x1 = EQCurveRenderer::frequencyToX (z.lo, plot);
        const float x2 = EQCurveRenderer::frequencyToX (z.hi, plot);
        g.setColour (juce::Colour (z.colour));
        g.fillRect (x1, plot.getY(), x2 - x1, plot.getHeight());

        g.setFont (juce::FontOptions (8.5f, juce::Font::italic));
        g.setColour (juce::Colour (z.colour).withAlpha (0.7f));
        g.drawText (z.name,
                    juce::Rectangle<float> (x1, plot.getY() + 4.0f, x2 - x1, 14.0f),
                    juce::Justification::centred, false);
    }
}

static juce::Path createSpectrumPathFromMags (const std::array<float, SpectrumAnalyzer::fftSize / 2>& mags,
                                               juce::Rectangle<float> bounds, double sr)
{
    if (sr < 1.0) return {};
    juce::Path path;
    const float sampleRate = static_cast<float> (sr);
    bool started = false;

    for (int i = 1; i < SpectrumAnalyzer::fftSize / 2; ++i)
    {
        const float freq = static_cast<float> (i) * sampleRate / static_cast<float> (SpectrumAnalyzer::fftSize);
        if (freq < 20.0f || freq > 20000.0f) continue;

        const float norm = std::log10 (freq / 20.0f) / std::log10 (20000.0f / 20.0f);
        const float x    = bounds.getX() + norm * bounds.getWidth();
        const float mag  = mags[static_cast<size_t> (i)];
        const float db   = juce::Decibels::gainToDecibels (mag + 1e-9f, -120.0f);
        const float y    = juce::jmap (db, -80.0f, 0.0f, bounds.getBottom(),
                                       bounds.getY() + bounds.getHeight() * 0.1f);

        if (!started) { path.startNewSubPath (x, y); started = true; }
        else          { path.lineTo (x, y); }
    }
    return path;
}

void EQPanel::setExternalAnalyzerMagnitudes (const std::array<float, SpectrumAnalyzer::fftSize / 2>& mags) noexcept
{
    externalMagnitudes = mags;
    hasExternalMagnitudes = true;
}

void EQPanel::selectBandExternally (int bandIndex)
{
    showPopupForBand (bandIndex);
    repaint();
}

void EQPanel::drawAnalyzer (juce::Graphics& g, juce::Rectangle<float> plot) const
{
    juce::Path strokePath;
    if (hasExternalMagnitudes)
        strokePath = createSpectrumPathFromMags (externalMagnitudes, plot, currentSampleRate);
    else
        strokePath = createSpectrumPath (spectrumAnalyzer, plot, currentSampleRate);
    if (strokePath.isEmpty()) return;

    // Filled area
    juce::Path fillPath = strokePath;
    fillPath.lineTo (plot.getRight(), plot.getBottom());
    fillPath.lineTo (plot.getX(), plot.getBottom());
    fillPath.closeSubPath();

    g.setColour (juce::Colour (analyzerLine).withAlpha (0.18f));
    g.fillPath (fillPath);

    g.setColour (juce::Colour (analyzerLine));
    g.strokePath (strokePath, juce::PathStrokeType (1.2f));
}

void EQPanel::drawEQCurve (juce::Graphics& g, juce::Rectangle<float> plot) const
{
    const auto eqPath = EQCurveRenderer::createStaticEQPath (bandStates, plot, currentSampleRate, visibleDbRange);
    if (eqPath.isEmpty()) return;

    // Glow
    g.setColour (juce::Colour (eqCurveGold).withAlpha (0.25f));
    g.strokePath (eqPath, juce::PathStrokeType (7.0f));

    // Curve
    g.setColour (juce::Colour (eqCurveGold));
    g.strokePath (eqPath, juce::PathStrokeType (2.5f));
}

juce::Colour EQPanel::nodeColour (EQBandType t)
{
    switch (t)
    {
        case EQBandType::LowCut:
        case EQBandType::HighCut:    return juce::Colour (EQPanel::bandTeal);
        case EQBandType::LowShelf:
        case EQBandType::HighShelf:  return juce::Colour (EQPanel::bandOrange);
        case EQBandType::Notch:      return juce::Colour (EQPanel::bandPink);
        case EQBandType::Bell:
        case EQBandType::Tilt:
        default:                     return juce::Colour (EQPanel::bandPurple);
    }
}

juce::Point<float> EQPanel::nodePosition (int bandIdx) const
{
    auto plot = getPlotBounds();
    const auto& s = bandStates[static_cast<size_t> (bandIdx)];
    const float x = EQCurveRenderer::frequencyToX (s.frequencyHz, plot);
    float db = s.gainDb;
    if (s.type == EQBandType::LowCut || s.type == EQBandType::HighCut || s.type == EQBandType::Notch)
        db = 0.0f;
    const float y = EQCurveRenderer::gainToY (db, plot, visibleDbRange);
    return { x, y };
}

void EQPanel::drawNodes (juce::Graphics& g, juce::Rectangle<float> /*plot*/) const
{
    for (int i = 0; i < 24; ++i)
    {
        const auto& s = bandStates[static_cast<size_t> (i)];
        const auto  pos = nodePosition (i);
        const float radius = (i == selectedBand) ? 9.0f : 7.0f;
        const auto  col = nodeColour (s.type);
        const float alpha = s.enabled ? 1.0f : 0.4f;

        // Outer glow ring for selected
        if (i == selectedBand)
        {
            g.setColour (col.withAlpha (0.35f * alpha));
            g.fillEllipse (pos.x - radius - 4.0f, pos.y - radius - 4.0f,
                           (radius + 4.0f) * 2.0f, (radius + 4.0f) * 2.0f);
        }

        // Fill + stroke
        juce::ColourGradient nodeGrad (col.brighter (0.4f).withAlpha (alpha), pos.x - radius * 0.3f, pos.y - radius * 0.3f,
                                       col.darker  (0.3f).withAlpha (alpha), pos.x + radius * 0.4f, pos.y + radius * 0.5f, true);
        g.setGradientFill (nodeGrad);
        g.fillEllipse (pos.x - radius, pos.y - radius, radius * 2.0f, radius * 2.0f);
        g.setColour (col.withAlpha (alpha));
        g.drawEllipse (pos.x - radius, pos.y - radius, radius * 2.0f, radius * 2.0f, 1.0f);

        // Band number
        if (radius >= 7.0f)
        {
            g.setFont (juce::FontOptions (7.5f, juce::Font::bold));
            g.setColour (juce::Colours::white.withAlpha (alpha));
            g.drawText (juce::String (i + 1),
                        juce::Rectangle<float> (pos.x - radius, pos.y - radius, radius * 2.0f, radius * 2.0f),
                        juce::Justification::centred, false);
        }
    }
}

void EQPanel::drawTooltip (juce::Graphics& g) const
{
    if (tooltipText.isEmpty() || !draggingNode) return;
    const float w = 110.0f, h = 22.0f;
    auto r = juce::Rectangle<float> (tooltipPos.x + 12.0f, tooltipPos.y - 12.0f, w, h);
    // Keep within component bounds
    if (r.getRight()  > getWidth())  r.setX (tooltipPos.x - w - 8.0f);
    if (r.getBottom() > getHeight()) r.setY (tooltipPos.y - h - 4.0f);

    g.setColour (juce::Colour (0xd0101018));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (juce::Colour (0xffffd451));
    g.drawRoundedRectangle (r, 4.0f, 0.8f);
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.setColour (juce::Colours::white);
    g.drawText (tooltipText, r.reduced (4.0f, 2.0f), juce::Justification::centredLeft, false);
}

void EQPanel::drawDbRangeBar (juce::Graphics& g) const
{
    const auto bar = getLocalBounds().removeFromBottom (20).toFloat().reduced (4.0f, 3.0f);
    const float w = bar.getWidth() / 4.0f;
    static const char* labels[] = { "±3", "±6", "±12", "±30" };

    for (int i = 0; i < 4; ++i)
    {
        auto cell = bar.withWidth (w).translated (w * i, 0.0f).reduced (1.0f, 0.0f);
        if (i == dbRangeIndex)
        {
            g.setColour (juce::Colour (0x44ffd451));
            g.fillRoundedRectangle (cell, 3.0f);
            g.setColour (juce::Colour (0x88ffd451));
            g.drawRoundedRectangle (cell, 3.0f, 0.7f);
        }
        else
        {
            g.setColour (juce::Colour (0x22ffffff));
            g.fillRoundedRectangle (cell, 3.0f);
        }
        g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        g.setColour (i == dbRangeIndex ? juce::Colour (0xffffd451) : juce::Colour (0x88ffffff));
        g.drawText (labels[i], cell, juce::Justification::centred, false);
    }
}

//------------------------------------------------------------------------------
int EQPanel::hitTestNode (juce::Point<float> pt) const
{
    // Test in reverse order so higher-indexed (later) bands are "on top"
    for (int i = 23; i >= 0; --i)
    {
        if (pt.getDistanceFrom (nodePosition (i)) < 11.0f)
            return i;
    }
    return -1;
}

void EQPanel::showPopupForBand (int index)
{
    selectedBand = index;
    if (!juce::isPositiveAndBelow (index, 24))
    {
        dismissPopup();
        return;
    }
    popup.setBandIndex (index, apvtsRef);

    // Position popup near node, keeping within bounds
    auto nodePos = nodePosition (index);
    const int pw = 250, ph = 170;
    int px = juce::roundToInt (nodePos.x) + 14;
    int py = juce::roundToInt (nodePos.y) - ph / 2;
    px = juce::jlimit (0, getWidth()  - pw, px);
    py = juce::jlimit (0, getHeight() - ph, py);
    popup.setBounds (px, py, pw, ph);
    popup.setVisible (true);
    popup.toFront (false);
    repaint();
}

void EQPanel::dismissPopup()
{
    selectedBand = -1;
    popup.setVisible (false);
    popup.setBandIndex (-1, nullptr);
    repaint();
}

void EQPanel::setParameterFloat (const juce::String& id, float value)
{
    if (apvtsRef == nullptr) return;
    if (auto* p = apvtsRef->getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void EQPanel::setParameterChoice (const juce::String& id, int index)
{
    if (apvtsRef == nullptr) return;
    if (auto* p = apvtsRef->getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (index)));
}

//------------------------------------------------------------------------------
void EQPanel::mouseMove (const juce::MouseEvent& e)
{
    const int h = hitTestNode (e.position);
    if (h != hoveredBand)
    {
        hoveredBand = h;
        setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor
                               : juce::MouseCursor::NormalCursor);
    }
}

void EQPanel::mouseDown (const juce::MouseEvent& e)
{
    // Click outside popup dismisses it (if popup is visible and click not on it)
    if (popup.isVisible())
    {
        if (!popup.getBounds().contains (e.getPosition()))
            dismissPopup();
    }

    const int hit = hitTestNode (e.position);

    if (e.mods.isRightButtonDown())
    {
        if (hit >= 0)
        {
            // Context menu
            juce::PopupMenu menu;
            menu.addItem (1, "Reset gain to 0 dB");
            menu.addItem (2, "Delete band");
            menu.addSeparator();
            menu.addItem (3, "Bell");
            menu.addItem (4, "Low Cut");
            menu.addItem (5, "High Cut");
            menu.addItem (6, "Low Shelf");
            menu.addItem (7, "High Shelf");
            menu.addItem (8, "Notch");

            menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                [this, hit] (int result)
                {
                    const auto pfx = "EQ_BAND_" + juce::String (hit + 1).paddedLeft ('0', 2) + "_";
                    if (result == 1) setParameterFloat (pfx + "GAIN", 0.0f);
                    if (result == 2) setParameterFloat (pfx + "ENABLED", 0.0f);
                    if (result >= 3) setParameterChoice (pfx + "TYPE", result - 3);
                });
        }
        return;
    }

    if (hit >= 0)
    {
        selectedBand = hit;
        showPopupForBand (hit);
        draggingNode   = true;
        dragStartPos   = e.position;
        dragStartState = bandStates[static_cast<size_t> (hit)];
    }
    else if (!expanded && onEmptyAreaClicked)
    {
        onEmptyAreaClicked();
    }
}

void EQPanel::mouseDrag (const juce::MouseEvent& e)
{
    if (!draggingNode || !juce::isPositiveAndBelow (selectedBand, 24))
        return;

    auto plot = getPlotBounds();
    const float scale = e.mods.isShiftDown() ? 0.1f : 1.0f;

    const auto pfx = "EQ_BAND_" + juce::String (selectedBand + 1).paddedLeft ('0', 2) + "_";

    // X → frequency using scaled displacement from drag start
    const float scaledX = dragStartPos.x + (e.position.x - dragStartPos.x) * scale;
    const float newFreq = juce::jlimit (20.0f, 20000.0f,
        EQCurveRenderer::xToFrequency (scaledX, plot));
    setParameterFloat (pfx + "FREQ", newFreq);

    // Y → gain (only for bands that support gain)
    const auto& st = bandStates[static_cast<size_t> (selectedBand)];
    const bool hasGain = (st.type == EQBandType::Bell || st.type == EQBandType::LowShelf
                       || st.type == EQBandType::HighShelf || st.type == EQBandType::Tilt);
    if (hasGain)
    {
        const float scaledY   = dragStartPos.y + (e.position.y - dragStartPos.y) * scale;
        const float baseGainY = EQCurveRenderer::yToGain (dragStartPos.y, plot, visibleDbRange);
        const float curGainY  = EQCurveRenderer::yToGain (scaledY,         plot, visibleDbRange);
        const float newGain   = juce::jlimit (-30.0f, 30.0f,
            dragStartState.gainDb + (curGainY - baseGainY));
        setParameterFloat (pfx + "GAIN", newGain);
    }

    // Tooltip
    tooltipPos = e.position;
    const float dispFreq = newFreq;
    tooltipText = (dispFreq >= 1000.0f
                    ? juce::String (dispFreq / 1000.0f, 2) + " kHz"
                    : juce::String (juce::roundToInt (dispFreq)) + " Hz");
    if (hasGain)
        tooltipText += "  " + juce::String (st.gainDb, 1) + " dB";
    repaint();
}

void EQPanel::mouseUp (const juce::MouseEvent&)
{
    draggingNode   = false;
    tooltipText    = {};
    repaint();
}

void EQPanel::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int hit = hitTestNode (e.position);
    if (hit >= 0)
    {
        // Reset gain
        setParameterFloat ("EQ_BAND_" + juce::String (hit + 1).paddedLeft ('0', 2) + "_GAIN", 0.0f);
    }
    else
    {
        // Check dB range bar
        auto bar = getLocalBounds().removeFromBottom (20).toFloat().reduced (4.0f, 3.0f);
        if (bar.contains (e.position))
        {
            const float normX = (e.position.x - bar.getX()) / bar.getWidth();
            dbRangeIndex  = juce::jlimit (0, 3, (int) (normX * 4.0f));
            visibleDbRange = dbRangeOptions[static_cast<size_t> (dbRangeIndex)];
            repaint();
        }
    }
}

void EQPanel::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    // dB range bar
    auto bar = getLocalBounds().removeFromBottom (20).toFloat().reduced (4.0f, 3.0f);
    if (bar.contains (e.position))
    {
        dbRangeIndex = juce::jlimit (0, 3, dbRangeIndex + (wheel.deltaY > 0 ? -1 : 1));
        visibleDbRange = dbRangeOptions[static_cast<size_t> (dbRangeIndex)];
        repaint();
        return;
    }

    // Q control on node
    const int hit = hitTestNode (e.position);
    if (hit >= 0 && apvtsRef != nullptr)
    {
        const auto pfx = "EQ_BAND_" + juce::String (hit + 1).paddedLeft ('0', 2) + "_";
        if (auto* qParam = apvtsRef->getRawParameterValue (pfx + "Q"))
        {
            const float currentQ = qParam->load();
            const float delta    = wheel.deltaY * (wheel.isReversed ? -1.0f : 1.0f);
            const float newQ     = juce::jlimit (0.025f, 40.0f, currentQ * std::pow (2.0f, delta));
            setParameterFloat (pfx + "Q", newQ);
        }
    }
}
