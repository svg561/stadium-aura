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

    logoTitle.setText ("STADIUM AURA", juce::dontSendNotification);
    logoTitle.setFont (juce::FontOptions (33.0f, juce::Font::bold).withKerningFactor (0.12f));
    logoTitle.setColour (juce::Label::textColourId, juce::Colour (0xffF2E6CC));
    logoTitle.setJustificationType (juce::Justification::centredLeft);
    logoSubtitle.setText ("ANALOG CREATIVE PROCESSOR", juce::dontSendNotification);
    logoSubtitle.setFont (juce::FontOptions (11.0f, juce::Font::bold).withKerningFactor (0.08f));
    logoSubtitle.setColour (juce::Label::textColourId, juce::Colour (0xffAFA79A));
    logoSubtitle.setJustificationType (juce::Justification::centredLeft);
    presetCard.setJustificationType (juce::Justification::centred);
    presetCard.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    presetCard.setColour (juce::Label::textColourId, juce::Colour (0xffe8c98d));
    presetCard.setColour (juce::Label::backgroundColourId, juce::Colour (0xff101418));
    factoryPresetLabel.setText ("Factory Preset", juce::dontSendNotification);
    factoryPresetLabel.setJustificationType (juce::Justification::centred);
    factoryPresetLabel.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    factoryPresetLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8d806c));
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
    for (auto* label : { &logoTitle, &logoSubtitle, &presetCard, &factoryPresetLabel,
                         &latencyLabel, &oversamplingLabel, &monitorLabel })
        addAndMakeVisible (*label);

    for (auto* panel : { &micSourcePanel, &micCharPanel, &preampPanel, &bigAuraPanel, &compressorPanel })
        addAndMakeVisible (*panel);

    const std::pair<juce::Slider*, const char*> sliders[] {
        { &inputKnob, "input" }, { &outputKnob, "output" },
        { &inputFader, "input" }, { &outputFader, "output" },
        { &bodyKnob, "MIC_CHAR_BODY" }, { &presenceKnob, "MIC_CHAR_PRESENCE" }, { &airKnob, "MIC_CHAR_AIR" },
        { &micCharAmountKnob, "MIC_CHAR_COLOR" }, { &micCharOutputKnob, "MIC_CHAR_OUTPUT_TRIM" },
        { &micCharInputTrimKnob, "MIC_CHAR_INPUT_TRIM" }, { &micCharProximityKnob, "MIC_CHAR_PROXIMITY" },
        { &micCharDeHarshKnob, "MIC_CHAR_DEHARSH" }, { &micCharSibilanceKnob, "MIC_CHAR_SIBILANCE" },
        { &tubeDriveKnob, "tubeDrive" }, { &saturation, "saturation" }, { &tubeBias, "harmonicBias" },
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
    aura.textFromValueFunction = [] (double value) { return juce::String (value * 100.0, 1); };
    aura.valueFromTextFunction = [] (const juce::String& text)
    {
        return text.trim().trimCharactersAtEnd ("%").getDoubleValue() / 100.0;
    };
    sliderAttachments.push_back (std::make_unique<SliderAttachment> (processorRef.apvts, "AURA_BIG_AMOUNT", aura));

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
    attachCombo (vuMode, "vuMeterMode", { "Input", "Gain Reduction", "Output" });
    attachCombo (consoleMode, "consoleMode", { "Clean Line", "Large Format 9000", "British Iron", "American Punch", "Live Gold", "Velvet Desk" });
    attachCombo (compTimingMode, "COMP_TIMING_MODE", { "Fixed", "Manual", "Hybrid" });
    attachCombo (compScHpfMode, "COMP_SC_HPF_MODE", { "Off", "80 Hz", "150 Hz", "220 Hz" });

    for (auto* c : { static_cast<juce::Component*> (&inputRms), static_cast<juce::Component*> (&outputRms),
                     static_cast<juce::Component*> (&inputLrMeter), static_cast<juce::Component*> (&outputLrMeter),
                     static_cast<juce::Component*> (&grMeter), static_cast<juce::Component*> (&limMeter),
                     static_cast<juce::Component*> (&vuMeter), static_cast<juce::Component*> (&tubeChamber),
                     static_cast<juce::Component*> (&eqDisplay), static_cast<juce::Component*> (&trackButtons),
                     static_cast<juce::Component*> (&compModeButtons), static_cast<juce::Component*> (&qualityBar),
                     static_cast<juce::Component*> (&compModelBar), static_cast<juce::Component*> (&compProfileBar),
                     static_cast<juce::Component*> (&compGrMeter), static_cast<juce::Component*> (&vuModeBar) })
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

    auraBigLabel.setVisible (false);

    bigAuraSubtitle.setText ("MAGIC CONTROL", juce::dontSendNotification);
    bigAuraSubtitle.setFont (juce::FontOptions (10.0f, juce::Font::bold).withKerningFactor (0.06f));
    bigAuraSubtitle.setColour (juce::Label::textColourId, juce::Colour (0xff8d806c));
    bigAuraSubtitle.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (bigAuraSubtitle);

    saMarkLabel.setText ("SA", juce::dontSendNotification);
    saMarkLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    saMarkLabel.setColour (juce::Label::textColourId, juce::Colour (0xffc8962e));
    saMarkLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (saMarkLabel);
    saMarkLabel.setVisible (true);
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

    compModeButtons.setChoices ({ "FAST FET", "SMOOTH OPTO", "CROWN MU" });
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

    vuModeBar.setChoices ({ "IN", "GR", "OUT" });
    vuModeBar.setSelectedIndex (1, juce::dontSendNotification);
    vuModeBar.onChange = [this] (int index) { setChoiceParameter ("vuMeterMode", index); };

    qualityBar.setChoices ({ "ECO", "NORMAL", "HIGH", "ULTRA" });
    qualityBar.setSelectedIndex (2, juce::dontSendNotification);
    qualityBar.onChange = [this] (int index) { setChoiceParameter ("quality", index); };

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

    // Create and add the expanded EQ panel (initially hidden)
    expandedEQPanel = std::make_unique<ExpandedEQPanel> (processorRef);
    addChildComponent (*expandedEQPanel);
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

    const char* routeNames[] { "MIC", "PRE", "COMP", "HARM", "SUM", "MASTER", "OUTPUT" };
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
    constexpr int kDropH = 30;
    constexpr int kButtonH = 26;

    // ── TOP HEADER y 0–92 ─────────────────────────────────────────────────────
    logoTitle.setBounds       (designRect (sx, sy, 0, 0, 330, 52));
    logoSubtitle.setBounds    (designRect (sx, sy, 0, 52, 330, 88));
    favoriteBtn.setBounds     (designRect (sx, sy, 335, 28, 365, 68));
    presetPrev.setBounds      (designRect (sx, sy, 370, 28, 400, 68));
    presetNext.setBounds      (designRect (sx, sy, 405, 28, 435, 68));
    presetCard.setBounds      (designRect (sx, sy, 440, 24, 590, 56));
    saveBtn.setBounds         (designRect (sx, sy, 595, 28, 650, 68));
    factoryPresetLabel.setBounds (designRect (sx, sy, 440, 58, 650, 82));
    abA.setBounds             (designRect (sx, sy, 675, 710, 28, 68));
    abB.setBounds             (designRect (sx, sy, 712, 740, 28, 68));
    undoBtn.setBounds         (designRect (sx, sy, 745, 785, 28, 68));
    routing[0].setBounds      (designRect (sx, sy, 792, 860, 28, 68));
    routing[1].setBounds      (designRect (sx, sy, 865, 28, 935, 68));
    routing[2].setBounds      (designRect (sx, sy, 940, 28, 1015, 68));
    routing[3].setBounds      (designRect (sx, sy, 1020, 28, 1120, 68));
    routing[4].setBounds      (designRect (sx, sy, 1125, 28, 1198, 68));
    routing[5].setBounds      (designRect (sx, sy, 1203, 28, 1285, 68));
    routing[6].setBounds      (designRect (sx, sy, 1290, 28, 1370, 68));
    settingsBtn.setBounds     (designRect (sx, sy, 1380, 1420, 28, 68));
    helpBtn.setBounds         (designRect (sx, sy, 1425, 1465, 28, 68));
    saMarkLabel.setBounds     (designRect (sx, sy, 1490, 1536, 28, 68));
    monitorLabel.setBounds    (designRect (sx, sy, 675, 1370, 68, 88));
    presets.setVisible (false);

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
        sourceMic.setBounds (col.removeFromTop (kDropH));
        col.removeFromTop (4);
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
        auto rightStrip = col.removeFromRight (juce::roundToInt (88 * sx));
        bigAuraSubtitle.setBounds (col.removeFromTop (juce::roundToInt (18 * sy)));
        col.removeFromTop (4);

        auto sweetRow = col.removeFromBottom (juce::roundToInt (24 * sy));
        sweetLow.setBounds  (sweetRow.removeFromLeft (sweetRow.getWidth() / 3).reduced (2, 0));
        sweetZone.setBounds (sweetRow.removeFromLeft (sweetRow.getWidth() / 2).reduced (2, 0));
        sweetHot.setBounds  (sweetRow.reduced (2, 0));

        auto trackRow = col.removeFromBottom (juce::roundToInt (30 * sy));
        trackButtons.setBounds ({});
        juce::ignoreUnused (trackRow);

        tubeChamber.setBounds (col.reduced (2, 2));
        tubeChamber.toBack();

        const int auraKnobPx = juce::jlimit (180, 205, juce::roundToInt (192.0f * juce::jmin (sx, sy)));
        const int labelValueH = 38;
        auto auraArea = col.withSizeKeepingCentre (auraKnobPx, auraKnobPx + labelValueH);
        auraHeatRing.setBounds (auraArea.expanded (juce::roundToInt (10 * sx)));
        aura.setBounds (auraArea);
        aura.toFront (false);
        auraHeatRing.toBack();

        layoutKnobInCell (rightStrip.removeFromTop (juce::roundToInt (120 * sy)), tubeDriveKnob, juce::roundToInt (62 * sy));
        layoutKnobInCell (rightStrip.removeFromTop (juce::roundToInt (100 * sy)), tubeBias, juce::roundToInt (56 * sy));
        tubeType.setBounds (rightStrip.removeFromTop (kDropH).reduced (0, 2));

        for (auto& led : auraBigStageLeds)
            led.setBounds ({});
    }

    // Col 6 — COMPRESSOR
    {
        auto col = compressorPanel.getBounds().reduced (kPanelPad, kPanelTop);
        auto modeRow = col.removeFromTop (kDropH);
        compModelBar.setBounds (modeRow.removeFromLeft (modeRow.getWidth() / 2).reduced (2, 0));
        compProfileBar.setBounds (modeRow.reduced (2, 0));
        col.removeFromTop (4);

        compGrMeter.setBounds (col.removeFromTop (juce::roundToInt (22 * sy)).reduced (2, 0));
        vuMeter.setBounds (col.removeFromTop (juce::roundToInt (88 * sy)).reduced (8, 2));
        vuModeBar.setBounds (col.removeFromTop (juce::roundToInt (26 * sy)).reduced (2, 0));
        col.removeFromTop (4);

        updateCompressorControlVisibility();
        auto knobArea = col.removeFromTop (juce::jmax (140, col.getHeight() - kButtonH * 2 - 8));
        const int knobPx = juce::roundToInt (58 * sy);
        auto row1 = knobArea.removeFromTop (knobArea.getHeight() / 2);
        layoutKnobRowInArea (row1, 4, { &threshold, &ratio, &attack, &release }, knobPx);
        knobArea.removeFromTop (2);
        layoutKnobRowInArea (knobArea, 4,
                             { &compInputKnob, &compOutputKnob, &bleed, &compSidechainKnob }, knobPx);

        auto statusRow = col.removeFromTop (kButtonH);
        compressorEnable.setBounds (statusRow.removeFromRight (juce::roundToInt (72 * sx)).reduced (2, 0));
        compBypassBtn.setBounds ({});
        emotionLockBtn.setBounds ({});
        auraLevelBtn.setBounds ({});
        compTimingMode.setBounds (col.removeFromTop (kButtonH).reduced (2, 0));
        compScHpfMode.setBounds (col.removeFromTop (kButtonH).reduced (2, 0));

        compAmount.setBounds ({});
        compDriveKnob.setBounds ({});
        compMixKnob.setBounds ({});
        compDensityKnob.setBounds ({});
        compWarmthKnob.setBounds ({});
        compModeButtons.setBounds ({});
        vuMode.setBounds ({});
        compTargetGrLabel.setBounds ({});
        inputVuMeter.setBounds ({});
        grHorizontalMeter.setBounds ({});
        outputVuMeter.setBounds ({});
        grMeter.setBounds ({});
        outputKnob.setBounds ({});
    }

    // ── BOTTOM STRIP y 680–852 ────────────────────────────────────────────────
    layoutKnobInCell (designRect (sx, sy, 0, 143, 680, 852).reduced (6, 12), saturation, juce::roundToInt (78 * sy));
    layoutKnobInCell (designRect (sx, sy, 145, 282, 680, 852).reduced (6, 12), transformer, juce::roundToInt (78 * sy));

    {
        auto consoleArea = designRect (sx, sy, 284, 488, 680, 852).reduced (8, 12);
        consoleMode.setBounds (consoleArea.removeFromTop (kDropH));
        consoleArea.removeFromTop (4);
        layoutKnobInCell (consoleArea, summing, juce::roundToInt (72 * sy));
    }

    mix.setBounds ({});

    layoutKnobInCell (designRect (sx, sy, 490, 595, 680, 852).reduced (6, 12), glue, juce::roundToInt (72 * sy));
    eqDisplay.setBounds (designRect (sx, sy, 598, 1068, 680, 852).reduced (4, 8));

    {
        auto widthArea = designRect (sx, sy, 1070, 1202, 680, 852).reduced (6, 12);
        layoutKnobInCell (widthArea, width, juce::roundToInt (64 * sy));
    }

    {
        auto limArea = designRect (sx, sy, 1204, 1378, 680, 852).reduced (8, 12);
        limiter.setBounds (limArea.removeFromTop (kButtonH).reduced (2, 0));
        limArea.removeFromTop (4);
        layoutKnobInCell (limArea, ceiling, juce::roundToInt (68 * sy));
    }

    {
        auto detail = designRect (sx, sy, 1380, 1536, 680, 852).reduced (6, 10);
        limMeter.setBounds (detail.removeFromTop (detail.getHeight() / 2).reduced (2, 2));
        grMeter.setBounds (detail.reduced (2, 2));
    }

    qualityBar.setBounds ({});
    eqEnableBtn.setBounds ({});
    inputKnob.setBounds ({});
    inputLrMeter.setBounds ({});
    outputLrMeter.setBounds (designRect (sx, sy, 1280, 1340, 864, 916).reduced (4, 4));

    // ── UTILITY BAR y 858–920 ─────────────────────────────────────────────────
    inputFader.setBounds  (designRect (sx, sy, 12, 138, 858, 916).reduced (4, 2));
    bypass.setBounds      (designRect (sx, sy, 150, 228, 862, 914).reduced (4, 6));
    mono.setBounds        (designRect (sx, sy, 234, 296, 862, 914).reduced (4, 6));
    dim.setBounds         (designRect (sx, sy, 302, 364, 862, 914).reduced (4, 6));
    presetCard.setBounds  (designRect (sx, sy, 380, 720, 864, 896));
    factoryPresetLabel.setBounds (designRect (sx, sy, 380, 720, 896, 914));
    redoBtn.setBounds     (designRect (sx, sy, 796, 856, 866, 912));
    latencyLabel.setBounds (designRect (sx, sy, 870, 990, 866, 912));
    oversamplingLabel.setBounds (designRect (sx, sy, 996, 1120, 866, 912));
    outputFader.setBounds (designRect (sx, sy, 1398, 1524, 862, 916).reduced (4, 2));

    eqDisplay.toFront (false);

    if (eqModalBackdrop != nullptr && eqModalBackdrop->isVisible())
    {
        eqModalBackdrop->setBounds (getLocalBounds());
        eqModalBackdrop->toBack();
    }

    if (expandedEQPanel != nullptr && expandedEQPanel->isVisible())
    {
        expandedEQPanel->setBounds (designRect (sx, sy, 10, 1526, 10, 910));
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
    vuModeBar.setSelectedIndex (vuIndex, juce::dontSendNotification);

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
