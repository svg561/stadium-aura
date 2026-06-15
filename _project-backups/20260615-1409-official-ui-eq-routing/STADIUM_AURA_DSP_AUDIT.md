# Stadium Aura DSP Audit

Audit updated 2026-06-15 after the locked master implementation pass.

## Code And Real-Time Structure

- `StadiumAuraAudioProcessor::processBlock()` in `Source/PluginProcessor.cpp` clears unused channels, uses `juce::ScopedNoDenormals`, reads APVTS atomics, publishes real meter atomics, and delegates to `AuraProcessor`.
- `AuraProcessor::prepare()` allocates buffers and prepares mic, preamp, transformer, width, compressor, limiter, and smoother state.
- The audio path performs no file access, UI calls, logging, locks, state serialization, explicit allocation, or vector resizing.
- State recall uses APVTS XML outside `processBlock()`. All APVTS parameters are automatable and preset/session saved.

## Locked Signal Path

Input -> source mic compensation -> broad problem-zone cleanup -> target mic character -> air/body protection -> legacy subtle character layer -> preamp -> detailed linked compressor/parallel bleed -> tube -> saturation -> transformer -> console density -> bus glue -> Aura-distributed polish -> compact tone EQ -> mono-safe width -> latency-aligned wet/dry -> output/dim -> lookahead limiter -> latency-aligned bypass -> meters.

Aura is a distributed macro rather than a single serial box. It contributes smoothed color, compression/glue, width, de-harsh behavior, and negative gain compensation.

## Parameter Audit

Every row is APVTS-automatable and recalled. Continuous parameters are smoothed at their DSP destination. Choice changes interpolate internal profiles or preserve running detector/filter state.

| IDs | UI / DSP destination | Safety status |
|---|---|---|
| `input`, `output` | Premium dB faders; exact input/final gain | 20 ms ramps |
| `mix` | Master wet/dry | 30 ms, latency aligned |
| `aura` | Central multi-stage macro | 70 ms, compensated |
| `sourceMicMode`, `targetMicMode` | Source/target selectors; relative mic stage | Profile interpolation, real audio |
| `micCorrectionAmount`, `micTargetAmount` | Source inversion / target contour depth | 50 ms |
| `badFrequencyTamer` | Broad signal-dependent harshness attenuation | 50 ms amount, bounded |
| `airProtection`, `bodyProtection` | Guard target boosts | 50 ms |
| `hardwareSafeMode` | Reduces mic moves/nonlinearity | Real boolean bound |
| `micCharacter` | Legacy extra target layer | Retained, real, hidden from main page |
| `preampMode`, `preampDrive` | 73/API/Clean families and color depth | Running-state interpolation, 35 ms drive |
| `compressorEnable`, `compressorMode` | Main linked compressor | Real gain path, detector state retained |
| `compThresholdDb`, `compRatio` | Static curve | 20/30 ms |
| `compAttackMs`, `compReleaseMs` | Detector timing | 40/50 ms |
| `compMakeupDb`, `compBleedPercent` | Comp output and phase-aligned parallel blend | 20 ms |
| `compSidechainHpfHz`, `compTimingMode` | Detector HPF and manual/fixed timing | 50 ms HPF; protected timing |
| `compressorAmount`, `makeup` | Legacy compressor/Aura depth and post-color gain | Retained and smoothed |
| `vuMeterMode` | Real input/GR/output VU source | UI-only selector over real atomics |
| `tubeType`, `tubeDrive`, `harmonicBias`, `tubeOutputDb` | Six transfer families, drive, asymmetry, trim | 20–30 ms continuous controls |
| `tubeSwap` | Legacy drive/asymmetry offset | Retained and real |
| `saturation`, `transformer` | Nonlinear blend and transformer color | 30/40 ms |
| `consoleMode`, `consoleDensity`, `trackCount` | Four compensated console families and density macro | 40 ms density; not true multitrack summing |
| `summing`, `glue` | Legacy summing amount and separate bus compressor | Retained and smoothed |
| `tone` | Real compact tilt EQ | 40 ms; curve UI follows value |
| `width`, `monoCheck` | Width above protected low bass / mono monitor | 50 ms width; safe sum |
| `limiter`, `ceiling` | Final lookahead limiter | real reduction; 20 ms ceiling |
| `bypass` | Global latency-matched dry crossfade | 7 ms |
| `dim` | Monitor attenuation | 20 ms to -18 dB |
| `quality` | Eco/Normal/High/Ultra choice | High uses 2x interpolated color path; Ultra is TODO |

No visible control is decorative. Routing buttons are explicitly UI focus controls, not pretend bypasses. The tube chamber, VU, level meters, limiter meter, and Sweet Zone all use real processor data.

## Metering

- Input/output: actual block peak atomics.
- Compressor GR: actual main plus bus compressor reduction.
- Limiter GR: actual limiter gain reduction.
- VU: selectable input, GR, or output with UI-side analog ballistics at 60 Hz.
- Tube glow: actual post-tube activity plus drive contribution.
- Sweet Zone: input level, output headroom, compressor GR, limiter GR, and tube activity.

## Presets

Thirteen factory programs are present. Loading a program writes all legacy parameters and deterministically resets/overrides every new parameter family. DAW state recall stores the complete APVTS tree.

## Builds And Tests

- Standard Release: Standalone, AU, VST3 built.
- AAX Release: built with SDK 2.9; bundle remains unsigned/PACE-blocked for public Pro Tools.
- Standard and AAX build trees: `StadiumAuraDSP` passed.
- RC behavioral harness (`build/rc_verify`) passed 15/15 after premium UI RC fix.
- Bypass uses 80 ms latency-matched crossfade; mono uses 50 ms width/mid blend.
- Tube Drive contributes independent saturation blend plus 30 dB drive scale.

## Honest TODOs And Risks

- `Analyze Source` learned broad-tone profile and persistence are not implemented; no button is shown.
- Expanded 8–12 band/dynamic EQ and real analyzer are not implemented; only the real smoothed tilt EQ is shown.
- High quality uses interpolation, not reconstruction-filtered oversampling. Ultra remains TODO.
- Limiter is sample-peak, not true-peak/inter-sample.
- Relative mic profiles are broad original voicings, not measured microphone inverses.
- Manual host scans, live Standalone audio, automation drawing, listening tests, HiDPI inspection, and Avid developer validation remain.
