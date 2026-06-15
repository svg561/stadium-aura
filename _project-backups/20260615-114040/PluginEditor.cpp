#include "PluginEditor.h"

StadiumAuraAudioProcessorEditor::StadiumAuraAudioProcessorEditor (StadiumAuraAudioProcessor& p)
    : AudioProcessorEditor (&p), auraProcessorRef (p)
{
    setLookAndFeel (&lookAndFeel);
    setResizable (true, true);
    setResizeLimits (1000, 600, 1600, 960);
    setSize (1280, 740);
    aura.setHeroStyle (true);

    PremiumKnob* knobs[] { &input, &output, &mix, &aura, &tube, &saturation, &transformer,
                           &summing, &glue, &width, &compression, &ceiling, &bias, &makeup, &tone };
    const char* ids[] { "input", "output", "mix", "aura", "tubeDrive", "saturation", "transformer",
                        "summing", "glue", "width", "compressorAmount", "ceiling", "harmonicBias", "makeup", "tone" };
    for (size_t i = 0; i < std::size (knobs); ++i)
        addKnob (*knobs[i], ids[i], sliderAttachments[i]);

    for (auto* button : { &bypass, &mono, &limiter, &dim }) addAndMakeVisible (*button);
    bypassAttachment = std::make_unique<ButtonAttachment> (auraProcessorRef.apvts, "bypass", bypass);
    monoAttachment = std::make_unique<ButtonAttachment> (auraProcessorRef.apvts, "monoCheck", mono);
    limiterAttachment = std::make_unique<ButtonAttachment> (auraProcessorRef.apvts, "limiter", limiter);
    dimAttachment = std::make_unique<ButtonAttachment> (auraProcessorRef.apvts, "dim", dim);

    compressorMode.addItemList ({ "Fast FET", "Smooth Opto", "Tube Leveler" }, 1);
    trackCount.addItemList ({ "1 Track", "8 Tracks", "16 Tracks", "24 Tracks", "32 Tracks" }, 1);
    quality.addItemList ({ "Eco", "Normal", "High", "Ultra" }, 1);
    micCharacter.addItemList ({ "67 Vintage Smooth", "C12 Open Air", "251 Classic", "87 Modern Balanced", "47 Velvet Tube", "251E Silky Air", "800G Air Pop" }, 1);
    preampMode.addItemList ({ "73 Vintage Iron", "Clean Class A", "2520 Punch" }, 1);
    tubeSwap.addItemList ({ "12AX7", "12AU7", "Clean Triode" }, 1);
    for (auto* combo : { &compressorMode, &trackCount, &quality, &micCharacter, &preampMode, &tubeSwap }) addAndMakeVisible (*combo);
    compressorModeAttachment = std::make_unique<ComboAttachment> (auraProcessorRef.apvts, "compressorMode", compressorMode);
    trackCountAttachment = std::make_unique<ComboAttachment> (auraProcessorRef.apvts, "trackCount", trackCount);
    qualityAttachment = std::make_unique<ComboAttachment> (auraProcessorRef.apvts, "quality", quality);
    micCharacterAttachment = std::make_unique<ComboAttachment> (auraProcessorRef.apvts, "micCharacter", micCharacter);
    preampModeAttachment = std::make_unique<ComboAttachment> (auraProcessorRef.apvts, "preampMode", preampMode);
    tubeSwapAttachment = std::make_unique<ComboAttachment> (auraProcessorRef.apvts, "tubeSwap", tubeSwap);

    for (int i = 0; i < auraProcessorRef.getNumPrograms(); ++i) presets.addItem (auraProcessorRef.getProgramName (i), i + 1);
    presets.setSelectedItemIndex (auraProcessorRef.getCurrentProgram(), juce::dontSendNotification);
    presets.onChange = [this] { auraProcessorRef.setCurrentProgram (presets.getSelectedItemIndex()); };
    addAndMakeVisible (presets);
    for (auto* meter : { &inputMeter, &outputMeter, &grMeter, &limiterMeter }) addAndMakeVisible (*meter);
    addAndMakeVisible (preampTube);
    addAndMakeVisible (driverTube);
    startTimerHz (60);
}

StadiumAuraAudioProcessorEditor::~StadiumAuraAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void StadiumAuraAudioProcessorEditor::addKnob (PremiumKnob& knob, const char* id,
                                                std::unique_ptr<SliderAttachment>& attachment)
{
    addAndMakeVisible (knob);
    attachment = std::make_unique<SliderAttachment> (auraProcessorRef.apvts, id, knob);
}

