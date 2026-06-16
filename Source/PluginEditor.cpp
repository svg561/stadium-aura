#include "PluginEditor.h"
#include "dsp/AuraCompressorEngine.h"
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
        { &preampDrive, "preampDrive" },
        { &compInputKnob,   "COMP_INPUT"     }, { &threshold,   "COMP_THRESHOLD" },
        { &ratio,           "COMP_RATIO"     }, { &attack,      "COMP_ATTACK"    },
        { &release,         "COMP_RELEASE"   }, { &compMixKnob, "COMP_MIX"       },
        { &compDriveKnob,   "COMP_DRIVE"     }, { &compOutputKnob, "COMP_MAKEUP" },
        { &compDensityKnob, "COMP_DENSITY"   }, { &compWarmthKnob, "COMP_WARMTH" },
        { &compAmount,      "COMP_AMOUNT"    },
        { &bleed,       "compBleedPercent" },
        { &mix, "mix" }, { &width, "width" }, { &ceiling, "ceiling" },
        { &compSidechainKnob, "COMP_SIDECHAIN_HPF" }
    };
    for (auto [slider, id] : sliders) attachSlider (*slider, id);

    addAndMakeVisible (aura);
    aura.setTextValueSuffix (" %");
    aura.textFromValueFunction = [] (double value) { return juce::String (value * 100.0, 1); };
    aura.valueFromTextFunction = [] (const juce::String& text)
    {
        return text.trim().trimCharactersAtEnd ("%").getDoubleValue() / 100.0;
    };
    sliderAttachments.push_back (std::make_unique<SliderAttachment> (processorRef.apvts, "AURA_BIG_AMOUNT", aura));

    attachButton (hardwareSafe, "hardwareSafeMode");
    attachButton (compressorEnable, "COMP_ENABLED");
    attachButton (compBypassBtn, "COMP_BYPASS");
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
    attachCombo (compTimingMode, "COMP_TIMING_MODE", { "Fixed", "Manual", "Hybrid" });
    attachCombo (compScHpfMode, "COMP_SC_HPF_MODE", { "Off", "80 Hz", "150 Hz", "220 Hz" });

    for (auto* c : { static_cast<juce::Component*> (&inputRms), static_cast<juce::Component*> (&outputRms),
                     static_cast<juce::Component*> (&inputLrMeter), static_cast<juce::Component*> (&outputLrMeter),
                     static_cast<juce::Component*> (&grMeter), static_cast<juce::Component*> (&limMeter),
                     static_cast<juce::Component*> (&vuMeter), static_cast<juce::Component*> (&tubeChamber),
                     static_cast<juce::Component*> (&eqDisplay), static_cast<juce::Component*> (&trackButtons),
                     static_cast<juce::Component*> (&compModeButtons), static_cast<juce::Component*> (&qualityBar),
                     static_cast<juce::Component*> (&compModelBar), static_cast<juce::Component*> (&compProfileBar),
                     static_cast<juce::Component*> (&compGrMeter) })
        addAndMakeVisible (*c);

    addAndMakeVisible (compBypassBtn);
    addAndMakeVisible (compTimingMode);
    addAndMakeVisible (compScHpfMode);
    compTargetGrLabel.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    compTargetGrLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8d806c));
    compTargetGrLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (compTargetGrLabel);

    for (auto* label : { &emotionLockStatusLabel, &auraLevelStateLabel })
    {
        label->setFont (juce::FontOptions (9.5f, juce::Font::bold));
        label->setColour (juce::Label::textColourId, juce::Colour (0xffc58a3d));
        label->setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (*label);
    }

    auraBigLabel.setText ("AURA BIG", juce::dontSendNotification);
    auraBigLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    auraBigLabel.setColour (juce::Label::textColourId, juce::Colour (0xffc8962e));  // gold
    auraBigLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (auraBigLabel);
    addAndMakeVisible (auraHeatRing);
    auraHeatRing.toBack();

    for (auto& led : auraBigStageLeds)
        addAndMakeVisible (led);

    for (auto* c : { static_cast<juce::Component*> (&inputVuMeter),
                     static_cast<juce::Component*> (&grHorizontalMeter),
                     static_cast<juce::Component*> (&outputVuMeter) })
        addAndMakeVisible (*c);

    trackButtons.setChoices ({ "8", "16", "24", "32" });
    trackButtons.setSelectedIndex (2, juce::dontSendNotification);
    trackButtons.onChange = [this] (int index)
    {
        setChoiceParameter ("trackCount", index + 1);
    };

    compModeButtons.setChoices ({ "FAST 76", "SMOOTH OPTO", "KID670" });
    compModeButtons.setSelectedIndex (1, juce::dontSendNotification);
    compModeButtons.onChange = [this] (int index) { setChoiceParameter ("compressorMode", index); };

    compModelBar.setChoices ({ "Aura 2A", "Aura 76", "Aura Tube", "Aura Limiter", "Aura Density", "Aura Drums" });
    compModelBar.setSelectedIndex (0, juce::dontSendNotification);
    compModelBar.onChange = [this] (int i)
    {
        setChoiceParameter ("COMP_MODEL", i);
        refreshCompressorProfileBar();
        updateCompressorControlVisibility();
    };

    compProfileBar.setSelectedIndex (0, juce::dontSendNotification);
    compProfileBar.onChange = [this] (int i) { setChoiceParameter ("COMP_PROFILE", i); };
    refreshCompressorProfileBar();
    updateCompressorControlVisibility();

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

    // Compact strip click opens the expanded AURA EQ overlay
    eqDisplay.onEmptyAreaClicked = [this]
    {
        if (expandedEQPanel != nullptr)
        {
            expandedEQPanel->setVisible (true);
            expandedEQPanel->toFront (false);
            resized();
        }
    };

    // Create and add the expanded EQ panel (initially hidden)
    expandedEQPanel = std::make_unique<ExpandedEQPanel> (processorRef);
    addChildComponent (*expandedEQPanel);
    expandedEQPanel->onClose = [this]
    {
        expandedEQPanel->setVisible (false);
        resized();
    };

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
    StandaloneAudio::autoSelectDefaultInput();  // try to activate mic on startup
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

