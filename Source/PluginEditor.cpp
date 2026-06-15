#include "PluginEditor.h"
#include "ui/RackDrawing.h"
#include "ui/StandaloneAudio.h"

namespace
{
constexpr const char* sectionEnableIds[] {
    "micSectionEnable", "preampSectionEnable", "compressorEnable",
    "harmonicsSectionEnable", "sumSectionEnable", "masterSectionEnable", nullptr
};
}

StadiumAuraAudioProcessorEditor::StadiumAuraAudioProcessorEditor (StadiumAuraAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&lookAndFeel);
    setResizable (true, true);
    setResizeLimits (1200, 760, 1920, 1150);
    setSize (1440, 920);
    aura.setHeroStyle (true);

    logoTitle.setText ("Stadium Aura", juce::dontSendNotification);
    logoTitle.setFont (juce::FontOptions (28.0f, juce::Font::bold));
    logoTitle.setColour (juce::Label::textColourId, juce::Colour (0xfff0d2a0));
    logoTitle.setJustificationType (juce::Justification::centred);
    logoSubtitle.setText ("PREMIUM ANALOG STAGE & DYNAMICS PROCESSOR", juce::dontSendNotification);
    logoSubtitle.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    logoSubtitle.setColour (juce::Label::textColourId, juce::Colour (0xffc58a3d));
    logoSubtitle.setJustificationType (juce::Justification::centred);
    presetCard.setJustificationType (juce::Justification::centred);
    presetCard.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    presetCard.setColour (juce::Label::textColourId, juce::Colour (0xffe8c98d));
    presetCard.setColour (juce::Label::backgroundColourId, juce::Colour (0xff101418));
    latencyLabel.setJustificationType (juce::Justification::centredRight);
    latencyLabel.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    latencyLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8d806c));
    oversamplingLabel.setJustificationType (juce::Justification::centredRight);
    oversamplingLabel.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    oversamplingLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8d806c));
    monitorLabel.setJustificationType (juce::Justification::centred);
    monitorLabel.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    monitorLabel.setColour (juce::Label::textColourId, juce::Colour (0xffffc15b));
    for (auto* label : { &sweetZone, &sweetLow, &sweetHot })
    {
        label->setJustificationType (juce::Justification::centred);
        label->setFont (juce::FontOptions (10.0f, juce::Font::bold));
        addAndMakeVisible (*label);
    }
    sweetLow.setText ("LOW", juce::dontSendNotification);
    sweetHot.setText ("HOT", juce::dontSendNotification);
    for (auto* label : { &logoTitle, &logoSubtitle, &presetCard, &latencyLabel, &oversamplingLabel, &monitorLabel })
        addAndMakeVisible (*label);

    for (auto* panel : { &leftPanel, &heroPanel, &rightPanel, &eqPanel })
        addAndMakeVisible (*panel);

    const std::pair<juce::Slider*, const char*> sliders[] {
        { &inputKnob, "input" }, { &outputKnob, "output" },
        { &inputFader, "input" }, { &outputFader, "output" },
        { &bodyKnob, "bodyProtection" }, { &presenceKnob, "tone" }, { &airKnob, "airProtection" },
        { &tubeDriveKnob, "tubeDrive" }, { &saturation, "saturation" }, { &tubeBias, "harmonicBias" },
        { &transformer, "transformer" }, { &summing, "summing" }, { &glue, "glue" },
        { &correction, "micCorrectionAmount" }, { &targetAmount, "micTargetAmount" }, { &badFreq, "badFrequencyTamer" },
        { &preampDrive, "preampDrive" }, { &aura, "aura" },
        // Compressor knobs re-wired to new COMP_* params (new engine)
        { &compAmount,  "COMP_AMOUNT"   }, { &compMakeup, "COMP_MAKEUP"    },
        { &attack,      "COMP_ATTACK"   }, { &release,    "COMP_RELEASE"   },
        { &threshold,   "COMP_THRESHOLD"}, { &ratio,      "COMP_RATIO"     },
        { &bleed,       "compBleedPercent" },  // legacy bleed kept on old param
        { &mix, "mix" }, { &width, "width" }, { &ceiling, "ceiling" },
        // New knobs
        { &compInputKnob,     "COMP_INPUT"        },
        { &compSidechainKnob, "COMP_SIDECHAIN_HPF"}
    };
    for (auto [slider, id] : sliders) attachSlider (*slider, id);

    attachButton (hardwareSafe, "hardwareSafeMode");
    attachButton (compressorEnable, "COMP_ENABLED");  // re-wired to new engine
    attachButton (limiter, "limiter");
    attachButton (bypass, "bypass");
    attachButton (mono, "monoCheck");
    attachButton (dim, "dim");
    attachButton (emotionLockBtn, "EMOTION_LOCK_ENABLED");
    attachButton (auraLevelBtn,   "COMP_AURA_LEVEL");

    attachCombo (sourceMic, "sourceMicMode", { "Unknown / Auto", "Dynamic General", "Condenser General",
        "Ribbon General", "57-Style Dynamic", "7B-Style Dynamic", "C80-Style Condenser", "Bright Condenser",
        "Dark Condenser", "Warm Tube Mic", "Flat / Measurement" });
    attachCombo (targetMic, "targetMicMode", { "67 Vintage Smooth", "C12 Open Air", "251 Classic Silk",
        "87 Modern Balanced", "47 Velvet Tube", "251E Silky Presence", "800G Air Pop" });
    attachCombo (preampMode, "preampMode", { "73 Vintage Iron", "API Punch", "Avalon Clean" });
    attachCombo (tubeType, "tubeType", { "Clean Triode", "Warm Triode", "Hot Triode", "Vintage Pentode", "Big Bottle", "Cream Opto Tube" });
    attachCombo (vuMode, "vuMeterMode", { "Input", "Gain Reduction", "Output" });
    attachCombo (consoleMode, "consoleMode", { "Clean Console", "Vintage Desk", "Modern Punch", "Tube Console" });

    for (auto* c : { static_cast<juce::Component*> (&inputRms), static_cast<juce::Component*> (&outputRms),
                     static_cast<juce::Component*> (&inputLrMeter), static_cast<juce::Component*> (&outputLrMeter),
                     static_cast<juce::Component*> (&grMeter), static_cast<juce::Component*> (&limMeter),
                     static_cast<juce::Component*> (&vuMeter), static_cast<juce::Component*> (&tubeChamber),
                     static_cast<juce::Component*> (&eqDisplay), static_cast<juce::Component*> (&trackButtons),
                     static_cast<juce::Component*> (&compModeButtons), static_cast<juce::Component*> (&qualityBar),
                     static_cast<juce::Component*> (&compModelBar) })
        addAndMakeVisible (*c);

    for (auto* label : { &emotionLockStatusLabel, &auraLevelStateLabel })
    {
        label->setFont (juce::FontOptions (9.5f, juce::Font::bold));
        label->setColour (juce::Label::textColourId, juce::Colour (0xffc58a3d));
        label->setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (*label);
    }

    trackButtons.setChoices ({ "8", "16", "24", "32" });
    trackButtons.setSelectedIndex (2, juce::dontSendNotification);
    trackButtons.onChange = [this] (int index)
    {
        setChoiceParameter ("trackCount", index + 1);
    };

    compModeButtons.setChoices ({ "FAST 76", "SMOOTH OPTO", "KID670" });
    compModeButtons.setSelectedIndex (1, juce::dontSendNotification);
    compModeButtons.onChange = [this] (int index) { setChoiceParameter ("compressorMode", index); };

    compModelBar.setChoices ({ "Lightning FET", "Velvet Opto", "Crown Mu", "Punch Cell", "Glue Bus", "Modern Clean" });
    compModelBar.setSelectedIndex (1, juce::dontSendNotification);
    compModelBar.onChange = [this] (int i) { setChoiceParameter ("COMP_MODEL", i); };

    qualityBar.setChoices ({ "ECO", "NORMAL", "HIGH", "ULTRA" });
    qualityBar.setSelectedIndex (2, juce::dontSendNotification);
    qualityBar.onChange = [this] (int index) { setChoiceParameter ("quality", index); };

    eqDisplay.onExpandedChanged = [this] (bool)
    {
        resized();
        eqDisplay.toFront (false);
    };
    eqDisplay.onNodeDragged = [this] (int band, float normX, float normY)
    {
        setEqNodeFromUi (band, normX, normY);
    };
    eqDisplay.bindToParameters (processorRef.apvts);

    const char* routeTips[] {
        "Mic: Source Match + character. Right-click toggles mic DSP bypass.",
        "Pre: Preamp architecture. Right-click toggles preamp bypass.",
        "Comp: Dynamics + VU. Right-click toggles compressor bypass.",
        "Harmonics: Tube, saturation, transformer. Right-click toggles harmonics bypass.",
        "Sum: Console density + glue. Right-click toggles summing bypass.",
        "Master: Aura macro + EQ + width. Right-click toggles master polish bypass.",
        "Output: Faders, meters, bypass, mono, dim."
    };
    for (int i = 0; i < static_cast<int> (routing.size()); ++i)
    {
        addAndMakeVisible (routing[static_cast<size_t> (i)]);
        routing[static_cast<size_t> (i)].setTooltip (routeTips[i]);
        routing[static_cast<size_t> (i)].onClick = [this, i] { setFocusedRoute (i); };
        routing[static_cast<size_t> (i)].onRightClick = [this, i] { toggleSectionBypass (i); };
    }
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

    for (auto* b : { static_cast<juce::Component*> (&presetPrev), static_cast<juce::Component*> (&presetNext),
                     static_cast<juce::Component*> (&saveBtn), static_cast<juce::Component*> (&abA),
                     static_cast<juce::Component*> (&abB), static_cast<juce::Component*> (&settingsBtn),
                     static_cast<juce::Component*> (&helpBtn), static_cast<juce::Component*> (&favoriteBtn),
                     static_cast<juce::Component*> (&undoBtn), static_cast<juce::Component*> (&redoBtn),
                     static_cast<juce::Component*> (&analyzeSource) })
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
    StandaloneAudio::configureEditorControls (settingsBtn, helpBtn);
    startTimerHz (60);
}