namespace
{
void drawRackPanel (juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& title)
{
    juce::ColourGradient panel (juce::Colour (0xff22211e), bounds.getX(), bounds.getY(),
                                juce::Colour (0xff0b0d0e), bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill (panel);
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (juce::Colour (0xff5f4b31));
    g.drawRoundedRectangle (bounds, 6.0f, 1.2f);
    g.setColour (juce::Colour (0xff050607));
    g.drawRoundedRectangle (bounds.reduced (3.0f), 4.0f, 1.0f);

    auto titleArea = bounds.removeFromTop (31.0f).reduced (5.0f, 3.0f);
    g.setColour (juce::Colour (0xff151514));
    g.fillRoundedRectangle (titleArea, 4.0f);
    g.setColour (juce::Colour (0xff6a5232));
    g.drawRoundedRectangle (titleArea, 4.0f, 1.0f);
    g.setColour (juce::Colour (0xffe5b965));
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText (title, titleArea, juce::Justification::centred);
}

void drawScrew (juce::Graphics& g, juce::Point<float> centre)
{
    g.setColour (juce::Colour (0xff080909));
    g.fillEllipse ({ centre.x - 6.0f, centre.y - 6.0f, 12.0f, 12.0f });
    g.setColour (juce::Colour (0xff756044));
    g.drawEllipse ({ centre.x - 5.0f, centre.y - 5.0f, 10.0f, 10.0f }, 1.0f);
    g.drawLine (centre.x - 3.0f, centre.y - 3.0f, centre.x + 3.0f, centre.y + 3.0f, 1.2f);
}
}

void TubeGlowComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (5.0f);
    auto label = bounds.removeFromBottom (27.0f);
    g.setColour (juce::Colour (0xff080909));
    g.fillRoundedRectangle (bounds, bounds.getWidth() * 0.32f);
    g.setColour (juce::Colour (0xff6d431e));
    g.drawRoundedRectangle (bounds, bounds.getWidth() * 0.32f, 1.5f);

    auto glass = bounds.reduced (bounds.getWidth() * 0.19f, 10.0f);
    const auto glowAlpha = juce::jlimit (0.16f, 0.95f, 0.18f + activity * 0.77f);
    juce::ColourGradient glow (juce::Colour::fromFloatRGBA (1.0f, 0.32f, 0.03f, glowAlpha), glass.getCentreX(), glass.getBottom(),
                               juce::Colour (0x11200d00), glass.getCentreX(), glass.getY(), true);
    g.setGradientFill (glow);
    g.fillRoundedRectangle (glass.expanded (8.0f, 2.0f), glass.getWidth() * 0.35f);
    juce::ColourGradient glassGradient (juce::Colour (0x99514025), glass.getX(), glass.getY(),
                                        juce::Colour (0x3320100a), glass.getRight(), glass.getBottom(), false);
    g.setGradientFill (glassGradient);
    g.fillRoundedRectangle (glass, glass.getWidth() * 0.42f);
    g.setColour (juce::Colour (0x88f4c18a));
    g.drawRoundedRectangle (glass, glass.getWidth() * 0.42f, 1.2f);

    g.setColour (juce::Colour::fromFloatRGBA (1.0f, 0.34f, 0.04f, 0.55f + activity * 0.45f));
    for (int wire = 0; wire < 3; ++wire)
    {
        const auto x = glass.getX() + glass.getWidth() * (0.32f + wire * 0.18f);
        g.drawLine (x, glass.getY() + 32.0f, x, glass.getBottom() - 24.0f, 1.3f);
    }
    for (int row = 0; row < 3; ++row)
    {
        const auto y = glass.getY() + glass.getHeight() * (0.38f + row * 0.18f);
        g.drawLine (glass.getX() + 8.0f, y, glass.getRight() - 8.0f, y, 1.2f);
    }
    g.setColour (juce::Colour (0xffffc25c));
    g.fillEllipse (glass.getCentreX() - 4.0f, glass.getCentreY() - 4.0f, 8.0f, 8.0f);

    g.setColour (juce::Colour (0xff100e0a));
    g.fillRoundedRectangle (label, 3.0f);
    g.setColour (juce::Colour (0xffc89449));
    g.drawRoundedRectangle (label, 3.0f, 1.0f);
    g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    g.drawText (model, label, juce::Justification::centred);
}

void StadiumAuraAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient background (juce::Colour (0xff1b1b19), 0.0f, 0.0f,
                                     juce::Colour (0xff050607), 0.0f, static_cast<float> (getHeight()), false);
    g.setGradientFill (background);
    g.fillAll();

    for (int y = 6; y < getHeight(); y += 4)
    {
        g.setColour (juce::Colour (0x09000000));
        g.drawHorizontalLine (y, 0.0f, static_cast<float> (getWidth()));
    }

    auto frame = getLocalBounds().toFloat().reduced (8.0f);
    g.setColour (juce::Colour (0xff0a0b0b));
    g.fillRoundedRectangle (frame, 8.0f);
    g.setColour (juce::Colour (0xff8c6a40));
    g.drawRoundedRectangle (frame, 8.0f, 1.3f);
    drawScrew (g, frame.getTopLeft() + juce::Point<float> (16.0f, 16.0f));
    drawScrew (g, frame.getTopRight() + juce::Point<float> (-16.0f, 16.0f));
    drawScrew (g, frame.getBottomLeft() + juce::Point<float> (16.0f, -16.0f));
    drawScrew (g, frame.getBottomRight() + juce::Point<float> (-16.0f, -16.0f));

    auto content = getLocalBounds().toFloat().reduced (65.0f, 16.0f);
    auto presetBar = content.removeFromBottom (48.0f);
    auto utility = content.removeFromBottom (150.0f).reduced (0.0f, 5.0f);
    auto header = content.removeFromTop (76.0f);
    auto main = content.reduced (0.0f, 4.0f);
    const auto leftWidth = main.getWidth() * 0.32f;
    const auto centreWidth = main.getWidth() * 0.38f;
    drawRackPanel (g, main.removeFromLeft (leftWidth).reduced (3.0f), "INPUT / TONE");
    main.removeFromLeft (3.0f);
    main.removeFromLeft (centreWidth);
    drawRackPanel (g, main.reduced (3.0f), "DYNAMICS  /  STEREO OUTPUT");
    drawRackPanel (g, utility, "SIGNAL / UTILITY");

    g.setColour (juce::Colour (0xffeed2a0));
    g.setFont (juce::FontOptions (43.0f, juce::Font::bold));
    g.drawText ("Stadium Aura", header, juce::Justification::centred);
    g.setColour (juce::Colour (0xffd4a34f));
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("PREMIUM ANALOG STAGE & DYNAMICS PROCESSOR", header.withTrimmedTop (50.0f), juce::Justification::centredTop);

    const juce::StringArray stages { "MIC", "PRE", "COMP", "HARMONICS", "SUM", "MASTER", "OUTPUT" };
    auto flow = header.withTrimmedTop (63.0f).reduced (header.getWidth() * 0.22f, 0.0f);
    const auto stageWidth = flow.getWidth() / stages.size();
    for (int i = 0; i < stages.size(); ++i)
    {
        auto stage = flow.removeFromLeft (stageWidth);
        g.setColour (juce::Colour (0xffd4a34f));
        g.fillEllipse (stage.getX() + 2.0f, stage.getCentreY() - 2.0f, 4.0f, 4.0f);
        g.setColour (juce::Colour (0xffad9270));
        g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
        g.drawText (stages[i], stage.reduced (8.0f, 0.0f), juce::Justification::centredLeft);
        if (i < stages.size() - 1)
        {
            g.setColour (juce::Colour (0xff59472e));
            g.drawLine (stage.getRight() - 5.0f, stage.getCentreY(), stage.getRight() + 3.0f, stage.getCentreY(), 1.0f);
        }
    }

    g.setColour (juce::Colour (0xff0a0b0b));
    g.fillRoundedRectangle (presetBar, 5.0f);
    g.setColour (juce::Colour (0xff6b5031));
    g.drawRoundedRectangle (presetBar, 5.0f, 1.0f);
    g.setColour (juce::Colour (0xffc49a54));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText ("PRESET", presetBar.removeFromLeft (100.0f), juce::Justification::centredRight);
}

