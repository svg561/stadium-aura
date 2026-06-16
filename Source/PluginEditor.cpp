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

constexpr int kDesignW = 1536;
constexpr int kDesignH = 920;

juce::Rectangle<int> designRect (float sx, float sy, int x0, int y0, int x1, int y1) noexcept
{
    return { juce::roundToInt (static_cast<float> (x0) * sx),
             juce::roundToInt (static_cast<float> (y0) * sy),
             juce::roundToInt (static_cast<float> (x1 - x0) * sx),
             juce::roundToInt (static_cast<float> (y1 - y0) * sy) };
}

void layoutKnobInCell (juce::Rectangle<int> cell, juce::Slider& knob, int knobPx) noexcept
{
    const int labelAndValue = 36;
    const int knobSize = juce::jmin (knobPx, cell.getWidth() - 4, cell.getHeight() - labelAndValue);
    knob.setBounds (cell.withSizeKeepingCentre (knobSize, knobSize + labelAndValue));
}

void layoutKnobRowInArea (juce::Rectangle<int> area, int columns, std::initializer_list<juce::Slider*> knobs, int knobPx)
{
    const auto n = static_cast<int> (knobs.size());
    if (n <= 0 || columns <= 0) return;
    const auto cellW = juce::jmax (1, area.getWidth() / columns);
    const auto cellH = juce::jmax (1, area.getHeight());
    int index = 0;
    for (auto* knob : knobs)
    {
        const auto col = index % columns;
        const auto row = index / columns;
        const juce::Rectangle<int> cell (area.getX() + col * cellW, area.getY() + row * cellH, cellW, cellH);
        layoutKnobInCell (cell.reduced (2, 1), *knob, knobPx);
        ++index;
    }
}
}

