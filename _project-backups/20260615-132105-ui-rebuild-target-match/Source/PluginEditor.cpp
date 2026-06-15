#include "PluginEditor.h"

namespace
{
void drawScrew (juce::Graphics& g, juce::Point<float> centre)
{
    g.setColour (juce::Colour (0xff080909));
    g.fillEllipse ({ centre.x - 6.0f, centre.y - 6.0f, 12.0f, 12.0f });
    g.setColour (juce::Colour (0xff9a7448));
    g.drawEllipse ({ centre.x - 5.0f, centre.y - 5.0f, 10.0f, 10.0f }, 1.0f);
    g.drawLine (centre.x - 3.0f, centre.y - 3.0f, centre.x + 3.0f, centre.y + 3.0f, 1.2f);
}

void styleNavTextButton (juce::TextButton& button)
{
    button.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1a1d21));
    button.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff8f6428));
    button.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffd8c7a4));
    button.setColour (juce::TextButton::textColourOnId, juce::Colour (0xff120f0b));
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

    logoTitle.setText ("STADIUM AURA", juce::dontSendNotification);
    logoTitle.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    logoTitle.setColour (juce::Label::textColourId, juce::Colour (0xfff0d2a0));
    logoSubtitle.setText ("ANALOG CREATIVE PROCESSOR", juce::dontSendNotification);
    logoSubtitle.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    logoSubtitle.setColour (juce::Label::textColourId, juce::Colour (0xffc58a3d));
    saBadge.setText ("SA", juce::dontSendNotification);
    saBadge.setJustificationType (juce::Justification::centred);
    saBadge.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    saBadge.setColour (juce::Label::textColourId, juce::Colour (0xff120f0b));
    saBadge.setColour (juce::Label::backgroundColourId, juce::Colour (0xffd59a48));
    presetCard.setJustificationType (juce::Justification::centred);
    presetCard.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    presetCard.setColour (juce::Label::textColourId, juce::Colour (0xffe8c98d));
    auraPercent.setJustificationType (juce::Justification::centred);
    auraPercent.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    auraPercent.setColour (juce::Label::textColourId, juce::Colour (0xffffd27a));
    for (auto* label : { &sweetZone, &sweetLow, &sweetHot })
    {
        label->setJustificationType (juce::Justification::centred);
        label->setFont (juce::FontOptions (10.0f, juce::Font::bold));
        addAndMakeVisible (*label);
    }
    sweetLow.setText ("LOW", juce::dontSendNotification);
    sweetHot.setText ("HOT", juce::dontSendNotification);
    for (auto* label : { &logoTitle, &logoSubtitle, &auraPercent, &presetCard, &saBadge })
        addAndMakeVisible (*label);

    for (auto* panel : { &matchingPanel, &micCharacterPanel, &preampPanel, &auraPanel,
                         &compressorPanel, &consolePanel, &eqPanel, &utilityPanel })
        addAndMakeVisible (*panel);

    const std::pair<juce::Slider*, const char*> sliders[] {
        { &inputFader, "input" }, { &outputFader, "output" }, { &correction, "micCorrectionAmount" },
        { &targetAmount, "micTargetAmount" }, { &badFreq, "badFrequencyTamer" },
        { &bodyProtect, "bodyProtection" }, { &airProtect, "airProtection" },
        { &micAmount, "makeup" }, { &deHarsh, "badFrequencyTamer" },
        { &preampDrive, "preampDrive" }, { &preampTone, "tone" }, { &preampOutput, "tubeOutputDb" },
        { &aura, "aura" }, { &tubeDriveSlider, "tubeDrive" }, { &tubeBias, "harmonicBias" },
        { &tubeOutput, "tubeOutputDb" }, { &threshold, "compThresholdDb" }, { &ratio, "compRatio" },
        { &attack, "compAttackMs" }, { &release, "compReleaseMs" }, { &compMakeup, "compMakeupDb" },
        { &bleed, "compBleedPercent" }, { &scHpf, "compSidechainHpfHz" },
        { &saturation, "saturation" }, { &transformer, "transformer" },
        { &consoleDensity, "consoleDensity" }, { &glue, "glue" }, { &width, "width" },
        { &mix, "mix" }, { &ceiling, "ceiling" }
    };
    for (auto [slider, id] : sliders) attachSlider (*slider, id);

    tubeDriveSlider.setSliderStyle (juce::Slider::LinearVertical);
    tubeDriveSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 18);
    tubeDriveSlider.setName ("TUBE DRIVE");
    tubeDriveSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffead9aa));

    attachButton (hardwareSafe, "hardwareSafeMode");
    attachButton (compressorEnable, "compressorEnable");
    attachButton (limiter, "limiter");
    attachButton (bypass, "bypass");
    attachButton (mono, "monoCheck");
    attachButton (dim, "dim");

    attachCombo (sourceMic, "sourceMicMode", { "Unknown / Auto", "Dynamic General", "Condenser General",
        "Ribbon General", "57-Style Dynamic", "7B-Style Dynamic", "C80-Style Condenser", "Bright Condenser",
        "Dark Condenser", "Warm Tube Mic", "Flat / Measurement" });
    attachCombo (targetMic, "targetMicMode", { "67 Vintage Smooth", "C12 Open Air", "251 Classic Silk",
        "87 Modern Balanced", "47 Velvet Tube", "251E Silky Presence", "800G Air Pop" });
    attachCombo (micCharacterMode, "micCharacter", { "67 Vintage Smooth", "C12 Open Air", "251 Classic",
        "87 Modern Balanced", "47 Velvet Tube", "251E Silky Air", "800G Air Pop" });
    attachCombo (preampMode, "preampMode", { "73 Vintage Iron", "API Punch", "Avalon Clean" });
    attachCombo (tubeType, "tubeType", { "Clean Triode", "Warm Triode", "Hot Triode", "Vintage Pentode", "Big Bottle", "Cream Opto Tube" });
    attachCombo (compressorMode, "compressorMode", { "Smooth Opto", "Fast 76", "Kid670 Vari-Mu" });
    attachCombo (compTiming, "compTimingMode", { "Manual", "Fixed", "Fixed / Manual" });
    attachCombo (vuMode, "vuMeterMode", { "Input", "Gain Reduction", "Output" });
    attachCombo (consoleMode, "consoleMode", { "Clean Console", "Vintage Desk", "Modern Punch", "Tube Console" });
    attachCombo (trackCount, "trackCount", { "1 Track", "8 Tracks", "16 Tracks", "24 Tracks", "32 Tracks" });
    attachCombo (quality, "quality", { "Eco", "Normal", "High", "Ultra" });

    for (auto* meter : { static_cast<juce::Component*> (&inputRms), static_cast<juce::Component*> (&outputRms),
                         static_cast<juce::Component*> (&limiterMeter), static_cast<juce::Component*> (&inputLr),
                         static_cast<juce::Component*> (&outputLr), static_cast<juce::Component*> (&vuMeter),
                         static_cast<juce::Component*> (&tubeChamber), static_cast<juce::Component*> (&compactEq) })
        addAndMakeVisible (*meter);

    for (auto& button : routing) addAndMakeVisible (button);
    for (int i = 0; i < static_cast<int> (routing.size()); ++i)
        routing[static_cast<size_t> (i)].onClick = [this, i] { setFocusedRoute (i); };
    setFocusedRoute (0);

    for (int i = 0; i < processorRef.getNumPrograms(); ++i)
        presets.addItem (processorRef.getProgramName (i), i + 1);
    presets.setSelectedItemIndex (processorRef.getCurrentProgram(), juce::dontSendNotification);
    presets.onChange = [this]
    {
        processorRef.setCurrentProgram (presets.getSelectedItemIndex());
        presetCard.setText (processorRef.getProgramName (processorRef.getCurrentProgram()), juce::dontSendNotification);
    };
    addAndMakeVisible (presets);

    for (auto* button : { static_cast<juce::Component*> (&abA), static_cast<juce::Component*> (&abB),
                          static_cast<juce::Component*> (&copyAb), static_cast<juce::Component*> (&analyzeSource),
                          static_cast<juce::Component*> (&proximity), static_cast<juce::Component*> (&hpfButton),
                          static_cast<juce::Component*> (&phaseInvert), static_cast<juce::Component*> (&compIn),
                          static_cast<juce::Component*> (&compOut), static_cast<juce::Component*> (&undoButton),
                          static_cast<juce::Component*> (&redoButton), static_cast<juce::Component*> (&favoriteButton) })
        addAndMakeVisible (*button);
    styleNavTextButton (abA);
    styleNavTextButton (abB);
    styleNavTextButton (copyAb);
    abA.setClickingTogglesState (true);
    abB.setClickingTogglesState (true);
    abA.setToggleState (true, juce::dontSendNotification);
    abA.onClick = [this] { if (abA.getToggleState()) restoreAbState (0); };
    abB.onClick = [this] { if (abB.getToggleState()) restoreAbState (1); };
    copyAb.onClick = [this] { captureAbState (activeAbSlot == 0 ? 1 : 0); };
    captureAbState (0);
    captureAbState (1);

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