void StadiumAuraAudioProcessorEditor::resized()
{
    auto content = getLocalBounds().reduced (65, 16);
    auto presetBar = content.removeFromBottom (48);
    auto utility = content.removeFromBottom (150).reduced (3, 8);
    content.removeFromTop (76);
    auto main = content.reduced (3, 7);

    auto left = main.removeFromLeft (juce::roundToInt (main.getWidth() * 0.32f)).reduced (10, 37);
    auto centre = main.removeFromLeft (juce::roundToInt (main.getWidth() * 0.56f)).reduced (4, 4);
    auto right = main.reduced (10, 37);

    auto selectors = left.removeFromTop (82);
    micCharacter.setBounds (selectors.removeFromTop (26).reduced (4, 1));
    preampMode.setBounds (selectors.removeFromTop (26).reduced (4, 1));
    tubeSwap.setBounds (selectors.removeFromTop (26).reduced (4, 1));
    auto leftTop = left.removeFromTop (left.getHeight() / 2);
    input.setBounds (leftTop.removeFromLeft (leftTop.getWidth() / 3).reduced (2));
    tube.setBounds (leftTop.removeFromLeft (leftTop.getWidth() / 2).reduced (2));
    saturation.setBounds (leftTop.reduced (2));
    auto leftBottom = left;
    const auto leftCell = leftBottom.getWidth() / 3;
    transformer.setBounds (leftBottom.removeFromLeft (leftCell).reduced (2));
    summing.setBounds (leftBottom.removeFromLeft (leftCell).reduced (2));
    glue.setBounds (leftBottom.reduced (2));

    auto tubeWidth = juce::jmax (72, centre.getWidth() / 5);
    preampTube.setBounds (centre.removeFromLeft (tubeWidth).reduced (2, 18));
    driverTube.setBounds (centre.removeFromRight (tubeWidth).reduced (2, 18));
    auto auraArea = centre;
    auto trackArea = auraArea.removeFromBottom (48);
    aura.setBounds (auraArea.reduced (2));
    trackCount.setBounds (trackArea.reduced (18, 8));

    auto dynamics = right.removeFromLeft (right.getWidth() * 3 / 5);
    auto stereo = right;
    compressorMode.setBounds (dynamics.removeFromTop (38).reduced (4));
    auto dynTop = dynamics.removeFromTop (dynamics.getHeight() / 2);
    compression.setBounds (dynTop.removeFromLeft (dynTop.getWidth() / 2).reduced (2));
    makeup.setBounds (dynTop.reduced (2));
    bias.setBounds (dynamics.removeFromLeft (dynamics.getWidth() / 2).reduced (2));
    dim.setBounds (dynamics.reduced (14, dynamics.getHeight() / 3));

    auto stereoTop = stereo.removeFromTop (stereo.getHeight() / 2);
    mix.setBounds (stereoTop.removeFromLeft (stereoTop.getWidth() / 2).reduced (2));
    width.setBounds (stereoTop.reduced (2));
    auto stereoBottom = stereo;
    const auto stereoCell = stereoBottom.getWidth() / 3;
    tone.setBounds (stereoBottom.removeFromLeft (stereoCell).reduced (2));
    output.setBounds (stereoBottom.removeFromLeft (stereoCell).reduced (2));
    ceiling.setBounds (stereoBottom.reduced (2));

    inputMeter.setBounds (utility.removeFromLeft (90).reduced (8, 18));
    grMeter.setBounds (utility.removeFromLeft (utility.getWidth() / 4).reduced (8, 28));
    auto utilityControls = utility.removeFromLeft (utility.getWidth() * 2 / 5).reduced (8, 22);
    auto buttons = utilityControls.removeFromTop (45);
    limiter.setBounds (buttons.removeFromLeft (buttons.getWidth() / 3).reduced (4));
    bypass.setBounds (buttons.removeFromLeft (buttons.getWidth() / 2).reduced (4));
    mono.setBounds (buttons.reduced (4));
    quality.setBounds (utilityControls.reduced (4, 12));
    limiterMeter.setBounds (utility.removeFromLeft (utility.getWidth() * 2 / 3).reduced (8, 28));
    outputMeter.setBounds (utility.reduced (8, 18));

    presetBar.removeFromLeft (112);
    presets.setBounds (presetBar.removeFromLeft (330).reduced (5, 9));
}

void StadiumAuraAudioProcessorEditor::timerCallback()
{
    inputMeter.setTarget (auraProcessorRef.inputMeter.load (std::memory_order_relaxed));
    outputMeter.setTarget (auraProcessorRef.outputMeter.load (std::memory_order_relaxed));
    grMeter.setTarget (auraProcessorRef.gainReductionMeter.load (std::memory_order_relaxed));
    limiterMeter.setTarget (auraProcessorRef.limiterReductionMeter.load (std::memory_order_relaxed));
    const auto tubeLevel = auraProcessorRef.tubeActivityMeter.load (std::memory_order_relaxed);
    const auto drive = auraProcessorRef.apvts.getRawParameterValue ("tubeDrive")->load() * 0.01f;
    const auto saturationValue = auraProcessorRef.apvts.getRawParameterValue ("saturation")->load() * 0.01f;
    const auto activity = juce::jlimit (0.0f, 1.0f, tubeLevel * 0.65f + drive * 0.25f + saturationValue * 0.10f);
    preampTube.setActivity (activity);
    driverTube.setActivity (activity * 0.88f);
}