StadiumAuraAudioProcessorEditor::~StadiumAuraAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

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
    area = area.withHeight (juce::jmax (area.getHeight(), 88));
    const auto w = area.getWidth() / n;
    for (auto* knob : knobs)
        knob->setBounds (area.removeFromLeft (w).reduced (3, 2));
}

void StadiumAuraAudioProcessorEditor::layoutKnobGrid (juce::Rectangle<int>& area, int columns, std::initializer_list<juce::Slider*> knobs)
{
    const auto n = static_cast<int> (knobs.size());
    if (n <= 0 || columns <= 0) return;
    const auto rows = (n + columns - 1) / columns;
    area = area.withHeight (juce::jmax (area.getHeight(), rows * 84));
    const auto cellH = area.getHeight() / rows;
    const auto cellW = area.getWidth() / columns;
    int index = 0;
    for (auto* knob : knobs)
    {
        const auto row = index / columns;
        const auto col = index % columns;
        knob->setBounds (juce::Rectangle<int> (area.getX() + col * cellW, area.getY() + row * cellH, cellW, cellH).reduced (3, 2));
        ++index;
    }
}

void StadiumAuraAudioProcessorEditor::setChoiceParameter (const char* id, int index)
{
    if (auto* parameter = processorRef.apvts.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (static_cast<float> (index)));
}