void StadiumAuraAudioProcessorEditor::refreshCompressorProfileBar()
{
    const auto modelIndex = juce::jlimit (0, 5, static_cast<int> (processorRef.apvts.getRawParameterValue ("COMP_MODEL")->load()));
    const auto model = static_cast<AuraCompressorModel> (modelIndex);
    const int count = getAuraCompressorProfileCount (model);
    juce::StringArray names;
    for (int i = 0; i < count; ++i)
        names.add (getAuraCompressorProfileName (model, i));
    compProfileBar.setChoices (names);
    const int profileIndex = juce::jlimit (0, juce::jmax (0, count - 1),
                                           static_cast<int> (processorRef.apvts.getRawParameterValue ("COMP_PROFILE")->load()));
    compProfileBar.setSelectedIndex (profileIndex, juce::dontSendNotification);
}

void StadiumAuraAudioProcessorEditor::updateCompressorControlVisibility()
{
    const auto modelIndex = juce::jlimit (0, 5, static_cast<int> (processorRef.apvts.getRawParameterValue ("COMP_MODEL")->load()));
    const auto model = static_cast<AuraCompressorModel> (modelIndex);
    compTimingMode.setVisible (model == AuraCompressorModel::AuraTube);
    compScHpfMode.setVisible (model == AuraCompressorModel::AuraTube || model == AuraCompressorModel::AuraDrums);
    compWarmthKnob.setVisible (model == AuraCompressorModel::Aura2A || model == AuraCompressorModel::AuraDensity);
    compDensityKnob.setVisible (model == AuraCompressorModel::AuraDensity);
    compDriveKnob.setVisible (model != AuraCompressorModel::AuraLimiter);
    compAmount.setVisible (model == AuraCompressorModel::Aura2A);
}

void StadiumAuraAudioProcessorEditor::layoutKnobRow (juce::Rectangle<int>& area, std::initializer_list<juce::Slider*> knobs)
{
    const auto n = static_cast<int> (knobs.size());
    if (n <= 0) return;
    const auto w = area.getWidth() / n;
    for (auto* knob : knobs)
        knob->setBounds (area.removeFromLeft (w).reduced (3, 2));
}