void StadiumAuraAudioProcessorEditor::captureAbState (int slot)
{
    if (auto xml = processorRef.apvts.copyState().createXml())
        abSnapshots[static_cast<size_t> (slot)] = std::move (xml);
}

void StadiumAuraAudioProcessorEditor::restoreAbState (int slot)
{
    if (abSnapshots[static_cast<size_t> (slot)] != nullptr)
        processorRef.apvts.replaceState (juce::ValueTree::fromXml (*abSnapshots[static_cast<size_t> (slot)]));
    activeAbSlot = slot;
    abA.setToggleState (slot == 0, juce::dontSendNotification);
    abB.setToggleState (slot == 1, juce::dontSendNotification);
    presets.setSelectedItemIndex (processorRef.getCurrentProgram(), juce::dontSendNotification);
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
    juce::ColourGradient background (juce::Colour (0xff1f1d1b), 0.0f, 0.0f,
                                     juce::Colour (0xff050607), 0.0f, static_cast<float> (getHeight()), false);
    g.setGradientFill (background);
    g.fillAll();
    for (int y = 4; y < getHeight(); y += 3)
    {
        g.setColour (juce::Colour (0x07000000));
        g.drawHorizontalLine (y, 0.0f, static_cast<float> (getWidth()));
    }

    auto frame = getLocalBounds().toFloat().reduced (10.0f);
    g.setColour (juce::Colour (0xff5f4528));
    g.drawRoundedRectangle (frame, 10.0f, 2.0f);
    drawScrew (g, frame.getTopLeft() + juce::Point<float> (18.0f, 18.0f));
    drawScrew (g, frame.getTopRight() + juce::Point<float> (-18.0f, 18.0f));
    drawScrew (g, frame.getBottomLeft() + juce::Point<float> (18.0f, -18.0f));
    drawScrew (g, frame.getBottomRight() + juce::Point<float> (-18.0f, -18.0f));

    auto topBar = getLocalBounds().reduced (42, 14).removeFromTop (58).toFloat();
    g.setColour (juce::Colour (0xff121416));
    g.fillRoundedRectangle (topBar, 6.0f);
    g.setColour (juce::Colour (0xff6d512f));
    g.drawRoundedRectangle (topBar.reduced (0.5f), 6.0f, 1.0f);
}

void StadiumAuraAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (42, 14);
    auto topBar = bounds.removeFromTop (58);
    auto bottomStrip = bounds.removeFromBottom (118);
    auto lowerRack = bounds.removeFromBottom (132);
    auto main = bounds.reduced (0, 6);

    logoTitle.setBounds (topBar.removeFromLeft (220).reduced (4, 6));
    logoSubtitle.setBounds (logoTitle.getBounds().withTrimmedTop (logoTitle.getHeight() - 18));
    saBadge.setBounds (topBar.removeFromRight (34).reduced (6, 14));
    auto routeArea = topBar.removeFromRight (560).reduced (4, 10);
    const auto routeWidth = routeArea.getWidth() / static_cast<int> (routing.size());
    for (auto& button : routing) button.setBounds (routeArea.removeFromLeft (routeWidth).reduced (2));
    auto abArea = topBar.removeFromRight (110).reduced (4, 12);
    abA.setBounds (abArea.removeFromLeft (30).reduced (1));
    abB.setBounds (abArea.removeFromLeft (30).reduced (1));
    copyAb.setBounds (abArea.reduced (1));
    presets.setBounds (topBar.removeFromLeft (260).reduced (4, 12));

    auto leftCol = main.removeFromLeft (juce::roundToInt (main.getWidth() * 0.24f));
    main.removeFromLeft (6);
    auto centreLeft = main.removeFromLeft (juce::roundToInt (main.getWidth() * 0.20f));
    main.removeFromLeft (6);
    auto hero = main.removeFromLeft (juce::roundToInt (main.getWidth() * 0.34f));
    main.removeFromLeft (6);
    auto rightCol = main;

    inputRms.setBounds (leftCol.removeFromBottom (72).reduced (4, 0));
    auto matching = leftCol.removeFromTop (juce::roundToInt (leftCol.getHeight() * 0.58f));
    leftCol.removeFromTop (6);
    matchingPanel.setBounds (matching);
    micCharacterPanel.setBounds (leftCol);
    auto matchContent = matching.reduced (10, 34);
    sourceMic.setBounds (matchContent.removeFromTop (24));
    matchContent.removeFromTop (3);
    targetMic.setBounds (matchContent.removeFromTop (24));
    matchContent.removeFromTop (4);
    analyzeSource.setBounds (matchContent.removeFromBottom (24).reduced (20, 0));
    hardwareSafe.setBounds (matchContent.removeFromBottom (24).reduced (20, 0));
    auto row = matchContent.removeFromTop (matchContent.getHeight() / 2);
    correction.setBounds (row.removeFromLeft (row.getWidth() / 2).reduced (2));
    targetAmount.setBounds (row.reduced (2));
    const auto cell = matchContent.getWidth() / 3;
    badFreq.setBounds (matchContent.removeFromLeft (cell).reduced (2));
    bodyProtect.setBounds (matchContent.removeFromLeft (cell).reduced (2));
    airProtect.setBounds (matchContent.reduced (2));

    auto micContent = leftCol.reduced (10, 34);
    micCharacterMode.setBounds (micContent.removeFromTop (24));
    micContent.removeFromTop (4);
    auto micRow = micContent.removeFromTop (micContent.getHeight() / 2);
    micAmount.setBounds (micRow.removeFromLeft (micRow.getWidth() / 2).reduced (2));
    deHarsh.setBounds (micRow.reduced (2));
    proximity.setBounds (micContent.removeFromLeft (micContent.getWidth() / 2).reduced (2));
    hpfButton.setBounds (micContent.reduced (2));

    preampPanel.setBounds (centreLeft);
    auto pre = centreLeft.reduced (10, 34);
    preampMode.setBounds (pre.removeFromTop (24));
    pre.removeFromTop (4);
    auto preTop = pre.removeFromTop (pre.getHeight() / 2);
    preampDrive.setBounds (preTop.removeFromLeft (preTop.getWidth() / 2).reduced (2));
    preampTone.setBounds (preTop.reduced (2));
    preampOutput.setBounds (pre.removeFromLeft (pre.getWidth() / 2).reduced (2));
    phaseInvert.setBounds (pre.reduced (2));

    auraPanel.setBounds (hero);
    auto heroContent = hero.reduced (10, 34);
    auto tubeArea = heroContent.removeFromBottom (juce::roundToInt (heroContent.getHeight() * 0.42f));
    auto auraTop = heroContent;
    sweetZone.setBounds (auraTop.removeFromBottom (18));
    auto sweetRow = auraTop.removeFromBottom (18);
    sweetLow.setBounds (sweetRow.removeFromLeft (sweetRow.getWidth() / 3));
    sweetHot.setBounds (sweetRow.removeFromRight (sweetRow.getWidth() / 3));
    auraPercent.setBounds (auraTop.removeFromBottom (24));
    aura.setBounds (auraTop.reduced (auraTop.getWidth() / 8, 0));
    tubeChamber.setBounds (tubeArea.removeFromTop (tubeArea.getHeight() - 120).reduced (20, 2));
    tubeType.setBounds (tubeArea.removeFromTop (24).reduced (30, 0));
    auto tubeRow = tubeArea;
    tubeDriveSlider.setBounds (tubeRow.removeFromLeft (tubeRow.getWidth() / 3).reduced (4));
    tubeBias.setBounds (tubeRow.removeFromLeft (tubeRow.getWidth() / 2).reduced (2));
    tubeOutput.setBounds (tubeRow.reduced (2));

    compressorPanel.setBounds (rightCol.removeFromTop (juce::roundToInt (rightCol.getHeight() * 0.72f)));
    outputRms.setBounds (rightCol.removeFromBottom (72).reduced (4, 0));
    utilityPanel.setBounds (rightCol);
    auto comp = compressorPanel.getBounds().reduced (10, 34);
    auto compTop = comp.removeFromTop (24);
    compressorMode.setBounds (compTop.removeFromLeft (compTop.getWidth() / 2).reduced (2));
    vuMode.setBounds (compTop.reduced (2));
    comp.removeFromTop (4);
    auto vuArea = comp.removeFromRight (juce::roundToInt (comp.getWidth() * 0.42f));
    compressorEnable.setBounds (vuArea.removeFromBottom (24).reduced (10, 0));
    compIn.setBounds (vuArea.removeFromBottom (22).removeFromLeft (vuArea.getWidth() / 2).reduced (2));
    compOut.setBounds (vuArea.removeFromBottom (22).reduced (2));
    vuMeter.setBounds (vuArea.reduced (4));
    auto dynRow1 = comp.removeFromTop (comp.getHeight() / 2);
    const auto dynCell = comp.getWidth() / 4;
    threshold.setBounds (dynRow1.removeFromLeft (dynCell).reduced (2));
    ratio.setBounds (dynRow1.removeFromLeft (dynCell).reduced (2));
    attack.setBounds (dynRow1.removeFromLeft (dynCell).reduced (2));
    release.setBounds (dynRow1.reduced (2));
    const auto dynCell2 = comp.getWidth() / 3;
    compMakeup.setBounds (comp.removeFromLeft (dynCell2).reduced (2));
    bleed.setBounds (comp.removeFromLeft (dynCell2).reduced (2));
    scHpf.setBounds (comp.reduced (2));
    compTiming.setBounds (compressorPanel.getBounds().removeFromBottom (26).reduced (14, 2));

    auto util = utilityPanel.getBounds().reduced (10, 34);
    auto utilTop = util.removeFromTop (28);
    limiter.setBounds (utilTop.removeFromLeft (88).reduced (2));
    ceiling.setBounds (utilTop.removeFromLeft (120).reduced (2));
    limiterMeter.setBounds (util.reduced (2));

    auto rack = lowerRack.reduced (0, 4);
    consolePanel.setBounds (rack.removeFromLeft (juce::roundToInt (rack.getWidth() * 0.34f)).reduced (2));
    eqPanel.setBounds (rack.removeFromLeft (juce::roundToInt (rack.getWidth() * 0.42f)).reduced (2));
    auto rackRight = rack.reduced (2);
    auto console = consolePanel.getBounds().reduced (10, 34);
    auto consoleTop = console.removeFromTop (24);
    consoleMode.setBounds (consoleTop.removeFromLeft (consoleTop.getWidth() / 2).reduced (2));
    trackCount.setBounds (consoleTop.reduced (2));
    const auto rackCell = console.getWidth() / 4;
    saturation.setBounds (console.removeFromLeft (rackCell).reduced (2));
    transformer.setBounds (console.removeFromLeft (rackCell).reduced (2));
    consoleDensity.setBounds (console.removeFromLeft (rackCell).reduced (2));
    glue.setBounds (console.reduced (2));
    compactEq.setBounds (eqPanel.getBounds().reduced (12, 36));
    width.setBounds (rackRight.removeFromTop (rackRight.getHeight() / 2).reduced (2));
    mix.setBounds (rackRight.removeFromTop (rackRight.getHeight()).reduced (2));
    quality.setBounds (rackRight.removeFromBottom (26).reduced (2));

    auto strip = bottomStrip.reduced (0, 4);
    inputFader.setBounds (strip.removeFromLeft (220).reduced (4, 8));
    inputLr.setBounds (strip.removeFromLeft (90).reduced (2, 8));
    auto centreStrip = strip.removeFromLeft (strip.getWidth() / 2);
    bypass.setBounds (centreStrip.removeFromLeft (90).reduced (2, 20));
    mono.setBounds (centreStrip.removeFromLeft (80).reduced (2, 20));
    dim.setBounds (centreStrip.removeFromLeft (110).reduced (2, 20));
    presetCard.setBounds (centreStrip.removeFromLeft (180).reduced (2, 18));
    favoriteButton.setBounds (centreStrip.removeFromLeft (42).reduced (2, 22));
    undoButton.setBounds (centreStrip.removeFromLeft (52).reduced (2, 22));
    redoButton.setBounds (centreStrip.removeFromLeft (52).reduced (2, 22));
    outputLr.setBounds (strip.removeFromRight (90).reduced (2, 8));
    outputFader.setBounds (strip.removeFromRight (220).reduced (4, 8));
}