void StadiumAuraAudioProcessorEditor::setFloatParameter (const char* id, float value)
{
    if (auto* parameter = processorRef.apvts.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

void StadiumAuraAudioProcessorEditor::setEqNodeFromUi (int band, float normX, float normY)
{
    auto frequencyFromNorm = [] (float norm, float minHz, float maxHz)
    {
        return minHz * std::pow (maxHz / minHz, juce::jlimit (0.0f, 1.0f, norm));
    };
    auto gainFromNorm = [] (float norm)
    {
        return juce::jmap (juce::jlimit (0.0f, 1.0f, norm), 1.0f, 0.0f, -12.0f, 12.0f);
    };

    if (band == 0)
        setFloatParameter ("eqHpfHz", frequencyFromNorm (normX, 20.0f, 500.0f));
    if (band == 1)
    {
        setFloatParameter ("eqLowShelfHz", frequencyFromNorm (normX, 40.0f, 500.0f));
        setFloatParameter ("eqLowShelfGainDb", gainFromNorm (normY));
    }
    if (band == 2)
    {
        setFloatParameter ("eqBellHz", frequencyFromNorm (normX, 120.0f, 10000.0f));
        setFloatParameter ("eqBellGainDb", gainFromNorm (normY));
    }
    if (band == 3)
    {
        setFloatParameter ("eqHighShelfHz", frequencyFromNorm (normX, 1500.0f, 16000.0f));
        setFloatParameter ("eqHighShelfGainDb", gainFromNorm (normY));
    }
    if (band == 4)
        setFloatParameter ("eqLpfHz", frequencyFromNorm (normX, 4000.0f, 20000.0f));
}

void StadiumAuraAudioProcessorEditor::updateEqDisplayState()
{
    // Advance the spectrum FFT on the UI thread (audio thread only pushes samples)
    processorRef.spectrumAnalyzer.processFFT();

    // Push the latest FFT data into the EQ panel's own analyzer
    const auto& mags = processorRef.spectrumAnalyzer.getMagnitudes();
    eqDisplay.pushSpectrumSamples (nullptr, 0); // keep the internal fifo alive
    eqDisplay.processSpectrumFFT();

    // Read all 24-band parameters from APVTS and update the panel
    eqDisplay.updateFromParameters (processorRef.apvts, processorRef.getSampleRate());
    eqDisplay.setTone (processorRef.apvts.getRawParameterValue ("tone")->load() * 0.01f);
    eqDisplay.setAnalyzerLevels (processorRef.getAnalyzerSnapshot());
    (void) mags;
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

bool StadiumAuraAudioProcessorEditor::isSectionEnabled (int routeIndex) const
{
    if (routeIndex < 0 || routeIndex > 5 || sectionEnableIds[static_cast<size_t> (routeIndex)] == nullptr)
        return true;
    return processorRef.apvts.getRawParameterValue (sectionEnableIds[static_cast<size_t> (routeIndex)])->load() > 0.5f;
}

void StadiumAuraAudioProcessorEditor::toggleSectionBypass (int routeIndex)
{
    if (routeIndex < 0 || routeIndex > 5 || sectionEnableIds[static_cast<size_t> (routeIndex)] == nullptr)
        return;
    if (auto* parameter = processorRef.apvts.getParameter (sectionEnableIds[static_cast<size_t> (routeIndex)]))
        parameter->setValueNotifyingHost (parameter->getValue() > 0.5f ? 0.0f : 1.0f);
    applyRouteVisuals();
}

void StadiumAuraAudioProcessorEditor::setFocusedRoute (int index)
{
    focusedRoute = juce::jlimit (0, static_cast<int> (routing.size()) - 1, index);
    for (int i = 0; i < static_cast<int> (routing.size()); ++i)
        routing[static_cast<size_t> (i)].setToggleState (i == focusedRoute, juce::dontSendNotification);
    applyRouteVisuals();
    resized();
}

void StadiumAuraAudioProcessorEditor::applyRouteVisuals()
{
    auto setPanel = [] (RackModulePanel& panel, bool highlight, bool dimmed)
    {
        panel.setRouteVisualState (highlight, dimmed);
    };

    const auto route = focusedRoute;
    setPanel (leftPanel, route == 0 || route == 1, route != 0 && route != 1);
    setPanel (heroPanel, route == 3 || route == 5, route != 3 && route != 5);
    setPanel (rightPanel, route == 2 || route == 6, route != 2 && route != 6);
    setPanel (eqPanel, route == 5, route != 5);

    sourceMic.setVisible (route == 0);
    targetMic.setVisible (route == 0);
    correction.setVisible (route == 0);
    targetAmount.setVisible (route == 0);
    badFreq.setVisible (route == 0);
    hardwareSafe.setVisible (route == 0);
    analyzeSource.setVisible (route == 0);
    preampMode.setVisible (route == 0 || route == 1);
    preampDrive.setVisible (route == 0 || route == 1);
    threshold.setVisible (route == 2);
    ratio.setVisible (route == 2);
    bleed.setVisible (false);  // legacy bleed hidden; compInputKnob/compSidechainKnob take its row slot

    const char* routeNames[] { "MIC", "PRE", "COMP", "HARM", "SUM", "MASTR", "OUT" };
    for (int i = 0; i < 6; ++i)
    {
        const auto enabled = isSectionEnabled (i);
        routing[static_cast<size_t> (i)].setAlpha (enabled ? 1.0f : 0.55f);
        routing[static_cast<size_t> (i)].setButtonText (routeNames[i] + juce::String (enabled ? "" : " OFF"));
    }
    routing[6].setAlpha (focusedRoute == 6 ? 1.0f : 0.88f);
    routing[6].setButtonText ("OUT");
}

void StadiumAuraAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto frame = getLocalBounds().toFloat().reduced (8.0f);
    RackDrawing::paintBrushedMetal (g, frame);
    g.setColour (juce::Colour (0xff5a4224));
    g.drawRoundedRectangle (frame, 8.0f, 2.0f);
    RackDrawing::paintScrews (g, frame, 14.0f);

    g.setColour (juce::Colour (0x18ffffff));
    g.drawHorizontalLine (juce::roundToInt (frame.getY() + 88.0f), frame.getX() + 20.0f, frame.getRight() - 20.0f);
    g.drawHorizontalLine (juce::roundToInt (frame.getBottom() - 168.0f), frame.getX() + 20.0f, frame.getRight() - 20.0f);
    g.drawHorizontalLine (juce::roundToInt (frame.getBottom() - 72.0f), frame.getX() + 20.0f, frame.getRight() - 20.0f);

    auto footer = getLocalBounds().reduced (16, 10).removeFromBottom (58).toFloat();
    g.setColour (juce::Colour (0xff0d1013));
    g.fillRoundedRectangle (footer.reduced (2.0f), 5.0f);
    g.setColour (juce::Colour (0xff3a2f22));
    g.drawRoundedRectangle (footer.reduced (2.0f), 5.0f, 1.0f);
}

void StadiumAuraAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (16, 10);
    auto topBar    = bounds.removeFromTop (52);
    auto footerBar = bounds.removeFromBottom (80);
    auto meterBar  = bounds.removeFromBottom (90);
    auto eqRow     = bounds.removeFromBottom (200);   // tall enough for real EQ display
    auto main      = bounds.reduced (0, 4);

    // ── TOP BAR ──────────────────────────────────────────────────────────────
    auto logo = topBar.removeFromLeft (270);
    logoTitle.setBounds (logo.removeFromTop (30));
    logoSubtitle.setBounds (logo);
    favoriteBtn.setBounds (topBar.removeFromLeft (26).reduced (2, 12));
    presetPrev.setBounds  (topBar.removeFromLeft (26).reduced (2, 12));
    presetNext.setBounds  (topBar.removeFromLeft (26).reduced (2, 12));
    presetCard.setBounds  (topBar.removeFromLeft (190).reduced (4, 8));
    saveBtn.setBounds     (topBar.removeFromLeft (48).reduced (2, 12));
    abA.setBounds         (topBar.removeFromLeft (26).reduced (2, 12));
    abB.setBounds         (topBar.removeFromLeft (26).reduced (2, 12));
    topBar.removeFromLeft (44);  // reserved space — undo/redo live in the footer
    settingsBtn.setBounds (topBar.removeFromRight (30).reduced (2, 12));
    helpBtn.setBounds     (topBar.removeFromRight (30).reduced (2, 12));
    monitorLabel.setBounds (topBar.removeFromRight (180).reduced (2, 12));
    auto nav = topBar.reduced (2, 4);
    const auto navW = nav.getWidth() / static_cast<int> (routing.size());
    for (auto& button : routing)
        button.setBounds (nav.removeFromLeft (navW).reduced (2, 1));

    // ── MAIN ROW: left | inMeter | center | outMeter | right ─────────────────
    auto inMeter  = main.removeFromLeft (58);
    auto outMeter = main.removeFromRight (58);
    const auto leftW   = juce::roundToInt (main.getWidth() * 0.27f);
    const auto centerW = juce::roundToInt (main.getWidth() * 0.40f);
    auto left   = main.removeFromLeft (leftW).reduced (2, 0);
    auto center = main.removeFromLeft (centerW).reduced (2, 0);
    auto right  = main.reduced (2, 0);

    leftPanel.setBounds (left);
    heroPanel.setBounds (center);
    rightPanel.setBounds (right);
    eqPanel.setBounds (eqRow.reduced (2, 0));
    inputRms.setBounds  (inMeter.reduced (2, 4));
    outputRms.setBounds (outMeter.reduced (2, 4));

    // ── LEFT PANEL ────────────────────────────────────────────────────────────
    auto leftArea = leftPanel.getBounds().reduced (10, 30);
    // Source/Target mic dropdowns — guaranteed separate vertical slots
    sourceMic.setBounds (leftArea.removeFromTop (24));
    leftArea.removeFromTop (4);
    targetMic.setBounds (leftArea.removeFromTop (24));
    leftArea.removeFromTop (8);
    // Matching engine knobs — 2 columns, 3 rows = 6 knobs
    const auto knobH = juce::jmin (78, (leftArea.getHeight() - 90) / 4);
    auto row1 = leftArea.removeFromTop (knobH);
    layoutKnobRow (row1, { &correction, &targetAmount });
    auto row2 = leftArea.removeFromTop (knobH);
    layoutKnobRow (row2, { &badFreq, &bodyKnob });
    auto row3 = leftArea.removeFromTop (knobH);
    airKnob.setBounds (row3.removeFromLeft (row3.getWidth() / 2).withSizeKeepingCentre (knobH - 4, knobH - 4));
    analyzeSource.setBounds (row3.reduced (4, knobH / 4));
    hardwareSafe.setBounds (leftArea.removeFromTop (26).reduced (4, 2));
    leftArea.removeFromTop (6);
    // Preamp/char knobs — mix lives in the right panel output row, not here
    auto charRow = leftArea.removeFromTop (knobH);
    presenceKnob.setBounds (charRow.withSizeKeepingCentre (juce::jmin (charRow.getWidth() - 8, knobH - 4), knobH - 4));
    preampMode.setBounds (leftArea.removeFromTop (24));
    leftArea.removeFromTop (4);
    if (leftArea.getHeight() >= 60)
        preampDrive.setBounds (leftArea.withSizeKeepingCentre (juce::jmin (leftArea.getWidth() - 8, 76), 76));

    // ── CENTER / HERO PANEL ───────────────────────────────────────────────────
    auto hero = heroPanel.getBounds().reduced (10, 30);
    // Tube column on right — tall capsule tubes
    auto tubeColumn = hero.removeFromRight (130);
    tubeDriveKnob.setBounds (tubeColumn.removeFromTop (90).withSizeKeepingCentre (76, 76));
    tubeBias.setBounds       (tubeColumn.removeFromTop (80).withSizeKeepingCentre (68, 68));
    tubeType.setBounds       (tubeColumn.removeFromTop (24).reduced (4, 0));
    tubeChamber.setBounds    (tubeColumn.reduced (4, 6));          // remaining space = tall tubes
    // Aura knob — square, centered, no stretching
    auto trackRow = hero.removeFromBottom (26);
    trackButtons.setBounds (trackRow.reduced (40, 0));
    auto sweetRow = hero.removeFromBottom (20);
    sweetLow.setBounds  (sweetRow.removeFromLeft (sweetRow.getWidth() / 3));
    sweetZone.setBounds (sweetRow.removeFromLeft (sweetRow.getWidth() / 2));
    sweetHot.setBounds  (sweetRow);
    const auto auraSize = juce::jmin (hero.getWidth(), hero.getHeight()) - 10;
    aura.setBounds (hero.withSizeKeepingCentre (auraSize, auraSize).reduced (6));

    // ── RIGHT / DYNAMICS PANEL ────────────────────────────────────────────────
    auto rightArea = rightPanel.getBounds().reduced (10, 30);

    // Model bar (new engine — 6 models)
    compModelBar.setBounds (rightArea.removeFromTop (28));
    rightArea.removeFromTop (4);

    // Legacy mode bar (smaller, secondary)
    compModeButtons.setBounds (rightArea.removeFromTop (22));
    rightArea.removeFromTop (4);

    auto vuArea = rightArea.removeFromRight (juce::roundToInt (rightArea.getWidth() * 0.42f));
    vuMode.setBounds         (vuArea.removeFromBottom (22).reduced (2));
    compressorEnable.setBounds (vuArea.removeFromBottom (24).reduced (2));

    // Emotion Lock + Aura Level toggles above the VU meter
    auto elRow = vuArea.removeFromBottom (22);
    emotionLockBtn.setBounds (elRow.removeFromLeft (elRow.getWidth() / 2).reduced (2));
    auraLevelBtn.setBounds   (elRow.reduced (2));
    auto elLabelRow = vuArea.removeFromBottom (16);
    emotionLockStatusLabel.setBounds (elLabelRow.removeFromLeft (elLabelRow.getWidth() / 2).reduced (2));
    auraLevelStateLabel.setBounds    (elLabelRow.reduced (2));

    vuMeter.setBounds (vuArea.reduced (2));

    // Knob row 1: compAmount, compMakeup, attack, release
    auto compRow1 = rightArea.removeFromTop (92);
    layoutKnobRow (compRow1, { &compAmount, &compMakeup, &attack, &release });

    // Knob row 2: threshold, ratio, compInputKnob, compSidechainKnob
    auto compRow2 = rightArea.removeFromTop (82);
    layoutKnobRow (compRow2, { &threshold, &ratio, &compInputKnob, &compSidechainKnob });

    rightArea.removeFromTop (4);

    // Knob row 3: mix, width, ceiling
    auto outRow = rightArea.removeFromTop (78);
    layoutKnobRow (outRow, { &mix, &width, &ceiling });

    if (rightArea.getHeight() >= 80)
        outputKnob.setBounds (rightArea.withSizeKeepingCentre (juce::jmin (rightArea.getWidth() - 8, 90), 90));

    // ── EQ ROW ────────────────────────────────────────────────────────────────
    auto lower = eqPanel.getBounds().reduced (10, 28);
    // Saturation / Transformer knobs on far left
    auto lowerLeft = lower.removeFromLeft (130);
    layoutKnobRow (lowerLeft, { &saturation, &transformer });
    // Console / Summing block
    auto consolePart = lower.removeFromLeft (180);
    consoleMode.setBounds (consolePart.removeFromTop (24).reduced (4, 0));
    consolePart.removeFromTop (4);
    { auto tmp = consolePart.removeFromTop (80); layoutKnobRow (tmp, { &summing, &glue }); }
    // Limiter block on far right
    auto limiterBox = lower.removeFromRight (150);
    limiter.setBounds (limiterBox.removeFromTop (28).reduced (4, 2));
    limiterBox.removeFromTop (4);
    ceiling.setBounds (limiterBox.removeFromTop (80).withSizeKeepingCentre (72, 72));
    limMeter.setBounds (limiterBox.reduced (4, 2));
    // EQ spectrum/display takes the remaining center space — full height
    eqDisplay.setBounds (lower.reduced (4, 0));
    if (eqDisplay.isExpanded())
        eqDisplay.setBounds (getLocalBounds().reduced (80, 60));

    // ── METER BAR ─────────────────────────────────────────────────────────────
    auto meters = meterBar.reduced (2, 4);
    grMeter.setBounds (meters.removeFromLeft (juce::roundToInt (meters.getWidth() * 0.38f)).reduced (4, 6));
    auto util = meters.removeFromLeft (juce::roundToInt (meters.getWidth() * 0.62f));
    mono.setBounds   (util.removeFromLeft (64).reduced (2, 16));
    bypass.setBounds (util.removeFromLeft (88).reduced (2, 10));
    dim.setBounds    (util.removeFromLeft (64).reduced (2, 16));
    qualityBar.setBounds (util.reduced (2, 18));
    // width knob lives in the right panel output row, not the meter bar

    // ── FOOTER / BOTTOM STRIP ─────────────────────────────────────────────────
    auto footer = footerBar.reduced (2, 4);
    inputFader.setBounds   (footer.removeFromLeft (130).reduced (4, 2));
    inputLrMeter.setBounds (footer.removeFromLeft (72).reduced (4, 2));
    // preset & undo/redo in the center of footer
    presets.setBounds      (footer.removeFromLeft (juce::jmax (200, footer.getWidth() / 3)).reduced (8, 16));
    undoBtn.setBounds      (footer.removeFromLeft (52).reduced (6, 18));
    redoBtn.setBounds      (footer.removeFromLeft (52).reduced (6, 18));
    oversamplingLabel.setBounds (footer.removeFromRight (130).reduced (2, 18));
    latencyLabel.setBounds      (footer.removeFromRight (120).reduced (2, 18));
    outputLrMeter.setBounds (footer.removeFromRight (72).reduced (4, 2));
    outputFader.setBounds   (footer.removeFromRight (130).reduced (4, 2));

    eqDisplay.toFront (false);
}

void StadiumAuraAudioProcessorEditor::timerCallback()
{
    // Keep input unmuted for the first ~2 s — JUCE's standalone audio device
    // state loads after the editor constructor and can silently re-mute input.
    if (startupUnmuteCountdown > 0)
    {
        --startupUnmuteCountdown;
        StandaloneAudio::forceUnmuteInput();
    }

    const auto in = processorRef.inputMeter.load (std::memory_order_relaxed);
    const auto out = processorRef.outputMeter.load (std::memory_order_relaxed);
    const auto gr = processorRef.gainReductionMeter.load (std::memory_order_relaxed);
    const auto lim = processorRef.limiterReductionMeter.load (std::memory_order_relaxed);
    inputRms.setTarget (in);
    outputRms.setTarget (out);
    inputLrMeter.setTargets (processorRef.inputLeftMeter.load (std::memory_order_relaxed),
                             processorRef.inputRightMeter.load (std::memory_order_relaxed));
    outputLrMeter.setTargets (processorRef.outputLeftMeter.load (std::memory_order_relaxed),
                              processorRef.outputRightMeter.load (std::memory_order_relaxed));
    grMeter.setTargetDb (gr);
    limMeter.setTargetDb (lim);

    const auto vuIndex = juce::jlimit (0, 2, static_cast<int> (processorRef.apvts.getRawParameterValue ("vuMeterMode")->load()));
    vuMeter.setTargets (in, gr, out, static_cast<VuMeterComponent::Mode> (vuIndex));

    const auto tubeLevel = processorRef.tubeActivityMeter.load (std::memory_order_relaxed);
    const auto drive = processorRef.apvts.getRawParameterValue ("tubeDrive")->load();
    tubeChamber.setActivity (juce::jlimit (0.0f, 1.0f, tubeLevel * 0.72f + drive * 0.0028f));
    updateEqDisplayState();

    presetCard.setText (processorRef.getProgramName (processorRef.getCurrentProgram()), juce::dontSendNotification);

    const auto qualityIndex = juce::jlimit (0, 3, static_cast<int> (processorRef.apvts.getRawParameterValue ("quality")->load()));
    qualityBar.setSelectedIndex (qualityIndex, juce::dontSendNotification);
    const auto trackIndex = juce::jlimit (1, 4, static_cast<int> (processorRef.apvts.getRawParameterValue ("trackCount")->load()));
    trackButtons.setSelectedIndex (trackIndex - 1, juce::dontSendNotification);
    const auto compIndex = juce::jlimit (0, 2, static_cast<int> (processorRef.apvts.getRawParameterValue ("compressorMode")->load()));
    compModeButtons.setSelectedIndex (compIndex, juce::dontSendNotification);

    const auto newCompIndex = juce::jlimit (0, 5, static_cast<int> (processorRef.apvts.getRawParameterValue ("COMP_MODEL")->load()));
    compModelBar.setSelectedIndex (newCompIndex, juce::dontSendNotification);

    // Emotion Lock and Aura Level visual feedback
    const bool elEnabled = processorRef.apvts.getRawParameterValue ("EMOTION_LOCK_ENABLED")->load() > 0.5f;
    emotionLockBtn.setAlpha (elEnabled ? 1.0f : 0.6f);
    emotionLockStatusLabel.setText (elEnabled ? "Active" : "", juce::dontSendNotification);

    const bool alEnabled = processorRef.apvts.getRawParameterValue ("COMP_AURA_LEVEL")->load() > 0.5f;
    auraLevelBtn.setAlpha (alEnabled ? 1.0f : 0.6f);
    auraLevelStateLabel.setText (alEnabled ? "Listening" : "", juce::dontSendNotification);

    // Sync new GR from the new engine to the GR meter (override with new engine when active)
    const bool newEngineOn = processorRef.apvts.getRawParameterValue ("COMP_ENABLED")->load() > 0.5f;
    if (newEngineOn)
    {
        const float newGr = processorRef.newCompGainReduction.load (std::memory_order_relaxed);
        grMeter.setTargetDb (newGr);
    }

    const auto latencyMs = processorRef.getLatencySamples() * 1000.0 / processorRef.getSampleRate();
    latencyLabel.setText ("LATENCY: " + juce::String (latencyMs, 1) + " ms", juce::dontSendNotification);
    oversamplingLabel.setText ("OVERSAMPLING: " + juce::String (qualityIndex >= 2 ? "4x" : (qualityIndex == 1 ? "2x" : "1x")),
                               juce::dontSendNotification);

    if (StandaloneAudio::isStandaloneBuild())
    {
        if (StandaloneAudio::isInputMuted())
        {
            monitorLabel.setText (StandaloneAudio::getMonitorStatusText(), juce::dontSendNotification);
            monitorLabel.setColour (juce::Label::textColourId, juce::Colour (0xffff6b3a));
        }
        else if (processorRef.inputMeter.load (std::memory_order_relaxed) < 0.001f)
        {
            monitorLabel.setText (juce::CharPointer_UTF8 ("\xe2\x9a\xa0 No live input detected"), juce::dontSendNotification);
            monitorLabel.setColour (juce::Label::textColourId, juce::Colour (0xffff9a2e));
        }
        else
        {
            monitorLabel.setText (StandaloneAudio::getMonitorStatusText(), juce::dontSendNotification);
            monitorLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8fcf4a));
        }
    }

    const auto hot = lim > 0.8f || gr > 10.0f || out > 0.94f || tubeLevel > 0.88f;
    const auto sweet = ! hot && in > 0.08f && (gr > 0.5f || drive > 18.0f) && out < 0.94f;
    sweetZone.setText (sweet ? "SWEET" : " ", juce::dontSendNotification);
    sweetZone.setColour (juce::Label::textColourId, sweet ? juce::Colour (0xffffc15b) : juce::Colour (0x00000000));
    sweetLow.setColour (juce::Label::textColourId, ! sweet && ! hot ? juce::Colour (0xff8d806c) : juce::Colour (0xff4d463c));
    sweetHot.setColour (juce::Label::textColourId, hot ? juce::Colour (0xffff6b3a) : juce::Colour (0xff4d463c));
}
