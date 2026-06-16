#!/usr/bin/env python3
# Stadium Aura hard UI patch. Run from repo root:
#   python3 Tools/apply_stadium_aura_hard_ui_patch.py
#
# Changes UI labels, actual resized() bounds, one large VU placement,
# and a real tube chamber renderer. It does not touch DSP code, parameter IDs,
# parameter ranges, saved defaults, or audio routing.
from pathlib import Path
import re
import sys

ROOT = Path.cwd()
plugin_h = ROOT / "Source" / "PluginEditor.h"
plugin_cpp = ROOT / "Source" / "PluginEditor.cpp"
rack_cpp = ROOT / "Source" / "ui" / "RackComponents.cpp"

missing = [p for p in [plugin_h, plugin_cpp, rack_cpp] if not p.exists()]
if missing:
    print("Missing files:")
    for p in missing:
        print(" -", p)
    sys.exit(1)


def backup(path: Path):
    bak = path.with_suffix(path.suffix + ".bak_ui_hard")
    if not bak.exists():
        bak.write_text(path.read_text(encoding="utf-8"), encoding="utf-8")


for p in [plugin_h, plugin_cpp, rack_cpp]:
    backup(p)

h = plugin_h.read_text(encoding="utf-8")
replacements = {
    'RackModulePanel leftPanel { "MIC CHARACTER" };': 'RackModulePanel leftPanel { "MIC / PRE / CONSOLE" };',
    'RackModulePanel heroPanel { "AURA" };': 'RackModulePanel heroPanel { "AURA BIG" };',
    'RackModulePanel rightPanel { "DYNAMICS / OUTPUT" };': 'RackModulePanel rightPanel { "COMPRESSOR / OUTPUT" };',
    'NavRouteButton { "SUM" }': 'NavRouteButton { "SUMMING" }',
    'DisabledFeatureButton analyzeSource { "ANALYZE",': 'DisabledFeatureButton analyzeSource { "ANALYZE SOURCE",',
    'PremiumKnob presenceKnob { "PRES",': 'PremiumKnob presenceKnob { "PRESENCE",',
    'PremiumKnob micCharOutputKnob { "OUT",': 'PremiumKnob micCharOutputKnob { "OUTPUT",',
    'PremiumKnob micCharSibilanceKnob { "SIB",': 'PremiumKnob micCharSibilanceKnob { "SIBILANCE",',
    'PremiumKnob saturation { "SAT",': 'PremiumKnob saturation { "SATURATION",',
    'PremiumKnob transformer { "XFMR",': 'PremiumKnob transformer { "TRANSFORMER",',
    'PremiumKnob summing { "SUM",': 'PremiumKnob summing { "SUMMING",',
    'PremiumKnob correction { "CORR",': 'PremiumKnob correction { "CORRECTION",',
    'PremiumKnob targetAmount { "TGT",': 'PremiumKnob targetAmount { "TARGET",',
    'PremiumKnob badFreq { "BAD",': 'PremiumKnob badFreq { "BAD FREQ",',
    'PremiumKnob preampDrive { "DRV",': 'PremiumKnob preampDrive { "DRIVE",',
    'PremiumKnob aura { "AURA",': 'PremiumKnob aura { "AURA BIG",',
    'PremiumKnob compAmount { "AMT",': 'PremiumKnob compAmount { "AMOUNT",',
    'PremiumKnob attack { "ATK",': 'PremiumKnob attack { "ATTACK",',
    'PremiumKnob release { "REL",': 'PremiumKnob release { "RELEASE",',
    'PremiumKnob threshold { "THR",': 'PremiumKnob threshold { "THRESHOLD",',
    'PremiumKnob ratio { "RAT",': 'PremiumKnob ratio { "RATIO",',
    'PremiumKnob bleed { "BLD",': 'PremiumKnob bleed { "BLEED",',
    'PremiumKnob width { "WID",': 'PremiumKnob width { "WIDTH",',
    'PremiumKnob ceiling { "CEIL",': 'PremiumKnob ceiling { "CEILING",',
    'juce::ToggleButton hardwareSafe { "SAFE" };': 'juce::ToggleButton hardwareSafe { "HARDWARE SAFE" };',
    'PremiumKnob compDensityKnob { "DENS",': 'PremiumKnob compDensityKnob { "DENSITY",',
    'PremiumKnob compWarmthKnob  { "WARM",': 'PremiumKnob compWarmthKnob  { "WARMTH",',
    'PremiumKnob compOutputKnob  { "OUT",': 'PremiumKnob compOutputKnob  { "OUTPUT",',
    'HorizontalReductionMeter compGrMeter { "COMP GR" };': 'HorizontalReductionMeter compGrMeter { "GAIN REDUCTION" };',
    'PremiumKnob compSidechainKnob { "HPF",': 'PremiumKnob compSidechainKnob { "SC HPF",',
    'AuraBigStageLed { "DENS" }': 'AuraBigStageLed { "DENSITY" }',
    'AuraBigStageLed { "WID" }': 'AuraBigStageLed { "WIDTH" }',
    'AuraBigStageLed { "LIM" }': 'AuraBigStageLed { "LIMIT" }',
}
for old, new in replacements.items():
    h = h.replace(old, new)