StadiumAuraAudioProcessorEditor::StadiumAuraAudioProcessorEditor (StadiumAuraAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&lookAndFeel);
    setResizable (true, true);
    setResizeLimits (1280, 760, 2560, 1200);
    setSize (1536, 920);
    aura.setHeroStyle (true);
    aura.setHideNameLabel (true);

    logoStadium.setText ("STADIUM", juce::dontSendNotification);
    logoStadium.setFont (juce::FontOptions (33.0f, juce::Font::bold).withKerningFactor (0.12f));
    logoStadium.setColour (juce::Label::textColourId, juce::Colour (0xffF2E6CC));
    logoStadium.setJustificationType (juce::Justification::centredLeft);
    logoAura.setText ("AURA", juce::dontSendNotification);
    logoAura.setFont (juce::FontOptions (33.0f, juce::Font::bold).withKerningFactor (0.12f));
    logoAura.setColour (juce::Label::textColourId, juce::Colour (0xffFF8A22));
    logoAura.setJustificationType (juce::Justification::centredLeft);
    logoSubtitle.setText ("ANALOG CREATIVE PROCESSOR", juce::dontSendNotification);
    logoSubtitle.setFont (juce::FontOptions (11.0f, juce::Font::bold).withKerningFactor (0.08f));
    logoSubtitle.setColour (juce::Label::textColourId, juce::Colour (0xffAFA79A));
    logoSubtitle.setJustificationType (juce::Justification::centredLeft);
    presetCard.setJustificationType (juce::Justification::centred);
    presetCard.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    presetCard.setColour (juce::Label::textColourId, juce::Colour (0xffFF8A22));
    presetCard.setColour (juce::Label::backgroundColourId, juce::Colour (0xff101418));
    factoryPresetLabel.setText ("Factory Preset", juce::dontSendNotification);
    factoryPresetLabel.setJustificationType (juce::Justification::centred);
    factoryPresetLabel.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    factoryPresetLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8d806c));
    utilityPresetLabel.setJustificationType (juce::Justification::centred);
    utilityPresetLabel.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    utilityPresetLabel.setColour (juce::Label::textColourId, juce::Colour (0xffe8c98d));
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
    for (auto* label : { &logoStadium, &logoAura, &logoSubtitle, &presetCard, &factoryPresetLabel,
                         &utilityPresetLabel, &latencyLabel, &oversamplingLabel, &monitorLabel })
        addAndMakeVisible (*label);

    sourceMicLabel.setText ("SOURCE MIC", juce::dontSendNotification);
    targetMicLabel.setText ("TARGET MIC", juce::dontSendNotification);
    for (auto* label : { &sourceMicLabel, &targetMicLabel })
    {
        label->setFont (juce::FontOptions (9.0f, juce::Font::bold).withKerningFactor (0.04f));
        label->setColour (juce::Label::textColourId, juce::Colour (0xff8d806c));
        label->setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (*label);
    }

    for (auto* panel : { &micSourcePanel, &micCharPanel, &preampPanel, &bigAuraPanel, &compressorPanel,
                         &saturationPanel, &transformerPanel, &consolePanel, &gluePanel,
                         &eqStripPanel, &widthPanel, &limiterPanel })
        addAndMakeVisible (*panel);

    const std::pair<juce::Slider*, const char*> sliders[] {
        { &inputKnob, "input" }, { &outputKnob, "output" },
        { &inputFader, "input" }, { &outputFader, "output" },
        { &bodyKnob, "MIC_CHAR_BODY" }, { &presenceKnob, "MIC_CHAR_PRESENCE" }, { &airKnob, "MIC_CHAR_AIR" },
        { &micCharAmountKnob, "MIC_CHAR_COLOR" }, { &micCharOutputKnob, "MIC_CHAR_OUTPUT_TRIM" },
        { &micCharInputTrimKnob, "MIC_CHAR_INPUT_TRIM" }, { &micCharProximityKnob, "MIC_CHAR_PROXIMITY" },
        { &micCharDeHarshKnob, "MIC_CHAR_DEHARSH" }, { &micCharSibilanceKnob, "MIC_CHAR_SIBILANCE" },
        { &saturation, "saturation" }, { &tubeBias, "harmonicBias" },
        { &transformer, "transformer" }, { &summing, "summing" }, { &glue, "glue" },
        { &correction, "micCorrectionAmount" }, { &targetAmount, "micTargetAmount" }, { &badFreq, "badFrequencyTamer" },
        { &bodyProtectKnob, "bodyProtection" }, { &airProtectKnob, "airProtection" },
        { &preampDrive, "preampDrive" }, { &preampToneKnob, "tone" }, { &preampOutputKnob, "tubeOutputDb" },
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
    aura.textFromValueFunction = [] (double value)
    {
        return juce::String (juce::jlimit (0.0, 100.0, value * 100.0), 0) + " %";
    };
    aura.valueFromTextFunction = [] (const juce::String& text)
    {
        return text.trim().trimCharactersAtEnd ("%").getDoubleValue() / 100.0;
    };
    sliderAttachments.push_back (std::make_unique<SliderAttachment> (processorRef.apvts, "AURA_BIG_AMOUNT", aura));

    addAndMakeVisible (tubeDriveFader);
    sliderAttachments.push_back (std::make_unique<SliderAttachment> (processorRef.apvts, "tubeDrive", tubeDriveFader));
    tubeDriveKnob.setVisible (false);

    attachButton (micCharBypass, "MIC_CHAR_BYPASS");
    attachButton (micCharSimpleMode, "MIC_CHAR_SIMPLE_MODE");
    attachButton (hardwareSafe, "hardwareSafeMode");
    attachButton (compressorEnable, "COMP_ENABLED");
    attachButton (compBypassBtn, "COMP_BYPASS");
    attachButton (limiter, "limiter");
    attachButton (bypass, "bypass");
    attachButton (eqEnableBtn, "eqEnable");
    attachButton (mono, "monoCheck");
    attachButton (dim, "dim");
    attachButton (emotionLockBtn, "EMOTION_LOCK_ENABLED");
    attachButton (auraLevelBtn,   "COMP_AURA_LEVEL");

    attachCombo (micCharProfile, "MIC_CHAR_PROFILE", {
        "Aura Vintage 87", "Aura Silk Tube", "Aura Golden 251", "Aura Crystal 12", "Aura Broadcast 7",
        "Aura Modern Pop", "Aura Warm Rap", "Aura Female Air", "Aura Male Body", "Aura Clean Capture" });
    attachCombo (sourceMic, "sourceMicMode", { "Unknown / Auto", "Dynamic General", "Condenser General",
        "Ribbon General", "57-Style Dynamic", "7B-Style Dynamic", "C80-Style Condenser", "Bright Condenser",
        "Dark Condenser", "Warm Tube Mic", "Flat / Measurement" });
    attachCombo (targetMic, "targetMicMode", { "67 Vintage Smooth", "C12 Open Air", "251 Classic Silk",
        "87 Modern Balanced", "47 Velvet Tube", "251E Silky Presence", "800G Air Pop" });
    attachCombo (preampMode, "preampMode", { "Iron 73", "American Punch", "Avalon Clean" });
    attachCombo (tubeType, "tubeType", { "Clean Triode", "Warm Triode", "Hot Triode", "Vintage Pentode", "Big Bottle", "Cream Opto Tube" });
    attachCombo (vuMode, "vuMeterMode", { "IN", "GR", "OUT" });
    attachCombo (consoleMode, "consoleMode", { "Vintage Desk", "Clean Line", "Large Format 9000", "British Iron", "American Punch", "Live Gold", "Velvet Desk" });
    attachCombo (compTimingMode, "COMP_TIMING_MODE", { "Fixed", "Manual", "Hybrid" });
    attachCombo (compScHpfMode, "COMP_SC_HPF_MODE", { "Off", "80 Hz", "150 Hz", "220 Hz" });
    attachCombo (compModeCombo, "compressorMode", { "FAST FET", "SMOOTH OPTO", "CROWN MU" });
    attachCombo (compModelCombo, "COMP_MODEL", { "Aura 2A", "Aura 76", "Aura Tube", "Aura Limiter", "Aura Density", "Aura Drums" });

    consoleModeLabel.setText ("MODE", juce::dontSendNotification);
    consoleTracksLabel.setText ("TRACKS", juce::dontSendNotification);
    for (auto* label : { &consoleModeLabel, &consoleTracksLabel })
    {
        label->setFont (juce::FontOptions (8.5f, juce::Font::bold).withKerningFactor (0.04f));
        label->setColour (juce::Label::textColourId, juce::Colour (0xff8d806c));
        label->setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (*label);
    }

    compModeLabel.setText ("MODE", juce::dontSendNotification);
    compMeterLabel.setText ("METER", juce::dontSendNotification);
    compModelLabel.setText ("MODEL", juce::dontSendNotification);
    compProfileLabel.setText ("PROFILE", juce::dontSendNotification);
    for (auto* label : { &compModeLabel, &compMeterLabel, &compModelLabel, &compProfileLabel })
    {
        label->setFont (juce::FontOptions (9.0f, juce::Font::bold).withKerningFactor (0.04f));
        label->setColour (juce::Label::textColourId, juce::Colour (0xff8d806c));
        label->setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (*label);
    }

    addAndMakeVisible (compProfileCombo);
    compProfileCombo.onChange = [this]
    {
        setChoiceParameter ("COMP_PROFILE", compProfileCombo.getSelectedItemIndex());
    };
    compModelCombo.onChange = [this]
    {
        refreshCompressorProfileCombo();
        updateCompressorControlVisibility();
        resized();
    };
    refreshCompressorProfileCombo();
    updateCompressorControlVisibility();

    for (auto* c : { static_cast<juce::Component*> (&inputRms), static_cast<juce::Component*> (&outputRms),
                     static_cast<juce::Component*> (&inputLrMeter), static_cast<juce::Component*> (&outputLrMeter),
                     static_cast<juce::Component*> (&grMeter), static_cast<juce::Component*> (&limMeter),
                     static_cast<juce::Component*> (&vuMeter), static_cast<juce::Component*> (&tubeChamber),
                     static_cast<juce::Component*> (&eqDisplay), static_cast<juce::Component*> (&trackButtons),
                     static_cast<juce::Component*> (&qualityBar), static_cast<juce::Component*> (&compMeterTapBar),
                     static_cast<juce::Component*> (&compGrMeter) })
        addAndMakeVisible (*c);

    addAndMakeVisible (micCharBypass);
    addAndMakeVisible (micCharSimpleMode);
    addAndMakeVisible (micCharHpfBtn);
    micCharHpfBtn.setTooltip ("High-pass filter — TODO: bind when dedicated HPF toggle param exists.");
    addAndMakeVisible (preampPolarityBtn);
    preampPolarityBtn.setTooltip ("Input polarity invert — TODO: bind when polarity param exists.");
    micCharSimpleMode.onClick = [this]
    {
        updateMicCharacterControlVisibility();
        resized();
    };
    updateMicCharacterControlVisibility();
    addAndMakeVisible (eqEnableBtn);
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

    auraBigLabel.setText ("BIG AURA", juce::dontSendNotification);
    auraBigLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold).withKerningFactor (0.10f));
    auraBigLabel.setColour (juce::Label::textColourId, juce::Colour (0xffc8962e));
    auraBigLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (auraBigLabel);

    bigAuraSubtitle.setText ("MAGIC CONTROL", juce::dontSendNotification);
    bigAuraSubtitle.setFont (juce::FontOptions (10.0f, juce::Font::bold).withKerningFactor (0.06f));
    bigAuraSubtitle.setColour (juce::Label::textColourId, juce::Colour (0xff8d806c));
    bigAuraSubtitle.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (bigAuraSubtitle);

    sweetZoneTitle.setText ("SWEET ZONE", juce::dontSendNotification);
    sweetZoneTitle.setFont (juce::FontOptions (8.5f, juce::Font::bold).withKerningFactor (0.06f));
    sweetZoneTitle.setColour (juce::Label::textColourId, juce::Colour (0xff8d806c));
    sweetZoneTitle.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (sweetZoneTitle);

    tubeTypeLabel.setText ("TUBE TYPE", juce::dontSendNotification);
    tubeTypeLabel.setFont (juce::FontOptions (8.5f, juce::Font::bold).withKerningFactor (0.04f));
    tubeTypeLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8d806c));
    tubeTypeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (tubeTypeLabel);

    saMarkLabel.setText ("SA", juce::dontSendNotification);
    saMarkLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    saMarkLabel.setColour (juce::Label::textColourId, juce::Colour (0xffc8962e));
    saMarkLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (saMarkLabel);
    saMarkLabel.setVisible (false);
    addAndMakeVisible (auraHeatRing);
    auraHeatRing.toBack();

    for (auto& led : auraBigStageLeds)
        addAndMakeVisible (led);

    for (auto* c : { static_cast<juce::Component*> (&inputVuMeter),
                     static_cast<juce::Component*> (&grHorizontalMeter),
                     static_cast<juce::Component*> (&outputVuMeter) })
        addAndMakeVisible (*c);

    trackButtons.setChoices ({ "8 Tracks", "16 Tracks", "24 Tracks", "32 Tracks" });
    trackButtons.setSelectedIndex (2, juce::dontSendNotification);
    trackButtons.onChange = [this] (int index)
    {
        setChoiceParameter ("trackCount", index + 1);
    };

    qualityBar.setChoices ({ "ECO", "NORMAL", "HIGH", "ULTRA" });
    qualityBar.setSelectedIndex (2, juce::dontSendNotification);
    qualityBar.onChange = [this] (int index) { setChoiceParameter ("quality", index); };

    compMeterTapBar.setChoices ({ "IN", "GR", "OUT" });
    compMeterTapBar.setSelectedIndex (1, juce::dontSendNotification);
    compMeterTapBar.onChange = [this] (int index) { setChoiceParameter ("vuMeterMode", index); };

    transformer.setTooltip ("Adds modeled iron weight and low-mid density.");
    aura.setTooltip ("Signature harmonic tone and sweet-zone intensity.");
    tubeBias.setTooltip ("Shifts tube response from cleaner to richer.");
    micCharDeHarshKnob.setTooltip ("Reduces sharp upper-mid vocal harshness.");
    bodyProtectKnob.setTooltip ("Protects vocal thickness while correcting tone.");
    airProtectKnob.setTooltip ("Preserves top-end clarity while smoothing harshness.");
    bypass.setTooltip ("Global plugin bypass.");
    eqEnableBtn.setTooltip ("Enable or disable the Aura EQ processing chain.");
    compressorEnable.setTooltip ("Enable or disable the compressor engine.");
    limiter.setTooltip ("Enable or disable the safety limiter.");

    eqDisplay.setCompactMode (true);
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
    eqDisplay.onEmptyAreaClicked = [this] { openExpandedEq(); };

    eqModalBackdrop = std::make_unique<EqModalBackdrop>();
    addChildComponent (*eqModalBackdrop);
    eqModalBackdrop->setVisible (false);

    // Create and add the expanded EQ panel (initially hidden)
    expandedEQPanel = std::make_unique<ExpandedEQPanel> (processorRef);
    addChildComponent (*expandedEQPanel);
    expandedEQPanel->setVisible (false);
    expandedEQPanel->onClose = [this] { closeExpandedEq(); };

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
    {
        if (processorRef.isFirstPresetInGroup (i))
            presets.addSectionHeading (processorRef.getFactoryPresetGroupName (i));
        presets.addItem (processorRef.getProgramName (i), i + 1);
    }
    presets.setSelectedItemIndex (processorRef.getCurrentProgram(), juce::dontSendNotification);
    presets.onChange = [this]
    {
        processorRef.setCurrentProgram (presets.getSelectedItemIndex());
        const auto name = processorRef.getProgramName (processorRef.getCurrentProgram());
        presetCard.setText (name, juce::dontSendNotification);
        utilityPresetLabel.setText (name, juce::dontSendNotification);
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

void StadiumAuraAudioProcessorEditor::openExpandedEq()
{
    if (expandedEQPanel == nullptr)
        return;
    eqModalBackdrop->setBounds (getLocalBounds());
    eqModalBackdrop->setVisible (true);
    eqModalBackdrop->toBack();
    expandedEQPanel->setVisible (true);
    expandedEQPanel->toFront (false);
    expandedEQPanel->grabKeyboardFocus();
    setWantsKeyboardFocus (true);
    resized();
}

void StadiumAuraAudioProcessorEditor::closeExpandedEq()
{
    if (expandedEQPanel == nullptr)
        return;
    expandedEQPanel->setVisible (false);
    eqModalBackdrop->setVisible (false);
    resized();
}

bool StadiumAuraAudioProcessorEditor::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey && expandedEQPanel != nullptr && expandedEQPanel->isVisible())
    {
        closeExpandedEq();
        return true;
    }
    return AudioProcessorEditor::keyPressed (key);
}

