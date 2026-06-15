#include "PluginEditor.h"

namespace
{
void drawScrew (juce::Graphics& g, juce::Point<float> centre)
{
    g.setColour (juce::Colour (0xff080909));
    g.fillEllipse ({ centre.x - 6.0f, centre.y - 6.0f, 12.0f, 12.0f });
    g.setColour (juce::Colour (0xff756044));
    g.drawEllipse ({ centre.x - 5.0f, centre.y - 5.0f, 10.0f, 10.0f }, 1.0f);
    g.drawLine (centre.x - 3.0f, centre.y - 3.0f, centre.x + 3.0f, centre.y + 3.0f, 1.2f);
}
}

StadiumAuraAudioProcessorEditor::StadiumAuraAudioProcessorEditor (StadiumAuraAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&lookAndFeel);
    setResizable (true, true);
    setResizeLimits (1120, 680, 1920, 1150);
    setSize (1440, 860);
    aura.setHeroStyle (true);

    for (auto* panel : { &sourcePanel, &preampPanel, &auraPanel, &dynamicsPanel, &masterPanel, &utilityPanel })
        addAndMakeVisible (*panel);

    const std::pair<juce::Slider*, const char*> sliders[] {
        { &input, "input" }, { &output, "output" }, { &correction, "micCorrectionAmount" },
        { &targetAmount, "micTargetAmount" }, { &badFreq, "badFrequencyTamer" },
        { &bodyProtect, "bodyProtection" }, { &airProtect, "airProtection" },
        { &preampDrive, "preampDrive" }, { &aura, "aura" }, { &tubeDrive, "tubeDrive" },
        { &tubeBias, "harmonicBias" }, { &tubeOutput, "tubeOutputDb" },
        { &threshold, "compThresholdDb" }, { &ratio, "compRatio" }, { &attack, "compAttackMs" },
        { &release, "compReleaseMs" }, { &compMakeup, "compMakeupDb" },
        { &bleed, "compBleedPercent" }, { &scHpf, "compSidechainHpfHz" },
        { &saturation, "saturation" }, { &transformer, "transformer" },
        { &consoleDensity, "consoleDensity" }, { &glue, "glue" }, { &tone, "tone" },
        { &width, "width" }, { &mix, "mix" }, { &ceiling, "ceiling" }
    };
    for (auto [slider, id] : sliders) attachSlider (*slider, id);

    attachButton (hardwareSafe, "hardwareSafeMode"); attachButton (compressorEnable, "compressorEnable");
    attachButton (limiter, "limiter"); attachButton (bypass, "bypass");
    attachButton (mono, "monoCheck"); attachButton (dim, "dim");

    attachCombo (sourceMic, "sourceMicMode", { "Unknown / Auto", "Dynamic General", "Condenser General",
        "Ribbon General", "57-Style Dynamic", "7B-Style Dynamic", "C80-Style Condenser", "Bright Condenser",
        "Dark Condenser", "Warm Tube Mic", "Flat / Measurement" });
    attachCombo (targetMic, "targetMicMode", { "67 Vintage Smooth", "C12 Open Air", "251 Classic Silk",
        "87 Modern Balanced", "47 Velvet Tube", "251E Silky Presence", "800G Air Pop" });
    attachCombo (preampMode, "preampMode", { "73 Vintage Iron", "API Punch", "Avalon Clean" });
    attachCombo (tubeType, "tubeType", { "Clean Triode", "Warm Triode", "Hot Triode", "Vintage Pentode", "Big Bottle", "Cream Opto Tube" });
    attachCombo (compressorMode, "compressorMode", { "Smooth Opto", "Fast 76", "Kid670 Vari-Mu" });
    attachCombo (compTiming, "compTimingMode", { "Manual", "Fixed", "Fixed / Manual" });
    attachCombo (vuMode, "vuMeterMode", { "Input", "Gain Reduction", "Output" });
    attachCombo (consoleMode, "consoleMode", { "Clean Console", "Vintage Desk", "Modern Punch", "Tube Console" });
    attachCombo (trackCount, "trackCount", { "1 Track", "8 Tracks", "16 Tracks", "24 Tracks", "32 Tracks" });
    attachCombo (quality, "quality", { "Eco", "Normal", "High", "Ultra" });

    for (auto* meter : { static_cast<juce::Component*> (&inputMeter), static_cast<juce::Component*> (&outputMeter),
                         static_cast<juce::Component*> (&limiterMeter), static_cast<juce::Component*> (&vuMeter),
                         static_cast<juce::Component*> (&tubeChamber), static_cast<juce::Component*> (&compactEq) })
        addAndMakeVisible (*meter);

    for (int i = 0; i < static_cast<int> (routing.size()); ++i)
    {
        addAndMakeVisible (routing[static_cast<size_t> (i)]);
        routing[static_cast<size_t> (i)].onClick = [this, i] { setFocusedRoute (i); };
    }
    setFocusedRoute (0);

    for (int i = 0; i < processorRef.getNumPrograms(); ++i) presets.addItem (processorRef.getProgramName (i), i + 1);
    presets.setSelectedItemIndex (processorRef.getCurrentProgram(), juce::dontSendNotification);
    presets.onChange = [this] { processorRef.setCurrentProgram (presets.getSelectedItemIndex()); };
    addAndMakeVisible (presets);

    sweetZone.setJustificationType (juce::Justification::centred);
    sweetZone.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    sweetZone.setColour (juce::Label::textColourId, juce::Colour (0xfff0bd63));
    addAndMakeVisible (sweetZone);
    startTimerHz (60);
}