plugin_h.write_text(h, encoding="utf-8")

cpp = plugin_cpp.read_text(encoding="utf-8")
cpp = cpp.replace(
    'auraBigLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));',
    'auraBigLabel.setFont (juce::FontOptions (22.0f, juce::Font::bold));'
)
cpp = cpp.replace(
    'attachCombo (preampMode, "preampMode", { "73 Vintage Iron", "API Punch", "Avalon Clean" });',
    'attachCombo (preampMode, "preampMode", { "Iron 73", "American Punch", "Avalon Clean" });'
)
cpp = cpp.replace(
    'attachCombo (consoleMode, "consoleMode", { "Clean Console", "Vintage Desk", "Modern Punch", "Tube Console" });',
    'attachCombo (consoleMode, "consoleMode", { "Clean Line", "Large Format 9000", "British Iron", "American Punch", "Live Gold", "Velvet Desk" });'
)
cpp = cpp.replace(
    'compModeButtons.setChoices ({ "FAST 76", "SMOOTH OPTO", "KID670" });',
    'compModeButtons.setChoices ({ "FAST FET", "SMOOTH OPTO", "CROWN MU" });'
)
cpp = cpp.replace(
    'const char* routeNames[] { "MIC", "PRE", "COMP", "HARM", "SUM", "MASTR", "OUT" };',
    'const char* routeNames[] { "MIC", "PRE", "COMP", "HARM", "SUMMING", "MASTER", "OUTPUT" };'
)
cpp = cpp.replace('routing[6].setButtonText ("OUT");', 'routing[6].setButtonText ("OUTPUT");')

