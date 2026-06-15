# Stadium Aura Project Map

Baseline captured: 2026-06-15 before the locked master expansion.

Project root: `/Users/studio/Documents/New project`.

## Product Identity

- CMake target: `StadiumAura`
- Product: `Stadium Aura`
- Company: `Stadium Audio`
- Bundle ID: `com.stadiumaudio.stadiumaura`
- Manufacturer code: `StAu`
- Plug-in code: `SdAu`
- Version: `1.0.0`

## Build System

- Root build definition: `CMakeLists.txt`
- JUCE checkout: `JUCE/` (JUCE 8.0.13 at baseline)
- Local CMake: `.tools/cmake-3.31.8-macos-universal/CMake.app/Contents/bin/cmake`
- Default formats: AU, VST3, Standalone
- Conditional format: AAX when `STADIUM_AURA_AAX_SDK_PATH` is valid
- AAX SDK: `third_party/AAX_SDK`
- Test target: `StadiumAuraTests`

## Existing Build Outputs

- `build/StadiumAura_artefacts/Release/Standalone/Stadium Aura.app`
- `build/StadiumAura_artefacts/Release/AU/Stadium Aura.component`
- `build/StadiumAura_artefacts/Release/VST3/Stadium Aura.vst3`
- `build-aax/StadiumAura_artefacts/Release/AAX/Stadium Aura.aaxplugin`
- The AAX binary is a developer build; public Pro Tools loading still requires the Avid/PACE developer validation and signing workflow.

## Source Ownership

- `Source/PluginProcessor.*`: JUCE processor, APVTS, programs, state, meters
- `Source/PluginEditor.*`: current rack editor and attachments
- `Source/dsp/AuraProcessor.*`: top-level real-time DSP chain and smoothing
- `Source/dsp/MicCharacterProcessor.*`: seven broad mic-character tonal modes
- `Source/dsp/PreampArchitecture.h`: three preamp coloration modes
- `Source/dsp/CompressorSection.*`: program compressor and gain reduction
- `Source/dsp/TubeSaturation.h`: nonlinear tube stage
- `Source/dsp/TransformerColor.h`: transformer coloration
- `Source/dsp/SummingGlue.h`: summing density and glue
- `Source/dsp/WidthProcessor.h`: mono-safe width processing
- `Source/dsp/SafetyLimiter.*`: ceiling protection
- `Source/ui/PremiumLookAndFeel.*`: black/gold control drawing
- `Source/ui/PremiumKnob.*`: relative rotary control interaction
- `Source/ui/PremiumFader.*`: relative dB faders with fine mode and reset
- `Source/ui/RackComponents.*`: section panels, routing buttons, real VU, tube chamber, compact EQ
- `Source/ui/RackMeter.*`: atomic-target level/reduction display
- `Tests/DspTests.cpp`: finite-output, limiter, mode, mono, and bypass checks
- `scripts/setup_and_build_mac.sh`: dependency setup, configure, build, and tests

## Baseline Parameter IDs

Existing IDs are retained for session compatibility:

`input`, `output`, `mix`, `aura`, `tubeDrive`, `saturation`, `harmonicBias`,
`transformer`, `summing`, `glue`, `tone`, `trackCount`, `micCharacter`,
`preampMode`, `tubeSwap`, `width`, `monoCheck`, `limiter`, `ceiling`,
`compressorMode`, `compressorAmount`, `makeup`, `bypass`, `dim`, `quality`.

New locked-master parameters must use new stable IDs and must not rename these IDs.

## Baseline DSP Flow

The current implementation already provides smoothed gain/mix/Aura/color/width/ceiling/bypass controls, broad mic character, preamp coloration, compression, tube/saturation, transformer, summing/glue, tone, width, latency-aligned dry/wet and bypass, output trim, and limiting. The locked target requires this to be reorganized and expanded into the documented canonical signal path.

## Reference And Safety Files

- Primary visual target: `design-references/stadium-aura-premium-ui-target.png`
- Pre-expansion backup: `_project-backups/20260615-114040/`
- Change ledger: `STADIUM_AURA_FILE_CHANGES.md`

## Known Baseline Gaps (Resolved)

The following baseline gaps from the initial audit are now implemented:

- Relative source/target mic matching engine with protection controls
- Exposed compressor controls, parallel bleed, and real VU meter modes
- Six tube families with real drive/bias DSP and activity metering
- Premium vertical input/output faders and modular rack panels
- Functional routing focus with section highlight/dim
- Compact real tilt-EQ curve display (expanded multi-band EQ still TODO)
- Thirteen factory presets

## Remaining Gaps

- Persistent `Analyze Source` learned profile (button disabled, architecture documented)
- Expanded 8–24 band interactive EQ editor
- Per-section routing bypass parameters (COMP uses `compressorEnable`; others focus-only)
- Custom Standalone device settings page
- Ultra oversampling mode
- PACE-signed AAX for public Pro Tools distribution
- UI visual match to reference still requires personal screenshot QA at 1536×1000

## Documentation And Assets

All required `STADIUM_AURA_*.md` files live in the project root. The supplied image is in `design-references/`. No custom fonts are bundled. `CMakeLists.txt.save` is a suspicious pre-existing duplicate and was deliberately left untouched.
