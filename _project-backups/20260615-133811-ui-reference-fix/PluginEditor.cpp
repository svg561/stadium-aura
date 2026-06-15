#include "PluginEditor.h"
#include "ui/RackDrawing.h"

StadiumAuraAudioProcessorEditor::StadiumAuraAudioProcessorEditor (StadiumAuraAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&lookAndFeel);
    setResizable (true, true);
    setResizeLimits (1120, 680, 1920, 1150);
    setSize (1536, 1000);
    aura.setHeroStyle (true);

    logoTitle.setText ("STADIUM AURA", juce::dontSendNotification);
    logoTitle.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    logoTitle.setColour (juce::Label::textColourId, juce::Colour (0xfff0d2a0));
    logoTitle.setJustificationType (juce::Justification::centredLeft);
    logoSubtitle.setText ("ANALOG SOUL. DIGITAL INTELLIGENCE.", juce::dontSendNotification);
    logoSubtitle.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    logoSubtitle.setColour (juce::Label::textColourId, juce::Colour (0xffc58a3d));
    logoSubtitle.setJustificationType (juce::Justification::centredLeft);
    saBadge.setText ("SA", juce::dontSendNotification);
    saBadge.setJustificationType (juce::Justification::centred);
    saBadge.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    saBadge.setColour (juce::Label::textColourId, juce::Colour (0xff100c06));
    saBadge.setColour (juce::Label::backgroundColourId, juce::Colour (0xffd59a48));
    presetCard.setJustificationType (juce::Justification::centred);
    presetCard.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    presetCard.setColour (juce::Label::textColourId, juce::Colour (0xffe8c98d));
    presetCard.setColour (juce::Label::backgroundColourId, juce::Colour (0xff101418));
    auraPercent.setJustificationType (juce::Justification::centred);
    auraPercent.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    auraPercent.setColour (juce::Label::textColourId, juce::Colour (0xffffd27a));
    grLabel.setJustificationType (juce::Justification::centred);
    grLabel.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    grLabel.setColour (juce::Label::textColourId, juce::Colour (0xffd8b878));
    for (auto* label : { &sweetZone, &sweetLow, &sweetHot })
    {
        label->setJustificationType (juce::Justification::centred);
        label->setFont (juce::FontOptions (10.0f, juce::Font::bold));
        addAndMakeVisible (*label);
    }
    sweetLow.setText ("LOW", juce::dontSendNotification);
    sweetHot.setText ("HOT", juce::dontSendNotification);
    for (auto* label : { &logoTitle, &logoSubtitle, &auraPercent, &presetCard, &saBadge, &grLabel })
        addAndMakeVisible (*label);

    for (auto* panel : { &matchingPanel, &micPanel, &preampPanel, &heroPanel, &compPanel,
                         &satPanel, &xfmrPanel, &consolePanel, &gluePanel, &eqPanel,
                         &widthPanel, &limiterPanel })
        addAndMakeVisible (*panel);

    const std::pair<juce::Slider*, const char*> sliders[] {
        { &inputFader, "input" }, { &outputFader, "output" }, { &correction, "micCorrectionAmount" },
        { &targetAmount, "micTargetAmount" }, { &badFreq, "badFrequencyTamer" },
        { &bodyProtect, "bodyProtection" }, { &airProtect, "airProtection" },
        { &preampDrive, "preampDrive" }, { &preampTone, "tone" }, { &preampOutput, "tubeOutputDb" },
        { &aura, "aura" }, { &tubeDriveSlider, "tubeDrive" }, { &tubeBias, "harmonicBias" },
        { &threshold, "compThresholdDb" }, { &ratio, "compRatio" }, { &attack, "compAttackMs" },
        { &release, "compReleaseMs" }, { &compMakeup, "compMakeupDb" }, { &bleed, "compBleedPercent" },
        { &scHpf, "compSidechainHpfHz" }, { &saturation, "saturation" }, { &transformer, "transformer" },
        { &consoleDensity, "consoleDensity" }, { &glue, "glue" }, { &width, "width" },
        { &mix, "mix" }, { &ceiling, "ceiling" }
    };
    for (auto [slider, id] : sliders) attachSlider (*slider, id);

    tubeDriveSlider.setSliderStyle (juce::Slider::LinearVertical);
    tubeDriveSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 16);
    tubeDriveSlider.setName ("TUBE DRIVE");

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

    for (auto* c : { static_cast<juce::Component*> (&inputRms), static_cast<juce::Component*> (&outputRms),
                     static_cast<juce::Component*> (&limiterMeter), static_cast<juce::Component*> (&inputLr),
                     static_cast<juce::Component*> (&outputLr), static_cast<juce::Component*> (&vuMeter),
                     static_cast<juce::Component*> (&tubeChamber), static_cast<juce::Component*> (&eqDisplay),
                     static_cast<juce::Component*> (&auraRing) })
        addAndMakeVisible (*c);

    const char* routeTips[] {
        "Mic: focus Source Match and Mic Character. Correction/Target affect matching DSP.",
        "Pre: focus Preamp Architecture. Drive and tone shape preamp coloration.",
        "Comp: focus Dynamics. COMP ON toggles compressor DSP beside the VU meter.",
        "Harmonics: focus Tube, Saturation, and Transformer harmonic stages.",
        "Sum: focus Console/Summing and Bus Glue density.",
        "Master: focus Aura macro, EQ tone, width, and ceiling limiter.",
        "Output: focus input/output faders, meters, bypass, mono, and dim."
    };
    for (int i = 0; i < static_cast<int> (routing.size()); ++i)
    {
        routing[static_cast<size_t> (i)].setTooltip (routeTips[i]);
        routing[static_cast<size_t> (i)].onClick = [this, i] { setFocusedRoute (i); };
    }
    setFocusedRoute (0);

    for (int i = 0; i < processorRef.getNumPrograms(); ++i) presets.addItem (processorRef.getProgramName (i), i + 1);
    presets.setSelectedItemIndex (processorRef.getCurrentProgram(), juce::dontSendNotification);
    presets.onChange = [this]
    {
        processorRef.setCurrentProgram (presets.getSelectedItemIndex());
        presetCard.setText (processorRef.getProgramName (processorRef.getCurrentProgram()), juce::dontSendNotification);
    };
    addAndMakeVisible (presets);

    for (auto* b : { static_cast<juce::Component*> (&presetPrev), static_cast<juce::Component*> (&presetNext),
                     static_cast<juce::Component*> (&saveBtn), static_cast<juce::Component*> (&abA),
                     static_cast<juce::Component*> (&abB), static_cast<juce::Component*> (&undoTop),
                     static_cast<juce::Component*> (&redoTop), static_cast<juce::Component*> (&settingsBtn),
                     static_cast<juce::Component*> (&helpBtn), static_cast<juce::Component*> (&favoriteTop),
                     static_cast<juce::Component*> (&favoriteBottom), static_cast<juce::Component*> (&undoBottom),
                     static_cast<juce::Component*> (&redoBottom), static_cast<juce::Component*> (&analyzeSource),
                     static_cast<juce::Component*> (&deHarsh), static_cast<juce::Component*> (&proximity), static_cast<juce::Component*> (&hpfButton),
                     static_cast<juce::Component*> (&phaseInvert), static_cast<juce::Component*> (&compIn),
                     static_cast<juce::Component*> (&compOut) })
        addAndMakeVisible (*b);

    abA.setClickingTogglesState (true);
    abB.setClickingTogglesState (true);
    abA.setToggleState (true, juce::dontSendNotification);
    abA.onClick = [this] { if (abA.getToggleState()) restoreAbState (0); };
    abB.onClick = [this] { if (abB.getToggleState()) restoreAbState (1); };
    presetPrev.onClick = [this] { presets.setSelectedItemIndex (juce::jmax (0, presets.getSelectedItemIndex() - 1)); };
    presetNext.onClick = [this] { presets.setSelectedItemIndex (juce::jmin (processorRef.getNumPrograms() - 1, presets.getSelectedItemIndex() + 1)); };
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