new_resized = r'''void StadiumAuraAudioProcessorEditor::resized()
{
    // HARD UI REBUILD PASS:
    // Changes actual component bounds. DSP, parameter IDs, ranges, defaults, and routing are untouched.
    constexpr int kOuterPad  = 10;
    constexpr int kPanelPad  = 14;
    constexpr int kPanelTop  = 32;
    constexpr int kGap       = 8;
    constexpr int kDropH     = 34;
    constexpr int kButtonH   = 28;
    auto area = getLocalBounds().reduced (kOuterPad);
    auto header = area.removeFromTop (74);
    const int mainH = juce::jlimit (360, 500, juce::roundToInt (area.getHeight() * 0.58f));
    auto main = area.removeFromTop (mainH);
    auto lowerModules = area.removeFromTop (juce::jlimit (84, 118, juce::roundToInt (area.getHeight() * 0.17f)));
    auto eqStrip = area.removeFromTop (juce::jlimit (78, 96, juce::roundToInt (area.getHeight() * 0.18f)));
    auto stadiumVuStrip = area.removeFromTop (juce::jlimit (88, 118, juce::roundToInt (area.getHeight() * 0.38f)));
    auto bottom = area;
    // HEADER
    {
        const int logoW = (getWidth() < 1320) ? 190 : 235;
        auto logo = header.removeFromLeft (logoW);
        logoTitle.setBounds    (logo.removeFromTop (42).reduced (0, 2));
        logoSubtitle.setBounds (logo.reduced (0, 2));
        favoriteBtn.setBounds (header.removeFromLeft (30).reduced (3, 13));
        presetPrev.setBounds  (header.removeFromLeft (30).reduced (3, 13));
        presetNext.setBounds  (header.removeFromLeft (30).reduced (3, 13));
        presetCard.setBounds  (header.removeFromLeft (210).reduced (6, 10));
        saveBtn.setBounds     (header.removeFromLeft (52).reduced (4, 13));
        abA.setBounds         (header.removeFromLeft (32).reduced (3, 13));
        abB.setBounds         (header.removeFromLeft (32).reduced (3, 13));
        header.removeFromLeft (22);
        settingsBtn.setBounds  (header.removeFromRight (34).reduced (3, 13));
        helpBtn.setBounds      (header.removeFromRight (34).reduced (3, 13));
        monitorLabel.setBounds (header.removeFromRight (188).reduced (2, 13));
        auto nav = header.reduced (2, 8);
        const int navW = juce::jmax (72, nav.getWidth() / static_cast<int> (routing.size()));
        for (auto& b : routing)
            b.setBounds (nav.removeFromLeft (navW).reduced (3, 0));
    }
    auto inMeter  = main.removeFromLeft (54);
    auto outMeter = main.removeFromRight (54);
    inputRms.setBounds  (inMeter.reduced (3, 6));
    outputRms.setBounds (outMeter.reduced (3, 6));
    auto left = main.removeFromLeft (juce::roundToInt (main.getWidth() * 0.34f));
    auto center = main.removeFromLeft (juce::roundToInt (main.getWidth() * 0.39f));
    auto right = main;
    leftPanel.setBounds (left.reduced (2, 0));
    heroPanel.setBounds (center.reduced (2, 0));
    rightPanel.setBounds (right.reduced (2, 0));
    // LEFT: MIC / PRE / CONSOLE
    {
        auto la = leftPanel.getBounds().reduced (kPanelPad, kPanelTop);
        micCharProfile.setBounds (la.removeFromTop (kDropH));
        la.removeFromTop (kGap);
        auto buttonRow = la.removeFromTop (kButtonH);
        micCharBypass.setBounds (buttonRow.removeFromLeft (buttonRow.getWidth() / 2).reduced (3, 0));
        micCharSimpleMode.setBounds (buttonRow.reduced (3, 0));
        la.removeFromTop (kGap);
        preampMode.setBounds (la.removeFromTop (kDropH));
        la.removeFromTop (kGap);
        consoleMode.setBounds (la.removeFromTop (kDropH));
        la.removeFromTop (kGap + 2);
        sourceMic.setBounds ({});
        targetMic.setBounds ({});
        correction.setBounds ({});
        targetAmount.setBounds ({});
        badFreq.setBounds ({});
        const bool simpleMic = processorRef.apvts.getRawParameterValue ("MIC_CHAR_SIMPLE_MODE")->load() > 0.5f;
        const int gridRows = simpleMic ? 2 : 3;
        const int availableForKnobs = juce::jmax (140, la.getHeight() - (kButtonH * 2 + kGap * 3));
        const int knobCellH = juce::jlimit (82, 112, availableForKnobs / gridRows);
        auto grid = la.removeFromTop (knobCellH * gridRows);
        if (simpleMic)
            layoutKnobGrid (grid, 2, { &bodyKnob, &presenceKnob, &airKnob, &micCharColorKnob });
        else
            layoutKnobGrid (grid, 2, { &bodyKnob, &presenceKnob, &airKnob, &micCharColorKnob,
                                       &micCharOutputKnob, &micCharInputTrimKnob });
        micCharProximityKnob.setBounds ({});
        micCharDeHarshKnob.setBounds ({});
        micCharSibilanceKnob.setBounds ({});
        la.removeFromTop (kGap);
        auto driveRow = la.removeFromTop (juce::jlimit (84, 104, la.getHeight() / 2));
        preampDrive.setBounds (driveRow.withSizeKeepingCentre (92, 92));
        la.removeFromTop (kGap);
        analyzeSource.setBounds (la.removeFromTop (kButtonH).reduced (3, 0));
        la.removeFromTop (kGap);
        hardwareSafe.setBounds (la.removeFromTop (kButtonH).reduced (3, 0));
    }
    // CENTER: AURA BIG HERO
    {
        auto ha = heroPanel.getBounds().reduced (kPanelPad, kPanelTop);
        auto tubeCol = ha.removeFromRight (156);
        tubeDriveKnob.setBounds (tubeCol.removeFromTop (102).withSizeKeepingCentre (86, 86));
        tubeBias.setBounds      (tubeCol.removeFromTop (90).withSizeKeepingCentre (76, 76));
        tubeType.setBounds      (tubeCol.removeFromTop (kDropH).reduced (4, 0));
        tubeCol.removeFromTop (kGap);
        tubeChamber.setBounds   (tubeCol.reduced (4, 4));
        auto bottomTrack = ha.removeFromBottom (30);
        trackButtons.setBounds (bottomTrack.withSizeKeepingCentre (juce::jmin (260, bottomTrack.getWidth()), 28));
        auto stageRow = ha.removeFromBottom (32);
        const int ledSlotW = juce::jmax (34, stageRow.getWidth() / static_cast<int> (auraBigStageLeds.size()));
        for (auto& led : auraBigStageLeds)
            led.setBounds (stageRow.removeFromLeft (ledSlotW).reduced (2, 2));
        auto sweetRow = ha.removeFromBottom (30);
        sweetLow.setBounds  (sweetRow.removeFromLeft (sweetRow.getWidth() / 3).reduced (2, 3));
        sweetZone.setBounds (sweetRow.removeFromLeft (sweetRow.getWidth() / 2).reduced (2, 3));
        sweetHot.setBounds  (sweetRow.reduced (2, 3));
        auraBigLabel.setBounds (ha.removeFromTop (30).reduced (0, 0));
        const int auraSize = juce::jlimit (210, 280, juce::jmin (ha.getWidth() - 12, ha.getHeight() - 6));
        auto auraRect = ha.withSizeKeepingCentre (auraSize, auraSize);
        auraHeatRing.setBounds (auraRect.expanded (12));
        aura.setBounds (auraRect.reduced (4));
    }
    // RIGHT: COMPRESSOR
    {
        auto ra = rightPanel.getBounds().reduced (kPanelPad, kPanelTop);
        auto topRow = ra.removeFromTop (kDropH);
        compModelBar.setBounds (topRow.removeFromLeft (topRow.getWidth() / 2).reduced (3, 0));
        compProfileBar.setBounds (topRow.reduced (3, 0));
        ra.removeFromTop (kGap);
        auto statusRow = ra.removeFromTop (kButtonH);
        compressorEnable.setBounds (statusRow.removeFromRight (86).reduced (3, 0));
        compBypassBtn.setBounds (statusRow.removeFromRight (86).reduced (3, 0));
        auraLevelBtn.setBounds (statusRow.removeFromRight (112).reduced (3, 0));
        emotionLockBtn.setBounds (statusRow.removeFromRight (124).reduced (3, 0));
        emotionLockStatusLabel.setBounds ({});
        auraLevelStateLabel.setBounds ({});
        ra.removeFromTop (kGap);
        const int vuH = juce::jlimit (104, 158, ra.getHeight() / 3);
        vuMeter.setBounds (ra.removeFromTop (vuH).reduced (20, 4));
        vuMode.setBounds ({});
        compModeButtons.setBounds ({});
        compGrMeter.setBounds ({});
        compTargetGrLabel.setBounds ({});
        grMeter.setBounds ({});
        bleed.setBounds ({});
        updateCompressorControlVisibility();
        const int gridH = ra.getHeight();
        const int rowH = juce::jlimit (82, 104, gridH / 3);
        auto row1 = ra.removeFromTop (rowH);
        layoutKnobGrid (row1, 4, { &threshold, &ratio, &attack, &release });
        ra.removeFromTop (kGap);
        auto row2 = ra.removeFromTop (rowH);
        layoutKnobGrid (row2, 4, { &compInputKnob, &compDriveKnob, &compMixKnob, &compOutputKnob });
        auto row3 = ra.removeFromTop (juce::jmin (rowH, ra.getHeight()));
        layoutKnobGrid (row3, 3, { &compAmount, &compSidechainKnob, &compWarmthKnob });
        compDensityKnob.setBounds ({});
        compTimingMode.setBounds ({});
        compScHpfMode.setBounds ({});
        outputKnob.setBounds ({});
    }
    // LOWER MODULES: analog color / console / limiter controls
    {
        auto la = lowerModules.reduced (2, 4);
        const int satW = 120;
        auto satBlock = la.removeFromLeft (satW);
        saturation.setBounds (satBlock.withSizeKeepingCentre (82, 82));
        auto transformerBlock = la.removeFromLeft (150);
        transformer.setBounds (transformerBlock.withSizeKeepingCentre (88, 88));
        auto consoleBlock = la.removeFromLeft (230).reduced (4, 0);
        consoleMode.setBounds (consoleBlock.removeFromTop (kDropH));
        consoleBlock.removeFromTop (kGap);
        auto consoleKnobs = consoleBlock;
        layoutKnobGrid (consoleKnobs, 2, { &summing, &glue });
        auto busBlock = la.removeFromLeft (128);
        mix.setBounds (busBlock.withSizeKeepingCentre (88, 88));
        auto rightTools = la.removeFromRight (260);
        limiter.setBounds (rightTools.removeFromTop (kButtonH).reduced (4, 0));
        auto limKnobs = rightTools;
        layoutKnobGrid (limKnobs, 2, { &width, &ceiling });
        eqPanel.setBounds (la.reduced (4, 0));
    }
    // COMPACT EQ: simple preview only. Full 24-band view belongs in expanded EQ.
    {
        auto ea = eqPanel.getBounds().reduced (kPanelPad, 28);
        eqDisplay.setBounds (ea.reduced (4, 0));
        if (eqDisplay.isExpanded())
            eqDisplay.setBounds (getLocalBounds().reduced (80, 60));
        limMeter.setBounds ({});
    }
    // ONE BIG STADIUM VU AREA: hide three weak meter strips.
    {
        auto ma = stadiumVuStrip.reduced (10, 5);
        inputVuMeter.setBounds ({});
        grHorizontalMeter.setBounds ({});
        outputVuMeter.setBounds ({});
        qualityBar.setBounds (ma.removeFromBottom (28).reduced (60, 2));
        vuMeter.setBounds (ma.reduced (160, 2));
    }
    // BOTTOM UTILITY STRIP
    {
        auto fa = bottom.reduced (2, 4);
        inputFader.setBounds (fa.removeFromLeft (126).reduced (5, 2));
        inputLrMeter.setBounds (fa.removeFromLeft (58).reduced (4, 2));
        mono.setBounds (fa.removeFromLeft (62).reduced (5, 8));
        bypass.setBounds (fa.removeFromLeft (78).reduced (5, 8));
        dim.setBounds (fa.removeFromLeft (62).reduced (5, 8));
        presets.setBounds (fa.removeFromLeft (juce::jlimit (220, 360, fa.getWidth() / 3)).reduced (8, 10));
        undoBtn.setBounds (fa.removeFromLeft (50).reduced (5, 12));
        redoBtn.setBounds (fa.removeFromLeft (50).reduced (5, 12));
        outputFader.setBounds (fa.removeFromRight (126).reduced (5, 2));
        outputLrMeter.setBounds (fa.removeFromRight (58).reduced (4, 2));
        oversamplingLabel.setBounds (fa.removeFromRight (132).reduced (3, 12));
        latencyLabel.setBounds (fa.removeFromRight (124).reduced (3, 12));
    }
    eqDisplay.toFront (false);
    if (expandedEQPanel != nullptr && expandedEQPanel->isVisible())
    {
        expandedEQPanel->setBounds (getLocalBounds().reduced (18, 14));
        expandedEQPanel->toFront (false);
    }
}
'''