void StadiumAuraAudioProcessorEditor::layoutKnobGrid (juce::Rectangle<int>& area, int columns, std::initializer_list<juce::Slider*> knobs)
{
    const auto n = static_cast<int> (knobs.size());
    if (n <= 0 || columns <= 0) return;
    const auto rows = (n + columns - 1) / columns;
    // Use actual area dimensions — no forced minimum that causes off-screen placement
    const auto cellH = juce::jmax (1, area.getHeight() / rows);
    const auto cellW = juce::jmax (1, area.getWidth() / columns);
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
    targetMic.setVisible (false);  // TODO: expose on a dedicated mic-character page
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
    // ── Layout constants ──────────────────────────────────────────────────────
    constexpr int kPanelPadding = 14;  // horizontal padding inside panel border
    constexpr int kPanelTop     = 30;  // vertical offset below panel title
    constexpr int kGap          = 4;   // small gap between adjacent controls
    constexpr int kDropdownH    = 28;  // dropdown / ComboBox row height
    constexpr int kBtnH         = 24;  // standard button height

    auto area       = getLocalBounds().reduced (10);
    auto header     = area.removeFromTop (72);
    // Give the main instrument area 64 % of the remaining height — larger knobs
    auto main       = area.removeFromTop (juce::roundToInt (area.getHeight() * 0.64f));
    auto eqStrip    = area.removeFromTop (100);
    auto meterStrip = area.removeFromTop (90);
    auto bottom     = area;   // remaining height for fader / monitor strip

    // ── HEADER ───────────────────────────────────────────────────────────────
    const int logoW = (getWidth() < 1300) ? 180 : 220;
    auto logo = header.removeFromLeft (logoW);
    logoTitle.setBounds    (logo.removeFromTop (logo.getHeight() / 2));
    logoSubtitle.setBounds (logo);

    favoriteBtn.setBounds (header.removeFromLeft (26).reduced (2, 12));
    presetPrev.setBounds  (header.removeFromLeft (26).reduced (2, 12));
    presetNext.setBounds  (header.removeFromLeft (26).reduced (2, 12));
    presetCard.setBounds  (header.removeFromLeft (180).reduced (4,  8));
    saveBtn.setBounds     (header.removeFromLeft (44) .reduced (2, 12));
    abA.setBounds         (header.removeFromLeft (26) .reduced (2, 12));
    abB.setBounds         (header.removeFromLeft (26) .reduced (2, 12));
    header.removeFromLeft (80);   // visual gap — undo / redo live in bottom strip

    settingsBtn.setBounds  (header.removeFromRight (30).reduced (2, 12));
    helpBtn.setBounds      (header.removeFromRight (30).reduced (2, 12));
    monitorLabel.setBounds (header.removeFromRight (160).reduced (2, 12));

    {
        auto nav  = header.reduced (2, 4);
        const int navW = nav.getWidth() / static_cast<int> (routing.size());
        for (auto& b : routing)
            b.setBounds (nav.removeFromLeft (navW).reduced (2, 1));
    }

    // ── MAIN ROW: inMeter | left | center | right | outMeter ─────────────────
    auto inMeter  = main.removeFromLeft (52);
    auto outMeter = main.removeFromRight (52);
    auto left     = main.removeFromLeft (juce::roundToInt (main.getWidth() * 0.30f));
    auto center   = main.removeFromLeft (juce::roundToInt (main.getWidth() * 0.45f));
    auto right    = main;

    leftPanel.setBounds  (left);
    heroPanel.setBounds  (center);
    rightPanel.setBounds (right);
    eqPanel.setBounds    (eqStrip.reduced (2, 0));
    inputRms.setBounds   (inMeter.reduced (2, 4));
    outputRms.setBounds  (outMeter.reduced (2, 4));

    // ── LEFT PANEL (INPUT / TONE) ─────────────────────────────────────────────
    {
        auto la = left.reduced (kPanelPadding, kPanelTop);

        // Three dropdowns — Mic Model, Preamp Model, Console Model
        // targetMic is always hidden (see applyRouteVisuals); no slot reserved here
        sourceMic.setBounds   (la.removeFromTop (kDropdownH));
        la.removeFromTop (kGap);
        preampMode.setBounds  (la.removeFromTop (kDropdownH));
        la.removeFromTop (kGap);
        consoleMode.setBounds (la.removeFromTop (kDropdownH));
        la.removeFromTop (kGap * 2);
        targetMic.setBounds   ({});   // hidden — targetMic has no layout slot

        // 2-column × 4-row knob grid; cell height derived from remaining space
        // PremiumKnob needs: nameLabel(14) + rotary + textBox(16) = cell height
        // kBtnH*2 + kGap*3 reserved for Analyze + Safe Mode buttons below
        const int buttonsH = kBtnH + kGap + kBtnH + kGap * 2;
        const int kH       = juce::jmax (56, juce::jmin (92, (la.getHeight() - buttonsH) / 4));
        auto gridArea      = la.removeFromTop (kH * 4);
        layoutKnobGrid (gridArea, 2, { &correction,  &targetAmount,
                                       &bodyKnob,    &airKnob,
                                       &presenceKnob,&inputKnob,
                                       &preampDrive, &badFreq });

        la.removeFromTop (kGap * 2);
        analyzeSource.setBounds (la.removeFromTop (kBtnH).reduced (2));
        la.removeFromTop (kGap);
        hardwareSafe.setBounds  (la.removeFromTop (kBtnH).reduced (2));
    }

    // ── CENTER PANEL (AURA) ───────────────────────────────────────────────────
    {
        auto ha = center.reduced (kPanelPadding, kPanelTop);

        // Tube column on the right — bias, type selector, chamber visualiser
        auto tubeCol = ha.removeFromRight (130);
        tubeDriveKnob.setBounds (tubeCol.removeFromTop (90).withSizeKeepingCentre (76, 76));
        tubeBias.setBounds      (tubeCol.removeFromTop (80).withSizeKeepingCentre (68, 68));
        tubeType.setBounds      (tubeCol.removeFromTop (kDropdownH).reduced (4, 0));
        tubeChamber.setBounds   (tubeCol.reduced (4, 6));

        // Track selector + sweet-zone labels at the bottom
        auto trackRow = ha.removeFromBottom (26);
        trackButtons.setBounds (trackRow.reduced (40, 0));
        auto sweetRow = ha.removeFromBottom (22);
        sweetLow.setBounds  (sweetRow.removeFromLeft (sweetRow.getWidth() / 3));
        sweetZone.setBounds (sweetRow.removeFromLeft (sweetRow.getWidth() / 2));
        sweetHot.setBounds  (sweetRow);
        auto stageLedRow = ha.removeFromBottom (20);
        const int ledSlotW = juce::jmax (1, stageLedRow.getWidth() / static_cast<int> (auraBigStageLeds.size()));
        for (auto& led : auraBigStageLeds)
            led.setBounds (stageLedRow.removeFromLeft (ledSlotW).reduced (0, 1));

        // "AURA BIG" label (bold gold) + large aura knob centered in remaining space
        // Leave at least 22 px above the knob so the label never overlaps the panel title
        const int auraSize = juce::jmin (ha.getWidth(), ha.getHeight() - 24) - 8;
        const auto auraRect = ha.withSizeKeepingCentre (auraSize, auraSize);
        auraBigLabel.setBounds (juce::Rectangle<int> (
            auraRect.getX(), auraRect.getY() - 22, auraRect.getWidth(), 18));
        auraHeatRing.setBounds (auraRect.expanded (10));
        aura.setBounds (auraRect.reduced (6));
    }

    // ── RIGHT PANEL (DYNAMICS / OUTPUT) ──────────────────────────────────────
    {
        auto ra = right.reduced (kPanelPadding, kPanelTop);

        auto bypassRow = ra.removeFromTop (kBtnH);
        compBypassBtn.setBounds (bypassRow.removeFromRight (72).reduced (2));
        compressorEnable.setBounds (bypassRow.removeFromRight (72).reduced (2));

        compModelBar.setBounds (ra.removeFromTop (28));
        ra.removeFromTop (kGap);
        compProfileBar.setBounds (ra.removeFromTop (24));
        ra.removeFromTop (kGap);

        auto optionRow = ra.removeFromTop (kDropdownH);
        compTimingMode.setBounds (optionRow.removeFromLeft (optionRow.getWidth() / 2).reduced (2, 0));
        compScHpfMode.setBounds  (optionRow.reduced (2, 0));

        ra.removeFromTop (kGap);
        compGrMeter.setBounds (ra.removeFromTop (22).reduced (2, 0));
        compTargetGrLabel.setBounds (ra.removeFromTop (14).reduced (2, 0));
        ra.removeFromTop (kGap);

        const int compKH = juce::jmax (56, juce::jmin (80, (ra.getHeight() - 80) / 4));
        auto cr1 = ra.removeFromTop (compKH);
        layoutKnobGrid (cr1, 2, { &compInputKnob, &threshold });
        ra.removeFromTop (kGap);
        auto cr2 = ra.removeFromTop (compKH);
        layoutKnobGrid (cr2, 2, { &ratio, &attack });
        ra.removeFromTop (kGap);
        auto cr3 = ra.removeFromTop (compKH);
        layoutKnobGrid (cr3, 2, { &release, &compMixKnob });
        ra.removeFromTop (kGap);
        auto cr4 = ra.removeFromTop (compKH);
        layoutKnobGrid (cr4, 2, { &compDriveKnob, &compOutputKnob });

        compDensityKnob.setBounds (cr4.withWidth (0)); // placed via visibility; reuse row when visible
        compWarmthKnob.setBounds ({});
        compAmount.setBounds ({});
        compMakeup.setBounds ({});
        compSidechainKnob.setBounds ({});
        compModeButtons.setBounds ({});
        bleed.setBounds ({});
        vuMeter.setBounds ({});
        vuMode.setBounds ({});
        emotionLockBtn.setBounds ({});
        auraLevelBtn.setBounds ({});
        emotionLockStatusLabel.setBounds ({});
        auraLevelStateLabel.setBounds ({});

        if (ra.getHeight() >= 48)
        {
            const int oKH   = juce::jmin (compKH, ra.getHeight() - 4);
            auto oArea      = ra.removeFromTop (oKH);
            const int oSize = juce::jmin (oArea.getWidth() / 2 - 8, oKH - 4);
            outputKnob.setBounds (oArea.removeFromLeft (oArea.getWidth() / 2)
                                       .withSizeKeepingCentre (oSize, oSize));
            ceiling.setBounds    (oArea.withSizeKeepingCentre (oSize, oSize));
            width.setBounds (ra.removeFromTop (oKH).withSizeKeepingCentre (oSize, oSize));
        }

        updateCompressorControlVisibility();
    }

    // ── EQ STRIP ─────────────────────────────────────────────────────────────
    {
        auto ea  = eqPanel.getBounds().reduced (kPanelPadding, 28);
        const int kH = juce::jmin (56, ea.getHeight());

        // Far left: saturation + transformer
        {
            auto ll  = ea.removeFromLeft (120);
            const int hw = ll.getWidth() / 2;
            saturation.setBounds  (juce::Rectangle<int> (ll.getX(),      ll.getY(), hw, kH).withSizeKeepingCentre (50, kH));
            transformer.setBounds (juce::Rectangle<int> (ll.getX() + hw, ll.getY(), hw, kH).withSizeKeepingCentre (50, kH));
        }

        // Far right: limiter toggle + limiter GR meter (ceiling moved to right panel)
        {
            auto lb = ea.removeFromRight (110);
            limiter.setBounds (lb.removeFromTop (28).reduced (4, 2));
            lb.removeFromTop (kGap);
            limMeter.setBounds (lb.reduced (4, 2));
        }

        // Console / summing block
        {
            auto ca  = ea.removeFromLeft (140);
            const int hw = ca.getWidth() / 2;
            summing.setBounds (juce::Rectangle<int> (ca.getX(),      ca.getY(), hw, kH).withSizeKeepingCentre (50, kH));
            glue.setBounds    (juce::Rectangle<int> (ca.getX() + hw, ca.getY(), hw, kH).withSizeKeepingCentre (50, kH));
        }

        // EQ display — remaining center space
        eqDisplay.setBounds (ea.reduced (4, 0));
        if (eqDisplay.isExpanded())
            eqDisplay.setBounds (getLocalBounds().reduced (80, 60));
    }

    grMeter.setBounds ({});   // hidden — superseded by grHorizontalMeter

    // ── METER STRIP (vintage horizontal VU meters) ────────────────────────────
    {
        auto ma      = meterStrip.reduced (2, 4);
        auto qualRow = ma.removeFromBottom (22);
        qualityBar.setBounds (qualRow);
        inputVuMeter.setBounds      (ma.removeFromLeft (ma.getWidth() / 3).reduced (4, 6));
        grHorizontalMeter.setBounds (ma.removeFromLeft (ma.getWidth() / 2).reduced (4, 6));
        outputVuMeter.setBounds     (ma.reduced (4, 6));
    }

    // ── BOTTOM STRIP ─────────────────────────────────────────────────────────
    {
        auto fa = bottom.reduced (2, 4);
        inputFader.setBounds    (fa.removeFromLeft (120).reduced (4, 2));
        inputLrMeter.setBounds  (fa.removeFromLeft (62) .reduced (4, 2));
        mono.setBounds          (fa.removeFromLeft (56) .reduced (4, 10));
        bypass.setBounds        (fa.removeFromLeft (76) .reduced (4, 6));
        dim.setBounds           (fa.removeFromLeft (56) .reduced (4, 10));
        presets.setBounds       (fa.removeFromLeft (juce::jmax (180, fa.getWidth() / 3)).reduced (8, 10));
        undoBtn.setBounds       (fa.removeFromLeft (46) .reduced (4, 14));
        redoBtn.setBounds       (fa.removeFromLeft (46) .reduced (4, 14));
        outputLrMeter.setBounds  (fa.removeFromRight (62) .reduced (4, 2));
        outputFader.setBounds    (fa.removeFromRight (120).reduced (4, 2));
        oversamplingLabel.setBounds (fa.removeFromRight (130).reduced (2, 12));
        latencyLabel.setBounds      (fa.removeFromRight (120).reduced (2, 12));
    }

    eqDisplay.toFront (false);

    // Expanded AURA EQ overlay covers almost the entire editor
    if (expandedEQPanel != nullptr && expandedEQPanel->isVisible())
    {
        expandedEQPanel->setBounds (getLocalBounds().reduced (18, 14));
        expandedEQPanel->toFront (false);
    }
}

