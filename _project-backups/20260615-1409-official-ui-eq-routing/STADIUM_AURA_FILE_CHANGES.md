# Stadium Aura File Changes

## 2026-06-15 Premium UI RC Fix

### Backup

- `_project-backups/20260615-130717-premium-ui-rc/`
- Includes full `Source/`, `CMakeLists.txt`, `README.md`, and reference PNG.

### Files Changed And Reasons

| File | Reason |
|---|---|
| `Source/dsp/AuraProcessor.cpp` | 80 ms bypass crossfade, 50 ms mono blend, tube-drive saturation blend, 30 dB drive scale |
| `Source/dsp/WidthProcessor.h` | Smooth mono blend via `monoBlend` instead of hard width collapse |
| `Source/PluginEditor.h` | Premium rack layout structure, new top/bottom strips, disabled placeholders |
| `Source/PluginEditor.cpp` | Full reference-driven layout, A/B snapshots, Sweet Zone, horizontal faders |
| `Source/ui/PremiumFader.h` | Horizontal fader mode for bottom input/output strips |
| `Source/ui/PremiumFader.cpp` | Horizontal slider styling and label placement |
| `Source/ui/RackComponents.h` | `StereoLrMeter`, `DisabledFeatureButton`, custom routing paint |
| `Source/ui/RackComponents.cpp` | Premium metal panels, dual tubes, EQ band nodes, stereo meters |
| `build/rc_verify.cpp` | Corrected bypass/mono transition metrics and tube-drive proof |

### Final Artifacts

- `build/StadiumAura_artefacts/Release/Standalone/Stadium Aura.app`
- `build/StadiumAura_artefacts/Release/AU/Stadium Aura.component`
- `build/StadiumAura_artefacts/Release/VST3/Stadium Aura.vst3`
- `build-aax/StadiumAura_artefacts/Release/AAX/Stadium Aura.aaxplugin`

### Test Commands Run

```sh
cmake --build build --config Release
cmake --build build-aax --config Release
ctest --test-dir build -C Release --output-on-failure
./build/rc_verify
auval -v aufx SdAu StAu
```

### RC Harness Result

All 15 checks passed (`SUMMARY fails=0`).

### Known Remaining TODOs

- Manual Standalone live audio and ear verification
- Logic load/automation/session recall
- VST3 host scan/load
- Pro Tools developer scan for unsigned AAX
- PACE signing and macOS notarization
- Proximity, Analyze Source, phase invert, compressor In/Out routing DSP

## 2026-06-15 Reference Layout Fix

### Backup

- `_project-backups/20260615-133811-ui-reference-fix/`

### Changes

| File | Change |
|---|---|
| `Source/PluginEditor.h/.cpp` | Full reference-driven 3-column layout: INPUT/TONE left, AURA center, DYNAMICS/OUTPUT right, EQ row, meter bar, preset footer |
| `Source/ui/RackComponents.h/.cpp` | `HorizontalReductionMeter`, `SegmentedChoiceBar`, route right-click bypass |
| `Source/PluginProcessor.cpp` | Section enable parameters (`micSectionEnable`, etc.) |
| `Source/dsp/AuraProcessor.*` | Click-free section bypass blending for mic/preamp/harmonics/sum/master |

### Build

- Release Standalone/AU/VST3: **PASS**
- `StadiumAuraDSP`: **PASS**

## 2026-06-15 Knob/Layout Cleanup

### Backup

- `_project-backups/20260615-135416-ui-knob-fix/`

### Changes

| File | Change |
|---|---|
| `Source/ui/PremiumKnob.*` | Removed duplicated unit suffixes, tightened labels, and prevented stretched oval knob areas |
| `Source/ui/PremiumLookAndFeel.cpp` | Reworked rotary drawing into darker hardware-style caps with smaller tick marks and centered pointers |
| `Source/PluginEditor.h/.cpp` | Removed stale Aura overlay label/ring, separated header/live monitor text, fixed left-column dropdown overlap, and used fixed section heights for controls |
| `Source/ui/StandaloneAudio.h` | Persists unmuted input state after enabling live Standalone monitoring |

### Build

- Release Standalone/AU/VST3: **PASS**
- `StadiumAuraDSP`: **PASS**

### Backup

- `_project-backups/20260615-133133-master-codex-ui/`
- Full `Source/` and reference PNG.

### Files Changed

| Original | New / Action | Reason | Backed Up |
|---|---|---|---|
| `Source/ui/RackComponents.h` | Added `setRouteVisualState()` | Section highlight/dim for routing focus | Yes |
| `Source/ui/RackComponents.cpp` | Route-aware panel paint | Amber glow on focused modules, dim bypassed sections | Yes |
| `Source/PluginEditor.h` | Premium section titles, `applyRouteVisuals()` | UX labels per locked master naming | Yes |
| `Source/PluginEditor.cpp` | Routing tooltips, focus dimming, param dedup | Fix duplicate `badFrequencyTamer` attachment; OUTPUT route emphasis | Yes |
| `design-references/*.png` | Copied from `stadium-aura-premium-ui-target.png` | Named reference slots per codex spec | N/A (copy) |
| `STADIUM_AURA_PROJECT_MAP.md` | Updated gaps/status | Reflect current implementation honestly | No |

### Build Result

- Release Standalone, AU, VST3: **built successfully** 2026-06-15 ~13:35
- `StadiumAuraDSP` unit test: **passed**
- Standalone process launch: **confirmed** via direct binary exec