cpp_before_resized = cpp
cpp = re.sub(
    r'void StadiumAuraAudioProcessorEditor::resized\(\)\s*\{.*?\n\}\n\nvoid StadiumAuraAudioProcessorEditor::timerCallback\(\)',
    new_resized + "\n\nvoid StadiumAuraAudioProcessorEditor::timerCallback()",
    cpp,
    flags=re.S
)
if cpp == cpp_before_resized:
    print("Warning: resized() replacement did not match — may already be patched.")

cpp = cpp.replace(
    'setPanel (leftPanel, route == 0 || route == 1, route != 0 && route != 1);\n    setPanel (heroPanel, route == 3 || route == 5, route != 3 && route != 5);\n    setPanel (rightPanel, route == 2 || route == 6, route != 2 && route != 6);\n    setPanel (eqPanel, route == 5, route != 5);',
    'setPanel (leftPanel, route == 0 || route == 1 || route == 4, route != 0 && route != 1 && route != 4);\n    setPanel (heroPanel, route == 3 || route == 5, route != 3 && route != 5);\n    setPanel (rightPanel, route == 2 || route == 6, route != 2 && route != 6);\n    setPanel (eqPanel, route == 5, route != 5);'
)
plugin_cpp.write_text(cpp, encoding="utf-8")