int StadiumAuraAudioProcessorEditor::scaledKnobSize (int minPx, int maxPx, float scale) const noexcept
{
    return juce::jlimit (minPx, maxPx, juce::roundToInt (static_cast<float> (maxPx) * scale));
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

void StadiumAuraAudioProcessorEditor::refreshCompressorProfileCombo()
{
    const auto modelIndex = juce::jlimit (0, 5, static_cast<int> (processorRef.apvts.getRawParameterValue ("COMP_MODEL")->load()));
    const auto model = static_cast<AuraCompressorModel> (modelIndex);
    const int count = getAuraCompressorProfileCount (model);
    const int profileIndex = juce::jlimit (0, juce::jmax (0, count - 1),
                                           static_cast<int> (processorRef.apvts.getRawParameterValue ("COMP_PROFILE")->load()));

    compProfileCombo.clear (juce::dontSendNotification);
    for (int i = 0; i < count; ++i)
        compProfileCombo.addItem (getAuraCompressorProfileName (model, i), i + 1);

    compProfileCombo.setSelectedItemIndex (profileIndex, juce::dontSendNotification);
    const bool showProfile = count > 1;
    compProfileCombo.setVisible (showProfile);
    compProfileLabel.setVisible (showProfile);
}

void StadiumAuraAudioProcessorEditor::updateMicCharacterControlVisibility()
{
    // Grid spec shows AMOUNT / PROXIMITY / DE-HARSH always; legacy simple-mode extras stay hidden.
    micCharInputTrimKnob.setVisible (false);
    micCharSibilanceKnob.setVisible (false);
    bodyKnob.setVisible (false);
    presenceKnob.setVisible (false);
    airKnob.setVisible (false);
    micCharOutputKnob.setVisible (false);
}

void StadiumAuraAudioProcessorEditor::updateCompressorControlVisibility()
{
    const auto modelIndex = juce::jlimit (0, 5, static_cast<int> (processorRef.apvts.getRawParameterValue ("COMP_MODEL")->load()));
    const auto model = static_cast<AuraCompressorModel> (modelIndex);
    compTimingMode.setVisible (model == AuraCompressorModel::AuraTube);
    compScHpfMode.setVisible (model == AuraCompressorModel::AuraTube || model == AuraCompressorModel::AuraDrums);
    compSidechainKnob.setVisible (model == AuraCompressorModel::AuraTube || model == AuraCompressorModel::AuraDrums);
    compWarmthKnob.setVisible (model == AuraCompressorModel::Aura2A || model == AuraCompressorModel::AuraDensity);
    compDensityKnob.setVisible (model == AuraCompressorModel::AuraDensity);
    compDriveKnob.setVisible (model != AuraCompressorModel::AuraLimiter);
    compAmount.setVisible (model == AuraCompressorModel::Aura2A || model == AuraCompressorModel::AuraDensity);
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
    setPanel (micSourcePanel, route == 0, route != 0);
    setPanel (micCharPanel, route == 0, route != 0);
    setPanel (preampPanel, route == 1, route != 1);
    setPanel (bigAuraPanel, route == 3 || route == 5, route != 3 && route != 5);
    setPanel (compressorPanel, route == 2, route != 2);

    const char* routeNames[] { "MIC", "PRE", "COMP", "HARMONICS", "SUM", "MASTER", "OUTPUT" };
    for (int i = 0; i < 6; ++i)
    {
        const auto enabled = isSectionEnabled (i);
        routing[static_cast<size_t> (i)].setAlpha (enabled ? 1.0f : 0.55f);
        routing[static_cast<size_t> (i)].setButtonText (routeNames[i] + juce::String (enabled ? "" : " OFF"));
    }
    routing[6].setAlpha (focusedRoute == 6 ? 1.0f : 0.88f);
    routing[6].setButtonText ("OUTPUT");
}

void StadiumAuraAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto frame = getLocalBounds().toFloat().reduced (8.0f);
    RackDrawing::paintBrushedMetal (g, frame);
    g.setColour (RackDrawing::Palette::accentGold().withAlpha (0.35f));
    g.drawRoundedRectangle (frame, 8.0f, 2.0f);
    RackDrawing::paintScrews (g, frame, 14.0f);

    const float sx = static_cast<float> (getWidth()) / static_cast<float> (kDesignW);
    const float sy = static_cast<float> (getHeight()) / static_cast<float> (kDesignH);
    const auto drawDivider = [&] (int designY)
    {
        const float y = frame.getY() + static_cast<float> (designY) * sy;
        g.setColour (juce::Colour (0x18ffffff));
        g.drawHorizontalLine (juce::roundToInt (y), frame.getX() + 20.0f, frame.getRight() - 20.0f);
    };
    drawDivider (92);
    drawDivider (675);
    drawDivider (680);
    drawDivider (852);
    drawDivider (858);

    const auto utility = designRect (sx, sy, 0, 858, kDesignW, 920).toFloat().reduced (2.0f);
    g.setColour (juce::Colour (0xff0d1013));
    g.fillRoundedRectangle (utility, 5.0f);
    g.setColour (juce::Colour (0xff3a2f22));
    g.drawRoundedRectangle (utility, 5.0f, 1.0f);
}