void StadiumAuraAudioProcessorEditor::timerCallback()
{
    // Keep input unmuted for the first ~3 s — JUCE's standalone audio device
    // state loads after the editor constructor and can silently re-mute input.
    if (startupUnmuteCountdown > 0)
    {
        --startupUnmuteCountdown;
        StandaloneAudio::forceUnmuteInput();

        // At ~1 s after startup: try to auto-configure the input device.
        // If no input device is found/activated, open the settings dialog.
        if (startupUnmuteCountdown == 120 && ! startupAutoInputTriggered)
        {
            startupAutoInputTriggered = true;
            const bool gotInput = StandaloneAudio::autoSelectDefaultInput();
            if (! gotInput)
                StandaloneAudio::showAudioSetup();
        }
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
    const float auraBigAmount = processorRef.apvts.getRawParameterValue ("AURA_BIG_AMOUNT")->load();
    const bool auraBigActive = auraBigAmount > 0.001f
        && processorRef.apvts.getRawParameterValue ("AURA_BIG_BYPASS")->load() <= 0.5f;

    if (auraBigActive)
    {
        const auto tubeHeat = processorRef.auraBigTubeHeat.load (std::memory_order_relaxed);
        const auto edgeHeat = processorRef.auraBigEdgeHeat.load (std::memory_order_relaxed);
        const auto ironHeat = processorRef.auraBigIronHeat.load (std::memory_order_relaxed);
        const auto combinedHeat = juce::jmax (tubeHeat, edgeHeat * 0.85f, ironHeat * 0.8f);
        tubeChamber.setActivity (juce::jlimit (0.0f, 1.0f, combinedHeat * 0.85f + auraBigAmount * 0.15f));
        tubeChamber.setDistortionLevel (juce::jlimit (0.0f, 1.0f, tubeHeat));
        auraHeatRing.setHeat (processorRef.auraBigGlobalHeat.load (std::memory_order_relaxed));
    }
    else
    {
        tubeChamber.setActivity (juce::jlimit (0.0f, 1.0f, tubeLevel * 0.72f + drive * 0.0028f));
        const auto distLevel = juce::jlimit (0.0f, 1.0f,
            (out > 0.90f ? (out - 0.90f) * 10.0f : 0.0f) + tubeLevel * 0.3f);
        tubeChamber.setDistortionLevel (distLevel);
        auraHeatRing.setHeat (0.0f);
    }

    const bool hardClip = processorRef.auraBigClipping.load (std::memory_order_relaxed)
        || processorRef.auraBigLimiterGrDb.load (std::memory_order_relaxed) < -9.0f;
    const bool amountZero = auraBigAmount <= 0.001f;
    const std::array<float, 9> stageHeats {{
        processorRef.auraBigStageIn.load (std::memory_order_relaxed),
        processorRef.auraBigStageTone.load (std::memory_order_relaxed),
        processorRef.auraBigStageTube.load (std::memory_order_relaxed),
        processorRef.auraBigStageEdge.load (std::memory_order_relaxed),
        processorRef.auraBigStageIron.load (std::memory_order_relaxed),
        processorRef.auraBigStageDensity.load (std::memory_order_relaxed),
        processorRef.auraBigStageAir.load (std::memory_order_relaxed),
        processorRef.auraBigStageWidth.load (std::memory_order_relaxed),
        processorRef.auraBigStageLimit.load (std::memory_order_relaxed)
    }};
    const std::array<bool, 9> stageBypass {{
        processorRef.auraBigBypassIn.load (std::memory_order_relaxed),
        processorRef.auraBigBypassTone.load (std::memory_order_relaxed),
        processorRef.auraBigBypassTube.load (std::memory_order_relaxed),
        processorRef.auraBigBypassEdge.load (std::memory_order_relaxed),
        processorRef.auraBigBypassIron.load (std::memory_order_relaxed),
        processorRef.auraBigBypassDensity.load (std::memory_order_relaxed),
        processorRef.auraBigBypassAir.load (std::memory_order_relaxed),
        processorRef.auraBigBypassWidth.load (std::memory_order_relaxed),
        processorRef.auraBigBypassLimit.load (std::memory_order_relaxed)
    }};
    for (size_t i = 0; i < auraBigStageLeds.size(); ++i)
    {
        const bool limitHard = i == 8 && processorRef.auraBigLimiterGrDb.load (std::memory_order_relaxed) < -9.0f;
        auraBigStageLeds[i].setState (stageHeats[i], amountZero || stageBypass[i], hardClip || limitHard);
    }

    // Horizontal VU meters
    inputVuMeter.setTargetDb  (juce::Decibels::gainToDecibels (in  + 1e-9f));
    grHorizontalMeter.setTargetDb (-gr);
    outputVuMeter.setTargetDb (juce::Decibels::gainToDecibels (out + 1e-9f));
    inputVuMeter.setDistortionWarning (in  > 0.93f);
    grHorizontalMeter.setDistortionWarning (gr > 14.0f);
    outputVuMeter.setDistortionWarning (out > 0.93f || gr > 10.0f || tubeLevel > 0.88f);
    grHorizontalMeter.setSaturation (juce::jlimit (0.0f, 1.0f, tubeLevel));

    updateEqDisplayState();

    // Refresh the expanded EQ panel when visible (30 Hz is sufficient for visual)
    if (expandedEQPanel != nullptr && expandedEQPanel->isVisible())
        expandedEQPanel->refreshFromParameters();

    presetCard.setText (processorRef.getProgramName (processorRef.getCurrentProgram()), juce::dontSendNotification);

    const auto qualityIndex = juce::jlimit (0, 3, static_cast<int> (processorRef.apvts.getRawParameterValue ("quality")->load()));
    qualityBar.setSelectedIndex (qualityIndex, juce::dontSendNotification);
    const auto trackIndex = juce::jlimit (1, 4, static_cast<int> (processorRef.apvts.getRawParameterValue ("trackCount")->load()));
    trackButtons.setSelectedIndex (trackIndex - 1, juce::dontSendNotification);
    const auto compIndex = juce::jlimit (0, 2, static_cast<int> (processorRef.apvts.getRawParameterValue ("compressorMode")->load()));
    compModeButtons.setSelectedIndex (compIndex, juce::dontSendNotification);

    const auto newCompIndex = juce::jlimit (0, 5, static_cast<int> (processorRef.apvts.getRawParameterValue ("COMP_MODEL")->load()));
    if (compModelBar.getSelectedIndex() != newCompIndex)
    {
        compModelBar.setSelectedIndex (newCompIndex, juce::dontSendNotification);
        refreshCompressorProfileBar();
        updateCompressorControlVisibility();
    }

    const auto modelIndex = juce::jlimit (0, 5, static_cast<int> (processorRef.apvts.getRawParameterValue ("COMP_MODEL")->load()));
    const int maxProfile = juce::jmax (0, getAuraCompressorProfileCount (static_cast<AuraCompressorModel> (modelIndex)) - 1);
    const auto profileIndex = juce::jlimit (0, maxProfile,
                                            static_cast<int> (processorRef.apvts.getRawParameterValue ("COMP_PROFILE")->load()));
    compProfileBar.setSelectedIndex (profileIndex, juce::dontSendNotification);

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
        const float grDisplay = juce::jlimit (0.0f, 20.0f, std::abs (newGr));
        grMeter.setTargetDb (grDisplay);
        compGrMeter.setTargetDb (grDisplay);
        grHorizontalMeter.setTargetDb (-grDisplay);
        const float targetGr = processorRef.newCompTargetGr.load (std::memory_order_relaxed);
        compTargetGrLabel.setText ("Target GR guide: " + juce::String (targetGr, 1) + " dB",
                                   juce::dontSendNotification);
    }
    else
    {
        compGrMeter.setTargetDb (gr);
        compTargetGrLabel.setText ("Target GR guide: —", juce::dontSendNotification);
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

    const bool useAuraBigSweetSpot = auraBigActive;

    if (useAuraBigSweetSpot)
    {
        const auto state = processorRef.auraBigSweetSpotState.load (std::memory_order_relaxed);
        const bool sweetSpot = state == 1;
        const bool hotSpot = state == 2;
        const bool clipping = state == 3;
        sweetZone.setText (sweetSpot ? "SWEET" : " ", juce::dontSendNotification);
        sweetZone.setColour (juce::Label::textColourId, sweetSpot ? juce::Colour (0xffffc15b) : juce::Colour (0x00000000));
        sweetLow.setColour (juce::Label::textColourId, state == 0 ? juce::Colour (0xff8d806c) : juce::Colour (0xff4d463c));
        sweetHot.setColour (juce::Label::textColourId, (hotSpot || clipping) ? juce::Colour (0xffff6b3a) : juce::Colour (0xff4d463c));
    }
    else
    {
        sweetZone.setText (sweet ? "SWEET" : " ", juce::dontSendNotification);
        sweetZone.setColour (juce::Label::textColourId, sweet ? juce::Colour (0xffffc15b) : juce::Colour (0x00000000));
        sweetLow.setColour (juce::Label::textColourId, ! sweet && ! hot ? juce::Colour (0xff8d806c) : juce::Colour (0xff4d463c));
        sweetHot.setColour (juce::Label::textColourId, hot ? juce::Colour (0xffff6b3a) : juce::Colour (0xff4d463c));
    }
}