void StadiumAuraAudioProcessorEditor::layoutKnobRow (juce::Rectangle<int>& area, std::initializer_list<juce::Slider*> knobs)
{
    const auto n = static_cast<int> (knobs.size());
    if (n <= 0) return;
    const auto w = area.getWidth() / n;
    for (auto* knob : knobs)
        knob->setBounds (area.removeFromLeft (w).reduced (1));
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
    applyRouteVisuals();
}

void StadiumAuraAudioProcessorEditor::applyRouteVisuals()
{
    auto setPanel = [] (RackModulePanel& panel, bool highlight, bool dimmed)
    {
        panel.setRouteVisualState (highlight, dimmed);
    };

    const auto route = focusedRoute;
    const auto dimUnless = [route] (int activeRoute) { return route != activeRoute; };

    setPanel (matchingPanel, route == 0, dimUnless (0));
    setPanel (micPanel, route == 0, dimUnless (0));
    setPanel (preampPanel, route == 1, dimUnless (1));
    setPanel (compPanel, route == 2, dimUnless (2));
    setPanel (satPanel, route == 3, dimUnless (3));
    setPanel (xfmrPanel, route == 3, dimUnless (3));
    setPanel (heroPanel, route == 3 || route == 5, route != 3 && route != 5);
    setPanel (consolePanel, route == 4, dimUnless (4));
    setPanel (gluePanel, route == 4, dimUnless (4));
    setPanel (eqPanel, route == 5, dimUnless (5));
    setPanel (widthPanel, route == 5, dimUnless (5));
    setPanel (limiterPanel, route == 5 || route == 6, route != 5 && route != 6);

    const auto outputFocus = route == 6;
    inputFader.setAlpha (outputFocus ? 1.0f : 0.88f);
    outputFader.setAlpha (outputFocus ? 1.0f : 0.88f);
    inputRms.setAlpha (outputFocus ? 1.0f : 0.75f);
    outputRms.setAlpha (outputFocus ? 1.0f : 0.75f);
    inputLr.setAlpha (outputFocus ? 1.0f : 0.75f);
    outputLr.setAlpha (outputFocus ? 1.0f : 0.75f);
}

void StadiumAuraAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto frame = getLocalBounds().toFloat().reduced (8.0f);
    RackDrawing::paintBrushedMetal (g, frame);
    g.setColour (juce::Colour (0xff5a4224));
    g.drawRoundedRectangle (frame, 8.0f, 2.0f);
    RackDrawing::paintScrews (g, frame, 14.0f);
    g.setColour (juce::Colour (0x18ffffff));
    g.drawHorizontalLine (juce::roundToInt (frame.getY() + 72.0f), frame.getX() + 20.0f, frame.getRight() - 20.0f);
    g.drawHorizontalLine (juce::roundToInt (frame.getBottom() - 112.0f), frame.getX() + 20.0f, frame.getRight() - 20.0f);
    g.drawHorizontalLine (juce::roundToInt (frame.getBottom() - 252.0f), frame.getX() + 20.0f, frame.getRight() - 20.0f);
}

void StadiumAuraAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (16, 10);
    auto topBar = bounds.removeFromTop (68);
    auto bottomStrip = bounds.removeFromBottom (108);
    auto lowerRack = bounds.removeFromBottom (132);
    auto main = bounds.reduced (0, 4);

    auto logoBlock = topBar.removeFromLeft (240).reduced (2, 4);
    logoTitle.setBounds (logoBlock.removeFromTop (30));
    logoSubtitle.setBounds (logoBlock);
    saBadge.setBounds (topBar.removeFromRight (34).reduced (6, 16));
    auto nav = topBar.removeFromRight (500).reduced (2, 10);
    const auto navW = nav.getWidth() / static_cast<int> (routing.size());
    for (auto& button : routing) button.setBounds (nav.removeFromLeft (navW).reduced (1));
    settingsBtn.setBounds (topBar.removeFromRight (28).reduced (2, 14));
    helpBtn.setBounds (topBar.removeFromRight (28).reduced (2, 14));
    redoTop.setBounds (topBar.removeFromRight (26).reduced (2, 14));
    undoTop.setBounds (topBar.removeFromRight (26).reduced (2, 14));
    abB.setBounds (topBar.removeFromRight (24).reduced (2, 14));
    abA.setBounds (topBar.removeFromRight (24).reduced (2, 14));
    saveBtn.setBounds (topBar.removeFromRight (26).reduced (2, 14));
    favoriteTop.setBounds (topBar.removeFromRight (26).reduced (2, 14));
    presetNext.setBounds (topBar.removeFromRight (24).reduced (2, 14));
    presetPrev.setBounds (topBar.removeFromRight (24).reduced (2, 14));
    presetCard.setBounds (topBar.removeFromLeft (juce::jmax (180, topBar.getWidth() / 2)).reduced (4, 12));
    presets.setBounds (topBar.reduced (4, 12));

    auto leftMatch = main.removeFromLeft (juce::roundToInt (main.getWidth() * 0.17f));
    auto inMeterCol = main.removeFromLeft (56);
    auto micCol = main.removeFromLeft (juce::roundToInt (main.getWidth() * 0.14f));
    auto preCol = main.removeFromLeft (juce::roundToInt (main.getWidth() * 0.12f));
    auto heroCol = main.removeFromLeft (juce::roundToInt (main.getWidth() * 0.30f));
    auto outMeterCol = main.removeFromRight (56);
    auto compCol = main;

    matchingPanel.setBounds (leftMatch);
    inputRms.setBounds (inMeterCol.reduced (2, 4));
    micPanel.setBounds (micCol);
    preampPanel.setBounds (preCol);
    heroPanel.setBounds (heroCol);
    compPanel.setBounds (compCol);
    outputRms.setBounds (outMeterCol.reduced (2, 4));

    auto match = matchingPanel.getBounds().reduced (8, 28);
    sourceMic.setBounds (match.removeFromTop (22));
    match.removeFromTop (2);
    targetMic.setBounds (match.removeFromTop (22));
    match.removeFromTop (3);
    analyzeSource.setBounds (match.removeFromBottom (22).reduced (8, 0));
    hardwareSafe.setBounds (match.removeFromBottom (22).reduced (8, 0));
    auto mRow1 = match.removeFromTop (match.getHeight() / 2);
    layoutKnobRow (mRow1, { &correction, &targetAmount });
    auto mRow2 = match;
    layoutKnobRow (mRow2, { &badFreq, &bodyProtect, &airProtect });

    auto mic = micPanel.getBounds().reduced (8, 28);
    micCharacterMode.setBounds (mic.removeFromTop (22));
    mic.removeFromTop (4);
    auto micRow2 = mic;
    deHarsh.setBounds (micRow2.removeFromLeft (micRow2.getWidth() / 3).reduced (1));
    proximity.setBounds (micRow2.removeFromLeft (micRow2.getWidth() / 2).reduced (1));
    hpfButton.setBounds (micRow2.reduced (1));

    auto pre = preampPanel.getBounds().reduced (8, 28);
    preampMode.setBounds (pre.removeFromTop (22));
    pre.removeFromTop (2);
    auto preRow1 = pre.removeFromTop (pre.getHeight() / 2);
    layoutKnobRow (preRow1, { &preampDrive, &preampTone });
    preampOutput.setBounds (pre.removeFromLeft (pre.getWidth() / 2).reduced (1));
    phaseInvert.setBounds (pre.reduced (1));

    auto hero = heroPanel.getBounds().reduced (8, 28);
    auto tubeBlock = hero.removeFromBottom (juce::roundToInt (hero.getHeight() * 0.40f));
    auto auraBlock = hero;
    sweetZone.setBounds (auraBlock.removeFromBottom (16));
    auto sweetRow = auraBlock.removeFromBottom (16);
    sweetLow.setBounds (sweetRow.removeFromLeft (sweetRow.getWidth() / 3));
    sweetHot.setBounds (sweetRow.removeFromRight (sweetRow.getWidth() / 3));
    auraPercent.setBounds (auraBlock.removeFromBottom (22));
    auraRing.setBounds (auraBlock.withSizeKeepingCentre (juce::jmin (auraBlock.getWidth(), auraBlock.getHeight()),
                                                          juce::jmin (auraBlock.getWidth(), auraBlock.getHeight())));
    aura.setBounds (auraBlock.reduced (auraBlock.getWidth() / 6, 0));
    tubeChamber.setBounds (tubeBlock.removeFromTop (tubeBlock.getHeight() - 88).reduced (4, 0));
    tubeType.setBounds (tubeBlock.removeFromTop (22).reduced (20, 0));
    auto tubeRow = tubeBlock;
    tubeDriveSlider.setBounds (tubeRow.removeFromLeft (tubeRow.getWidth() / 3).reduced (2));
    tubeBias.setBounds (tubeRow.reduced (2));

    auto comp = compPanel.getBounds().reduced (8, 28);
    auto compTop = comp.removeFromTop (22);
    compressorMode.setBounds (compTop.removeFromLeft (compTop.getWidth() / 2).reduced (1));
    vuMode.setBounds (compTop.reduced (1));
    comp.removeFromTop (2);
    auto vuArea = comp.removeFromRight (juce::roundToInt (comp.getWidth() * 0.40f));
    grLabel.setBounds (vuArea.removeFromBottom (16));
    compressorEnable.setBounds (vuArea.removeFromBottom (20).reduced (4, 0));
    compIn.setBounds (vuArea.removeFromBottom (18).removeFromLeft (vuArea.getWidth() / 2).reduced (1));
    compOut.setBounds (vuArea.removeFromBottom (18).reduced (1));
    vuMeter.setBounds (vuArea.reduced (2));
    auto cRow1 = comp.removeFromTop (comp.getHeight() / 2);
    layoutKnobRow (cRow1, { &threshold, &ratio, &attack, &release });
    layoutKnobRow (comp, { &compMakeup, &bleed, &scHpf });
    compTiming.setBounds (compPanel.getBounds().removeFromBottom (22).reduced (10, 2));

    auto rack = lowerRack.reduced (2, 0);
    const int rackW = rack.getWidth() / 8;
    satPanel.setBounds (rack.removeFromLeft (rackW).reduced (1));
    xfmrPanel.setBounds (rack.removeFromLeft (rackW).reduced (1));
    consolePanel.setBounds (rack.removeFromLeft (rackW * 2).reduced (1));
    gluePanel.setBounds (rack.removeFromLeft (rackW).reduced (1));
    eqPanel.setBounds (rack.removeFromLeft (rackW * 2).reduced (1));
    widthPanel.setBounds (rack.removeFromLeft (rackW).reduced (1));
    limiterPanel.setBounds (rack.reduced (1));

    saturation.setBounds (satPanel.getBounds().reduced (10, 30));
    transformer.setBounds (xfmrPanel.getBounds().reduced (10, 30));
    auto cons = consolePanel.getBounds().reduced (8, 28);
    auto consTop = cons.removeFromTop (22);
    consoleMode.setBounds (consTop.removeFromLeft (consTop.getWidth() / 2).reduced (1));
    trackCount.setBounds (consTop.reduced (1));
    consoleDensity.setBounds (cons.reduced (cons.getWidth() / 4, 2));
    glue.setBounds (gluePanel.getBounds().reduced (10, 30));
    eqDisplay.setBounds (eqPanel.getBounds().reduced (8, 26));
    auto wid = widthPanel.getBounds().reduced (8, 28);
    width.setBounds (wid.removeFromTop (wid.getHeight() / 2).reduced (2));
    mono.setBounds (wid.reduced (2));
    auto lim = limiterPanel.getBounds().reduced (8, 28);
    limiter.setBounds (lim.removeFromTop (22).reduced (2));
    ceiling.setBounds (lim.removeFromTop (lim.getHeight() / 2).reduced (2));
    limiterMeter.setBounds (lim.reduced (2));
    mix.setBounds (widthPanel.getBounds().removeFromBottom (18).reduced (4, 0));
    quality.setBounds (limiterPanel.getBounds().removeFromBottom (18).reduced (4, 0));

    auto strip = bottomStrip.reduced (2, 4);
    inputFader.setBounds (strip.removeFromLeft (200).reduced (4, 8));
    inputLr.setBounds (strip.removeFromLeft (72).reduced (2, 8));
    auto centre = strip.removeFromLeft (strip.getWidth() / 2);
    bypass.setBounds (centre.removeFromLeft (78).reduced (2, 18));
    mono.setBounds (centre.removeFromLeft (64).reduced (2, 18));
    dim.setBounds (centre.removeFromLeft (96).reduced (2, 18));
    presetCard.setBounds (centre.removeFromLeft (170).reduced (2, 16));
    favoriteBottom.setBounds (centre.removeFromLeft (34).reduced (2, 20));
    undoBottom.setBounds (centre.removeFromLeft (48).reduced (2, 20));
    redoBottom.setBounds (centre.removeFromLeft (48).reduced (2, 20));
    outputLr.setBounds (strip.removeFromRight (72).reduced (2, 8));
    outputFader.setBounds (strip.removeFromRight (200).reduced (4, 8));
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
    inputLr.setTargets (in, in * 0.93f);
    outputLr.setTargets (out, out * 0.94f);
    grLabel.setText ("GR " + juce::String (gr, 1) + " dB", juce::dontSendNotification);

    const auto vuIndex = juce::jlimit (0, 2, static_cast<int> (processorRef.apvts.getRawParameterValue ("vuMeterMode")->load()));
    vuMeter.setTargets (in, gr, out, static_cast<VuMeterComponent::Mode> (vuIndex));

    const auto tubeLevel = processorRef.tubeActivityMeter.load (std::memory_order_relaxed);
    const auto drive = processorRef.apvts.getRawParameterValue ("tubeDrive")->load();
    tubeChamber.setActivity (juce::jlimit (0.0f, 1.0f, tubeLevel * 0.72f + drive * 0.0028f));
    eqDisplay.setTone (processorRef.apvts.getRawParameterValue ("tone")->load() * 0.01f);

    const auto auraValue = processorRef.apvts.getRawParameterValue ("aura")->load();
    auraPercent.setText (juce::String (auraValue, 1) + " %", juce::dontSendNotification);
    auraRing.setValue (auraValue);
    presetCard.setText (processorRef.getProgramName (processorRef.getCurrentProgram()), juce::dontSendNotification);

    const auto hot = lim > 0.8f || gr > 10.0f || out > 0.94f || tubeLevel > 0.88f;
    const auto sweet = ! hot && in > 0.08f && (gr > 0.5f || drive > 18.0f) && out < 0.94f;
    sweetZone.setText (sweet ? "SWEET" : " ", juce::dontSendNotification);
    sweetZone.setColour (juce::Label::textColourId, sweet ? juce::Colour (0xffffc15b) : juce::Colour (0x00000000));
    sweetLow.setColour (juce::Label::textColourId, ! sweet && ! hot ? juce::Colour (0xff8d806c) : juce::Colour (0xff4d463c));
    sweetHot.setColour (juce::Label::textColourId, hot ? juce::Colour (0xffff6b3a) : juce::Colour (0xff4d463c));
}