void StadiumAuraAudioProcessorEditor::resized()
{
    const float sx = static_cast<float> (getWidth()) / static_cast<float> (kDesignW);
    const float sy = static_cast<float> (getHeight()) / static_cast<float> (kDesignH);
    constexpr int kPanelPad = 10;
    constexpr int kPanelTop = 28;
    constexpr int kDropH = 34;
    constexpr int kFieldLabelH = 14;
    constexpr int kButtonH = 26;

    // ── TOP HEADER y 0–92 ─────────────────────────────────────────────────────
    logoStadium.setBounds      (designRect (sx, sy, 0, 8, 200, 52));
    logoAura.setBounds         (designRect (sx, sy, 200, 8, 330, 52));
    logoSubtitle.setBounds     (designRect (sx, sy, 0, 52, 330, 88));
    favoriteBtn.setBounds      (designRect (sx, sy, 340, 24, 370, 64));
    presetPrev.setBounds       (designRect (sx, sy, 375, 24, 405, 64));
    presetNext.setBounds       (designRect (sx, sy, 410, 24, 440, 64));
    presetCard.setBounds       (designRect (sx, sy, 448, 24, 620, 48));
    factoryPresetLabel.setBounds (designRect (sx, sy, 448, 48, 620, 68));
    saveBtn.setBounds          (designRect (sx, sy, 628, 24, 678, 64));
    abA.setBounds              (designRect (sx, sy, 690, 24, 722, 64));
    abB.setBounds              (designRect (sx, sy, 724, 24, 756, 64));
    undoBtn.setBounds          (designRect (sx, sy, 764, 24, 820, 64));
    routing[0].setBounds       (designRect (sx, sy, 840, 24, 908, 64));
    routing[1].setBounds       (designRect (sx, sy, 912, 24, 980, 64));
    routing[2].setBounds       (designRect (sx, sy, 984, 24, 1058, 64));
    routing[3].setBounds       (designRect (sx, sy, 1062, 24, 1158, 64));
    routing[4].setBounds       (designRect (sx, sy, 1162, 24, 1230, 64));
    routing[5].setBounds       (designRect (sx, sy, 1234, 24, 1310, 64));
    routing[6].setBounds       (designRect (sx, sy, 1314, 24, 1390, 64));
    settingsBtn.setBounds      (designRect (sx, sy, 1400, 24, 1440, 64));
    helpBtn.setBounds          (designRect (sx, sy, 1444, 24, 1484, 64));
    saMarkLabel.setBounds      (designRect (sx, sy, 1490, 24, 1536, 64));
    saMarkLabel.setVisible (true);
    monitorLabel.setBounds     (designRect (sx, sy, 840, 68, 1390, 88));
    presets.setVisible (false);
    redoBtn.setBounds ({});

    // ── MAIN CONSOLE y 100–675 — 7 columns ────────────────────────────────────
    micSourcePanel.setBounds  (designRect (sx, sy, 0, 100, 230, 675));
    micCharPanel.setBounds    (designRect (sx, sy, 317, 100, 458, 675));
    preampPanel.setBounds     (designRect (sx, sy, 460, 100, 592, 675));
    bigAuraPanel.setBounds    (designRect (sx, sy, 596, 100, 988, 675));
    compressorPanel.setBounds (designRect (sx, sy, 992, 100, 1416, 675));
    outputRms.setBounds       (designRect (sx, sy, 1418, 100, 1536, 675));

    // Col 1 — MIC SOURCE
    {
        auto col = micSourcePanel.getBounds().reduced (kPanelPad, kPanelTop);
        sourceMicLabel.setBounds (col.removeFromTop (kFieldLabelH));
        col.removeFromTop (2);
        sourceMic.setBounds (col.removeFromTop (kDropH));
        col.removeFromTop (6);
        targetMicLabel.setBounds (col.removeFromTop (kFieldLabelH));
        col.removeFromTop (2);
        targetMic.setBounds (col.removeFromTop (kDropH));
        col.removeFromTop (6);
        auto knobRow = col.removeFromTop (juce::jmin (col.getHeight() / 2, juce::roundToInt (110 * sy)));
        layoutKnobRowInArea (knobRow, 2, { &correction, &targetAmount }, juce::roundToInt (72 * sy));
        col.removeFromTop (4);
        auto knobRow2 = col.removeFromTop (juce::jmin (col.getHeight() / 2, juce::roundToInt (110 * sy)));
        layoutKnobRowInArea (knobRow2, 2, { &badFreq, &bodyProtectKnob }, juce::roundToInt (72 * sy));
        col.removeFromTop (4);
        layoutKnobInCell (col.removeFromTop (juce::jmin (col.getHeight() / 2, juce::roundToInt (96 * sy))),
                          airProtectKnob, juce::roundToInt (68 * sy));
        col.removeFromTop (4);
        analyzeSource.setBounds (col.removeFromTop (kButtonH).reduced (2, 0));
        col.removeFromTop (4);
        hardwareSafe.setBounds (col.removeFromTop (kButtonH).reduced (2, 0));
    }

    // Col 2 — INPUT meter + IN TRIM
    {
        auto meterBounds = designRect (sx, sy, 232, 100, 315, 675).reduced (4, 8);
        auto trimArea = meterBounds.removeFromBottom (juce::roundToInt (100 * sy));
        layoutKnobInCell (trimArea, micCharInputTrimKnob, juce::roundToInt (64 * sy));
        inputRms.setBounds (meterBounds);
    }

    // Col 3 — MIC CHARACTER
    {
        auto col = micCharPanel.getBounds().reduced (kPanelPad, kPanelTop);
        micCharProfile.setBounds (col.removeFromTop (kDropH));
        col.removeFromTop (6);
        auto knobs = col.removeFromTop (juce::jmax (120, col.getHeight() - kButtonH - 8));
        layoutKnobRowInArea (knobs, 1, { &micCharAmountKnob, &micCharProximityKnob, &micCharDeHarshKnob },
                             juce::roundToInt (68 * sy));
        micCharHpfBtn.setBounds (col.removeFromTop (kButtonH).reduced (2, 0));
        micCharBypass.setVisible (false);
        micCharSimpleMode.setVisible (false);
    }

    // Col 4 — PREAMP
    {
        auto col = preampPanel.getBounds().reduced (kPanelPad, kPanelTop);
        preampMode.setBounds (col.removeFromTop (kDropH));
        col.removeFromTop (6);
        auto knobs = col.removeFromTop (juce::jmax (130, col.getHeight() - kButtonH - 8));
        layoutKnobRowInArea (knobs, 1, { &preampDrive, &preampToneKnob, &preampOutputKnob }, juce::roundToInt (68 * sy));
        preampPolarityBtn.setBounds (col.removeFromTop (kButtonH).reduced (2, 0));
    }

    // Col 5 — BIG AURA
    {
        auto col = bigAuraPanel.getBounds().reduced (kPanelPad, kPanelTop);
        auto rightStrip = col.removeFromRight (juce::roundToInt (92 * sx));

        auto chamberRow = col.removeFromBottom (juce::roundToInt (118 * sy));
        auto tubeTypeCol = chamberRow.removeFromRight (juce::roundToInt (108 * sx)).reduced (2, 4);
        tubeTypeLabel.setBounds (tubeTypeCol.removeFromTop (kFieldLabelH));
        tubeType.setBounds (tubeTypeCol.removeFromTop (kDropH).reduced (0, 2));
        tubeChamber.setBounds (chamberRow.reduced (2, 2));

        auto sweetRow = col.removeFromBottom (juce::roundToInt (24 * sy));
        sweetLow.setBounds  (sweetRow.removeFromLeft (sweetRow.getWidth() / 3).reduced (2, 0));
        sweetZone.setBounds (sweetRow.removeFromLeft (sweetRow.getWidth() / 2).reduced (2, 0));
        sweetHot.setBounds  (sweetRow.reduced (2, 0));
        sweetZoneTitle.setBounds (col.removeFromBottom (juce::roundToInt (14 * sy)).reduced (48, 0));

        trackButtons.setBounds ({});
        tubeDriveKnob.setBounds ({});
        tubeDriveFader.setBounds (rightStrip.removeFromTop (juce::roundToInt (210 * sy)).reduced (2, 4));
        layoutKnobInCell (rightStrip.reduced (2, 4), tubeBias, juce::roundToInt (56 * sy));

        bigAuraSubtitle.setBounds (col.removeFromTop (juce::roundToInt (16 * sy)));
        col.removeFromTop (2);
        auraBigLabel.setBounds (col.removeFromTop (juce::roundToInt (18 * sy)));
        col.removeFromTop (4);

        const int auraKnobPx = juce::jlimit (170, 200, juce::roundToInt (188.0f * juce::jmin (sx, sy)));
        const int labelValueH = 40;
        auto auraArea = col.withSizeKeepingCentre (auraKnobPx, auraKnobPx + labelValueH);
        auraHeatRing.setBounds (auraArea.expanded (juce::roundToInt (10 * sx)));
        aura.setBounds (auraArea);
        aura.toFront (false);
        auraHeatRing.toBack();
        tubeChamber.toBack();

        for (auto& led : auraBigStageLeds)
            led.setBounds ({});
    }

    // Col 6 — COMPRESSOR
    {
        auto col = compressorPanel.getBounds().reduced (kPanelPad, kPanelTop);
        refreshCompressorProfileCombo();

        auto modeLabelRow = col.removeFromTop (kFieldLabelH);
        compModeLabel.setBounds (modeLabelRow.removeFromLeft (modeLabelRow.getWidth() / 2).reduced (2, 0));
        compMeterLabel.setBounds (modeLabelRow.reduced (2, 0));
        col.removeFromTop (2);
        auto modeComboRow = col.removeFromTop (kDropH);
        compModeCombo.setBounds (modeComboRow.removeFromLeft (modeComboRow.getWidth() / 2).reduced (2, 0));
        vuMode.setBounds (modeComboRow.reduced (2, 0));
        col.removeFromTop (4);

        compModelLabel.setVisible (false);
        compModelCombo.setVisible (false);
        compProfileLabel.setVisible (false);
        compProfileCombo.setVisible (false);
        compModelLabel.setBounds ({});
        compModelCombo.setBounds ({});
        compProfileLabel.setBounds ({});
        compProfileCombo.setBounds ({});

        compGrMeter.setBounds ({});
        vuMeter.setBounds (col.removeFromTop (juce::roundToInt (112 * sy)).reduced (6, 2));
        col.removeFromTop (4);

        updateCompressorControlVisibility();
        compTimingMode.setVisible (true);
        auto knobArea = col.removeFromTop (juce::jmax (148, col.getHeight() - kButtonH * 2 - 12));
        const int knobPx = juce::roundToInt (58 * sy);
        auto row1 = knobArea.removeFromTop (knobArea.getHeight() / 2);
        layoutKnobRowInArea (row1, 4, { &threshold, &ratio, &attack, &release }, knobPx);
        knobArea.removeFromTop (2);
        auto row2 = knobArea.removeFromTop (knobArea.getHeight());
        const auto timingCol = row2.removeFromRight (juce::roundToInt (72 * sx)).reduced (2, 0);
        compTimingMode.setBounds (timingCol);
        layoutKnobRowInArea (row2, 3, { &compOutputKnob, &bleed, &compSidechainKnob }, knobPx);

        auto statusRow = col.removeFromTop (kButtonH + 4);
        compMeterTapBar.setBounds (statusRow.removeFromLeft (juce::roundToInt (150 * sx)).reduced (2, 0));
        compressorEnable.setBounds (statusRow.removeFromRight (juce::roundToInt (78 * sx)).reduced (2, 0));

        compBypassBtn.setBounds ({});
        emotionLockBtn.setBounds ({});
        auraLevelBtn.setBounds ({});
        compScHpfMode.setBounds ({});
        compInputKnob.setBounds ({});
        compAmount.setBounds ({});
        compDriveKnob.setBounds ({});
        compMixKnob.setBounds ({});
        compDensityKnob.setBounds ({});
        compWarmthKnob.setBounds ({});
        compTargetGrLabel.setBounds ({});
        inputVuMeter.setBounds ({});
        grHorizontalMeter.setBounds ({});
        outputVuMeter.setBounds ({});
        grMeter.setBounds ({});
        outputKnob.setBounds ({});
    }

    // ── BOTTOM PROCESSING STRIP y 680–852 ─────────────────────────────────────
    saturationPanel.setBounds  (designRect (sx, sy, 0, 680, 143, 852));
    transformerPanel.setBounds (designRect (sx, sy, 145, 680, 282, 852));
    consolePanel.setBounds     (designRect (sx, sy, 284, 680, 488, 852));
    gluePanel.setBounds        (designRect (sx, sy, 490, 680, 595, 852));
    eqStripPanel.setBounds     (designRect (sx, sy, 598, 680, 1068, 852));
    widthPanel.setBounds       (designRect (sx, sy, 1070, 680, 1202, 852));
    limiterPanel.setBounds     (designRect (sx, sy, 1204, 680, 1536, 852));

    for (auto* panel : { &saturationPanel, &transformerPanel, &consolePanel, &gluePanel,
                         &eqStripPanel, &widthPanel, &limiterPanel })
        panel->toBack();

    constexpr int kStripTop = 26;
    constexpr int kStripBottomRow = 28;

    layoutKnobInCell (saturationPanel.getBounds().reduced (6, kStripTop).withTrimmedBottom (kStripBottomRow),
                      saturation, juce::roundToInt (78 * sy));
    layoutKnobInCell (transformerPanel.getBounds().reduced (6, kStripTop).withTrimmedBottom (kStripBottomRow),
                      transformer, juce::roundToInt (78 * sy));

    {
        auto consoleArea = consolePanel.getBounds().reduced (8, kStripTop).withTrimmedBottom (kStripBottomRow);
        auto leftCol = consoleArea.removeFromLeft (consoleArea.getWidth() / 2).reduced (2, 0);
        consoleModeLabel.setBounds (leftCol.removeFromTop (kFieldLabelH));
        leftCol.removeFromTop (2);
        consoleMode.setBounds (leftCol.removeFromTop (kDropH));
        leftCol.removeFromTop (4);
        consoleTracksLabel.setBounds (leftCol.removeFromTop (kFieldLabelH));
        leftCol.removeFromTop (2);
        trackButtons.setBounds (leftCol.removeFromTop (kDropH));
        layoutKnobInCell (consoleArea.reduced (2, 0), summing, juce::roundToInt (68 * sy));
    }

    layoutKnobInCell (gluePanel.getBounds().reduced (6, kStripTop).withTrimmedBottom (kStripBottomRow),
                      glue, juce::roundToInt (72 * sy));
    eqDisplay.setBounds (eqStripPanel.getBounds().reduced (4, kStripTop).withTrimmedBottom (4));

    {
        auto widthArea = widthPanel.getBounds().reduced (6, kStripTop).withTrimmedBottom (kStripBottomRow);
        auto knobArea = widthArea.removeFromTop (juce::jmax (80, widthArea.getHeight() - kButtonH - 4));
        layoutKnobInCell (knobArea, width, juce::roundToInt (64 * sy));
        widthArea.removeFromTop (4);
        mono.setBounds (widthArea.removeFromBottom (kButtonH).reduced (8, 0));
    }

    {
        auto limArea = limiterPanel.getBounds().reduced (8, kStripTop).withTrimmedBottom (kStripBottomRow);
        auto meterCol = limArea.removeFromRight (juce::roundToInt (56 * sx));
        limMeter.setBounds (meterCol.reduced (2, 2));
        auto knobCol = limArea.removeFromRight (juce::roundToInt (88 * sx));
        layoutKnobInCell (knobCol, ceiling, juce::roundToInt (64 * sy));
        limiter.setBounds (limArea.reduced (4, 0).withSizeKeepingCentre (juce::jmin (72, limArea.getWidth() - 8), kButtonH));
    }

    qualityBar.setBounds (designRect (sx, sy, 145, 822, 488, 848).reduced (4, 2));

    grMeter.setBounds ({});
    eqEnableBtn.setBounds ({});
    mix.setBounds ({});
    inputKnob.setBounds ({});
    outputKnob.setBounds ({});
    inputVuMeter.setBounds ({});
    grHorizontalMeter.setBounds ({});
    outputVuMeter.setBounds ({});
    inputLrMeter.setBounds (designRect (sx, sy, 138, 864, 198, 916).reduced (4, 4));
    outputLrMeter.setBounds (designRect (sx, sy, 1280, 864, 1340, 916).reduced (4, 4));

    // ── UTILITY BAR y 858–920 ─────────────────────────────────────────────────
    inputFader.setBounds  (designRect (sx, sy, 12, 858, 138, 916).reduced (4, 2));
    bypass.setBounds      (designRect (sx, sy, 210, 862, 278, 914).reduced (4, 6));
    dim.setBounds         (designRect (sx, sy, 352, 862, 414, 914).reduced (4, 6));
    utilityPresetLabel.setBounds (designRect (sx, sy, 430, 864, 770, 914));
    latencyLabel.setBounds (designRect (sx, sy, 950, 866, 1070, 912));
    oversamplingLabel.setBounds (designRect (sx, sy, 1076, 866, 1200, 912));
    outputFader.setBounds (designRect (sx, sy, 1398, 862, 1524, 916).reduced (4, 2));
    redoBtn.setBounds     ({});
    undoBtn.setVisible (true);

    eqDisplay.toFront (false);

    if (eqModalBackdrop != nullptr && eqModalBackdrop->isVisible())
    {
        eqModalBackdrop->setBounds (getLocalBounds());
        eqModalBackdrop->toBack();
    }

    if (expandedEQPanel != nullptr && expandedEQPanel->isVisible())
    {
        expandedEQPanel->setBounds (designRect (sx, sy, 10, 10, 1526, 910));
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
    compMeterTapBar.setSelectedIndex (vuIndex, juce::dontSendNotification);
    if (vuMode.getSelectedItemIndex() != vuIndex)
        vuMode.setSelectedItemIndex (vuIndex, juce::dontSendNotification);

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
    if (auto* eqOn = processorRef.apvts.getRawParameterValue ("eqEnable"))
        eqDisplay.setEqBypassed (eqOn->load() <= 0.5f);

    // Refresh the expanded EQ panel when visible (30 Hz is sufficient for visual)
    if (expandedEQPanel != nullptr && expandedEQPanel->isVisible())
        expandedEQPanel->refreshFromParameters();

    const auto presetName = processorRef.getProgramName (processorRef.getCurrentProgram());
    presetCard.setText (presetName, juce::dontSendNotification);
    utilityPresetLabel.setText (presetName, juce::dontSendNotification);

    const auto qualityIndex = juce::jlimit (0, 3, static_cast<int> (processorRef.apvts.getRawParameterValue ("quality")->load()));
    qualityBar.setSelectedIndex (qualityIndex, juce::dontSendNotification);
    const auto trackIndex = juce::jlimit (1, 4, static_cast<int> (processorRef.apvts.getRawParameterValue ("trackCount")->load()));
    trackButtons.setSelectedIndex (trackIndex - 1, juce::dontSendNotification);

    const auto newCompModelIndex = juce::jlimit (0, 5, static_cast<int> (processorRef.apvts.getRawParameterValue ("COMP_MODEL")->load()));
    if (compModelCombo.getSelectedItemIndex() != newCompModelIndex)
    {
        compModelCombo.setSelectedItemIndex (newCompModelIndex, juce::dontSendNotification);
        refreshCompressorProfileCombo();
        updateCompressorControlVisibility();
    }

    const auto modelIndex = newCompModelIndex;
    const int maxProfile = juce::jmax (0, getAuraCompressorProfileCount (static_cast<AuraCompressorModel> (modelIndex)) - 1);
    const auto profileIndex = juce::jlimit (0, maxProfile,
                                            static_cast<int> (processorRef.apvts.getRawParameterValue ("COMP_PROFILE")->load()));
    if (compProfileCombo.getSelectedItemIndex() != profileIndex)
        compProfileCombo.setSelectedItemIndex (profileIndex, juce::dontSendNotification);

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