rc = rack_cpp.read_text(encoding="utf-8")
new_tube_paint = r'''void TubeChamberComponent::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (3.0f);
    RackDrawing::paintInsetDisplay (g, b);
    auto header = b.removeFromTop (16.0f);
    g.setColour (juce::Colour (0xffffc46a));
    g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
    g.drawText ("CLASS A TUBE CHAMBER", header, juce::Justification::centred);
    auto window = b.reduced (7.0f, 5.0f);
    juce::ColourGradient bay (juce::Colour (0xff1a1009), window.getX(), window.getY(),
                              juce::Colour (0xff050403), window.getRight(), window.getBottom(), true);
    g.setGradientFill (bay);
    g.fillRoundedRectangle (window, 8.0f);
    g.setColour (juce::Colour (0x18ff9a2e));
    for (float x = window.getX() + 8.0f; x < window.getRight(); x += 11.0f)
        g.drawVerticalLine (juce::roundToInt (x), window.getY() + 6.0f, window.getBottom() - 6.0f);
    g.setColour (juce::Colour (0x42d08a36));
    g.drawRoundedRectangle (window.reduced (0.5f), 8.0f, 1.2f);
    const float heat = juce::jlimit (0.0f, 1.0f, activity);
    const float red  = juce::jlimit (0.0f, 1.0f, distortionLevel);
    const float slot = window.getWidth() / 3.0f;
    const float tubeW = juce::jlimit (24.0f, 38.0f, slot * 0.42f);
    const float tubeH = juce::jlimit (74.0f, 142.0f, window.getHeight() - 22.0f);
    for (int i = 0; i < 3; ++i)
    {
        const float cx = window.getX() + slot * (static_cast<float> (i) + 0.5f);
        auto tube = juce::Rectangle<float> (cx - tubeW * 0.5f,
                                            window.getCentreY() - tubeH * 0.5f,
                                            tubeW,
                                            tubeH);
        const float bloom = juce::jlimit (0.0f, 1.0f, heat * 0.88f + red * 0.28f);
        juce::ColourGradient halo (juce::Colour (0xffff8a19).withAlpha (0.34f * bloom),
                                   tube.getCentreX(), tube.getCentreY(),
                                   juce::Colour (0xffff210c).withAlpha (0.0f),
                                   tube.getCentreX(), tube.getY() - 20.0f,
                                   true);
        g.setGradientFill (halo);
        g.fillEllipse (tube.expanded (18.0f, 16.0f));
        auto socket = tube.withY (tube.getBottom() - 10.0f).withHeight (16.0f).expanded (4.0f, 0.0f);
        juce::ColourGradient socketGrad (juce::Colour (0xff2a2117), socket.getX(), socket.getY(),
                                         juce::Colour (0xff070605), socket.getX(), socket.getBottom(), false);
        g.setGradientFill (socketGrad);
        g.fillRoundedRectangle (socket, 5.0f);
        g.setColour (juce::Colour (0xff9f713b).withAlpha (0.55f));
        g.drawRoundedRectangle (socket, 5.0f, 0.9f);
        auto glass = tube.reduced (1.0f, 0.0f).withTrimmedBottom (8.0f);
        const float radius = glass.getWidth() * 0.48f;
        juce::ColourGradient glassFill (juce::Colour (0x44ffffff), glass.getX(), glass.getY(),
                                        juce::Colour (0x08000000), glass.getRight(), glass.getBottom(), false);
        g.setGradientFill (glassFill);
        g.fillRoundedRectangle (glass, radius);
        auto plate = glass.reduced (glass.getWidth() * 0.32f, 10.0f);
        juce::Colour hotAmber = juce::Colour (0xffff9b21).interpolatedWith (juce::Colour (0xffff2a12), red);
        juce::ColourGradient plateGrad (hotAmber.withAlpha (0.25f + heat * 0.58f),
                                        plate.getCentreX(), plate.getBottom(),
                                        juce::Colour (0xff2a1304).withAlpha (0.36f),
                                        plate.getCentreX(), plate.getY(), false);
        g.setGradientFill (plateGrad);
        g.fillRoundedRectangle (plate, plate.getWidth() * 0.42f);
        if (red > 0.08f)
        {
            auto core = plate.reduced (plate.getWidth() * 0.35f, 8.0f);
            g.setColour (juce::Colour (0xffff220a).withAlpha (red * 0.70f));
            g.fillRoundedRectangle (core, core.getWidth() * 0.5f);
        }
        g.setColour (juce::Colour (0xffffe6a5).withAlpha (0.28f + heat * 0.58f));
        const float f1 = glass.getX() + glass.getWidth() * 0.38f;
        const float f2 = glass.getX() + glass.getWidth() * 0.62f;
        g.drawLine (f1, glass.getY() + 12.0f, f1, glass.getBottom() - 10.0f, 1.2f);
        g.drawLine (f2, glass.getY() + 12.0f, f2, glass.getBottom() - 10.0f, 1.2f);
        g.drawLine (f1, glass.getBottom() - 13.0f, f2, glass.getBottom() - 13.0f, 1.0f);
        g.setColour (juce::Colour (0xffd9efff).withAlpha (0.24f));
        g.drawRoundedRectangle (glass, radius, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawLine (glass.getX() + glass.getWidth() * 0.24f,
                    glass.getY() + 8.0f,
                    glass.getX() + glass.getWidth() * 0.24f,
                    glass.getBottom() - 8.0f,
                    1.1f);
        g.setColour (juce::Colour (0xffc4975f).withAlpha (0.75f));
        for (int p = -1; p <= 1; ++p)
            g.drawLine (glass.getCentreX() + p * 4.5f, socket.getBottom() - 2.0f,
                        glass.getCentreX() + p * 4.5f, socket.getBottom() + 5.0f, 1.0f);
    }
}'''

rc_before_paint = rc
rc = re.sub(
    r'void TubeChamberComponent::paint \(juce::Graphics& g\)\s*\{.*?\n\}\n\nvoid EqSpectrumComponent::setAnalyzerLevels',
    new_tube_paint + "\n\nvoid EqSpectrumComponent::setAnalyzerLevels",
    rc,
    flags=re.S
)
if rc == rc_before_paint:
    print("Warning: TubeChamberComponent::paint replacement did not match — may already be patched.")

rc = rc.replace(
    'g.drawText (expanded ? "EQ EDITOR - POST" : "EQ (Click to Expand)",',
    'g.drawText (expanded ? "AURA EQ EDITOR - POST" : "AURA EQ  |  EXPAND",'
)
rack_cpp.write_text(rc, encoding="utf-8")

print("Applied Stadium Aura hard UI patch.")
print("Backups created next to edited files with .bak_ui_hard suffix.")
print("Now build Standalone and visually verify: AURA BIG label, real tube chamber, one large VU, cleaner layout.")