StadiumAuraAudioProcessorEditor::~StadiumAuraAudioProcessorEditor() { setLookAndFeel (nullptr); }

void StadiumAuraAudioProcessorEditor::attachSlider (juce::Slider& slider, const char* id)
{
    addAndMakeVisible (slider);
    sliderAttachments.push_back (std::make_unique<SliderAttachment> (processorRef.apvts, id, slider));
}

void StadiumAuraAudioProcessorEditor::attachButton (juce::Button& button, const char* id)
{
    addAndMakeVisible (button);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processorRef.apvts, id, button));
}

void StadiumAuraAudioProcessorEditor::attachCombo (juce::ComboBox& combo, const char* id, const juce::StringArray& items)
{
    combo.addItemList (items, 1);
    addAndMakeVisible (combo);
    comboAttachments.push_back (std::make_unique<ComboAttachment> (processorRef.apvts, id, combo));
}

void StadiumAuraAudioProcessorEditor::setFocusedRoute (int index)
{
    focusedRoute = juce::jlimit (0, static_cast<int> (routing.size()) - 1, index);
    for (int i = 0; i < static_cast<int> (routing.size()); ++i)
        routing[static_cast<size_t> (i)].setToggleState (i == focusedRoute, juce::dontSendNotification);
    repaint();
}

void StadiumAuraAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient background (juce::Colour (0xff20201e), 0.0f, 0.0f,
                                     juce::Colour (0xff050607), 0.0f, static_cast<float> (getHeight()), false);
    g.setGradientFill (background);
    g.fillAll();
    for (int y = 5; y < getHeight(); y += 4)
    {
        g.setColour (juce::Colour (0x09000000));
        g.drawHorizontalLine (y, 0.0f, static_cast<float> (getWidth()));
    }
    auto frame = getLocalBounds().toFloat().reduced (8.0f);
    g.setColour (juce::Colour (0xff89653b));
    g.drawRoundedRectangle (frame, 8.0f, 1.4f);
    drawScrew (g, frame.getTopLeft() + juce::Point<float> (16.0f, 16.0f));
    drawScrew (g, frame.getTopRight() + juce::Point<float> (-16.0f, 16.0f));
    drawScrew (g, frame.getBottomLeft() + juce::Point<float> (16.0f, -16.0f));
    drawScrew (g, frame.getBottomRight() + juce::Point<float> (-16.0f, -16.0f));

    auto header = getLocalBounds().toFloat().reduced (42.0f, 15.0f).removeFromTop (92.0f);
    g.setColour (juce::Colour (0xfff0d2a0));
    g.setFont (juce::FontOptions (38.0f, juce::Font::bold));
    g.drawText ("STADIUM AURA", header.removeFromTop (48.0f), juce::Justification::centred);
    g.setColour (juce::Colour (0xffd39d48));
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText ("ANALOG SOUL. DIGITAL INTELLIGENCE.", header.removeFromTop (17.0f), juce::Justification::centred);
    g.setColour (juce::Colour (0xff87755d));
    g.setFont (juce::FontOptions (8.5f));
    g.drawText ("ORIGINAL PREMIUM ANALOG CREATIVE PROCESSOR", header, juce::Justification::centredTop);
}

void StadiumAuraAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (42, 15);
    auto header = area.removeFromTop (96);
    auto presetArea = header.removeFromLeft (250).reduced (4, 28);
    presets.setBounds (presetArea);
    auto routeArea = header.removeFromRight (680).reduced (4, 28);
    const auto routeWidth = routeArea.getWidth() / static_cast<int> (routing.size());
    for (auto& button : routing) button.setBounds (routeArea.removeFromLeft (routeWidth).reduced (2));

    auto utility = area.removeFromBottom (155).reduced (0, 5);
    utilityPanel.setBounds (utility);
    auto main = area.reduced (0, 4);
    const auto gap = 7;
    auto left = main.removeFromLeft (juce::roundToInt (main.getWidth() * 0.27f));
    main.removeFromLeft (gap);
    auto centre = main.removeFromLeft (juce::roundToInt (main.getWidth() * 0.31f));
    main.removeFromLeft (gap);
    auto right = main;

    auto source = left.removeFromTop (juce::roundToInt (left.getHeight() * 0.58f));
    left.removeFromTop (gap);
    sourcePanel.setBounds (source); preampPanel.setBounds (left);
    auto sourceContent = source.reduced (10, 34);
    sourceMic.setBounds (sourceContent.removeFromTop (27)); sourceContent.removeFromTop (4);
    targetMic.setBounds (sourceContent.removeFromTop (27)); sourceContent.removeFromTop (3);
    hardwareSafe.setBounds (sourceContent.removeFromBottom (27).reduced (30, 1));
    auto sourceTop = sourceContent.removeFromTop (sourceContent.getHeight() / 2);
    correction.setBounds (sourceTop.removeFromLeft (sourceTop.getWidth() / 2).reduced (2));
    targetAmount.setBounds (sourceTop.reduced (2));
    const auto sourceCell = sourceContent.getWidth() / 3;
    badFreq.setBounds (sourceContent.removeFromLeft (sourceCell).reduced (2));
    bodyProtect.setBounds (sourceContent.removeFromLeft (sourceCell).reduced (2));
    airProtect.setBounds (sourceContent.reduced (2));
    auto preContent = left.reduced (10, 34);
    preampMode.setBounds (preContent.removeFromTop (29));
    preampDrive.setBounds (preContent.reduced (preContent.getWidth() / 4, 2));

    auraPanel.setBounds (centre);
    auto auraContent = centre.reduced (10, 34);
    auto chamber = auraContent.removeFromBottom (juce::roundToInt (auraContent.getHeight() * 0.43f));
    auto hero = auraContent;
    sweetZone.setBounds (hero.removeFromBottom (26));
    aura.setBounds (hero.reduced (hero.getWidth() / 7, 0));
    auto tubeControls = chamber.removeFromBottom (115);
    tubeChamber.setBounds (chamber.reduced (30, 3));
    tubeType.setBounds (tubeControls.removeFromTop (27).reduced (35, 1));
    const auto tubeCell = tubeControls.getWidth() / 3;
    tubeDrive.setBounds (tubeControls.removeFromLeft (tubeCell).reduced (2));
    tubeBias.setBounds (tubeControls.removeFromLeft (tubeCell).reduced (2));
    tubeOutput.setBounds (tubeControls.reduced (2));

    auto dynamics = right.removeFromTop (juce::roundToInt (right.getHeight() * 0.64f));
    right.removeFromTop (gap);
    dynamicsPanel.setBounds (dynamics); masterPanel.setBounds (right);
    auto dyn = dynamics.reduced (10, 34);
    auto dynTop = dyn.removeFromTop (29);
    compressorMode.setBounds (dynTop.removeFromLeft (dynTop.getWidth() / 3).reduced (2));
    compTiming.setBounds (dynTop.removeFromLeft (dynTop.getWidth() / 2).reduced (2));
    vuMode.setBounds (dynTop.reduced (2));
    dyn.removeFromTop (4);
    auto vu = dyn.removeFromRight (juce::roundToInt (dyn.getWidth() * 0.42f));
    compressorEnable.setBounds (vu.removeFromBottom (28).reduced (18, 1));
    vuMeter.setBounds (vu.reduced (5));
    auto row1 = dyn.removeFromTop (dyn.getHeight() / 2);
    const auto dynCell = dyn.getWidth() / 4;
    threshold.setBounds (row1.removeFromLeft (dynCell).reduced (2));
    ratio.setBounds (row1.removeFromLeft (dynCell).reduced (2));
    attack.setBounds (row1.removeFromLeft (dynCell).reduced (2));
    release.setBounds (row1.reduced (2));
    const auto dynCell2 = dyn.getWidth() / 3;
    compMakeup.setBounds (dyn.removeFromLeft (dynCell2).reduced (2));
    bleed.setBounds (dyn.removeFromLeft (dynCell2).reduced (2));
    scHpf.setBounds (dyn.reduced (2));

    auto master = right.reduced (10, 34);
    auto masterTop = master.removeFromTop (29);
    consoleMode.setBounds (masterTop.removeFromLeft (masterTop.getWidth() / 2).reduced (2));
    trackCount.setBounds (masterTop.reduced (2));
    const auto masterCell = master.getWidth() / 7;
    saturation.setBounds (master.removeFromLeft (masterCell).reduced (1));
    transformer.setBounds (master.removeFromLeft (masterCell).reduced (1));
    consoleDensity.setBounds (master.removeFromLeft (masterCell).reduced (1));
    glue.setBounds (master.removeFromLeft (masterCell).reduced (1));
    mix.setBounds (master.removeFromLeft (masterCell).reduced (1));
    width.setBounds (master.removeFromLeft (masterCell).reduced (1));
    ceiling.setBounds (master.reduced (1));

    auto util = utility.reduced (10, 34);
    input.setBounds (util.removeFromLeft (72));
    inputMeter.setBounds (util.removeFromLeft (66).reduced (5));
    compactEq.setBounds (util.removeFromLeft (220).reduced (8, 16));
    tone.setBounds (util.removeFromLeft (95).reduced (2));
    auto buttons = util.removeFromLeft (280).reduced (4, 18);
    auto buttonRow = buttons.removeFromTop (36);
    bypass.setBounds (buttonRow.removeFromLeft (buttonRow.getWidth() / 3).reduced (2));
    mono.setBounds (buttonRow.removeFromLeft (buttonRow.getWidth() / 2).reduced (2));
    dim.setBounds (buttonRow.reduced (2));
    buttonRow = buttons.removeFromTop (36);
    limiter.setBounds (buttonRow.removeFromLeft (buttonRow.getWidth() / 2).reduced (2));
    quality.setBounds (buttonRow.reduced (2));
    limiterMeter.setBounds (util.removeFromLeft (160).reduced (8, 22));
    outputMeter.setBounds (util.removeFromRight (66).reduced (5));
    output.setBounds (util.removeFromRight (72));
}