void StadiumAuraAudioProcessorEditor::timerCallback()
{
    const auto in = processorRef.inputMeter.load (std::memory_order_relaxed);
    const auto out = processorRef.outputMeter.load (std::memory_order_relaxed);
    const auto gr = processorRef.gainReductionMeter.load (std::memory_order_relaxed);
    const auto lim = processorRef.limiterReductionMeter.load (std::memory_order_relaxed);
    inputRms.setTarget (in);
    outputRms.setTarget (out);
    limiterMeter.setTarget (lim);
    inputLr.setTargets (in, in * 0.92f);
    outputLr.setTargets (out, out * 0.95f);

    const auto vuIndex = juce::jlimit (0, 2, static_cast<int> (processorRef.apvts.getRawParameterValue ("vuMeterMode")->load()));
    vuMeter.setTargets (in, gr, out, static_cast<VuMeterComponent::Mode> (vuIndex));

    const auto tubeLevel = processorRef.tubeActivityMeter.load (std::memory_order_relaxed);
    const auto drive = processorRef.apvts.getRawParameterValue ("tubeDrive")->load() * 0.01f;
    tubeChamber.setActivity (juce::jlimit (0.0f, 1.0f, tubeLevel * 0.72f + drive * 0.28f));
    compactEq.setTone (processorRef.apvts.getRawParameterValue ("tone")->load() * 0.01f);

    const auto auraValue = processorRef.apvts.getRawParameterValue ("aura")->load();
    auraPercent.setText (juce::String (auraValue, 1) + " %", juce::dontSendNotification);
    presetCard.setText (processorRef.getProgramName (processorRef.getCurrentProgram()), juce::dontSendNotification);

    const auto hot = lim > 0.8f || gr > 10.0f || out > 0.94f || tubeLevel > 0.88f;
    const auto sweet = ! hot && in > 0.08f && (gr > 0.5f || drive > 0.18f) && out < 0.94f;
    sweetZone.setText (sweet ? "SWEET" : " ", juce::dontSendNotification);
    sweetZone.setColour (juce::Label::textColourId, sweet ? juce::Colour (0xffffc15b) : juce::Colour (0x00000000));
    sweetLow.setColour (juce::Label::textColourId, ! sweet && ! hot ? juce::Colour (0xff8d806c) : juce::Colour (0xff4d463c));
    sweetHot.setColour (juce::Label::textColourId, hot ? juce::Colour (0xffff6b3a) : juce::Colour (0xff4d463c));
}