void StadiumAuraAudioProcessorEditor::timerCallback()
{
    const auto in = processorRef.inputMeter.load (std::memory_order_relaxed);
    const auto out = processorRef.outputMeter.load (std::memory_order_relaxed);
    const auto gr = processorRef.gainReductionMeter.load (std::memory_order_relaxed);
    const auto lim = processorRef.limiterReductionMeter.load (std::memory_order_relaxed);
    inputMeter.setTarget (in); outputMeter.setTarget (out); limiterMeter.setTarget (lim);
    const auto vuIndex = juce::jlimit (0, 2, static_cast<int> (processorRef.apvts.getRawParameterValue ("vuMeterMode")->load()));
    vuMeter.setTargets (in, gr, out, static_cast<VuMeterComponent::Mode> (vuIndex));
    const auto tubeLevel = processorRef.tubeActivityMeter.load (std::memory_order_relaxed);
    const auto drive = processorRef.apvts.getRawParameterValue ("tubeDrive")->load() * 0.01f;
    tubeChamber.setActivity (juce::jlimit (0.0f, 1.0f, tubeLevel * 0.72f + drive * 0.28f));
    compactEq.setTone (processorRef.apvts.getRawParameterValue ("tone")->load() * 0.01f);

    const auto hot = lim > 0.8f || gr > 10.0f || out > 0.94f || tubeLevel > 0.88f;
    const auto sweet = ! hot && in > 0.08f && (gr > 0.5f || drive > 0.18f) && out < 0.94f;
    sweetZone.setText (hot ? "SWEET ZONE: HOT" : (sweet ? "SWEET ZONE: SWEET" : "SWEET ZONE: LOW"),
                       juce::dontSendNotification);
    sweetZone.setColour (juce::Label::textColourId, hot ? juce::Colour (0xffff6b3a)
                                                        : (sweet ? juce::Colour (0xffffc15b) : juce::Colour (0xff8d806c)));
}
