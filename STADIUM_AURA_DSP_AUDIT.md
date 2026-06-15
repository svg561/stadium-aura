# Stadium Aura DSP Audit

Audit updated 2026-06-15 after the official UI map / EQ routing pass.

## Code And Real-Time Structure

- `StadiumAuraAudioProcessor::processBlock()` in `Source/PluginProcessor.cpp` clears unused channels, uses `juce::ScopedNoDenormals`, reads APVTS atomics, publishes real meter atomics, and delegates to `AuraProcessor`.
- `AuraProcessor::prepare()` allocates buffers and prepares mic, preamp, transformer, EQ, width, compressor, limiter, analyzer/meter support, and smoother state.
- The audio path performs no file access, UI calls, logging, locks, state serialization, explicit allocation, or vector resizing.
- State recall uses APVTS XML outside `processBlock()`. All APVTS parameters are automatable and preset/session saved.

## Locked Signal Path

Input -> input trim/level -> source mic compensation -> broad problem-zone cleanup -> target mic correction -> air/body protection -> legacy subtle mic character layer -> preamp architecture -> Aura macro contribution -> tube drive/tube chamber -> saturation -> transformer -> linked compressor/parallel bleed -> console density/summing -> bus glue -> compact tone tilt -> five-band EQ -> stereo width/mono -> latency-aligned wet/dry -> output level/dim -> lookahead limiter/ceiling -> latency-aligned global bypass -> output.

Aura remains a distributed macro rather than a single serial box. It contributes smoothed color, compression/glue, width, de-harsh behavior, and negative gain compensation at the appropriate stages.

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
| `compressorEnable`, `compressorMode` | Main linked compressor after harmonics/transformer | Real gain path, detector state retained |
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
| `eqEnable`, `eqHpfHz`, `eqLowShelfHz`, `eqLowShelfGainDb`, `eqBellHz`, `eqBellGainDb`, `eqBellQ`, `eqHighShelfHz`, `eqHighShelfGainDb`, `eqLpfHz` | Real five-band post-glue EQ and interactive expanded editor | 20-50 ms smoothing; APVTS safe |
| `width`, `monoCheck` | Width above protected low bass / mono monitor | 50 ms width; safe sum |
| `limiter`, `ceiling` | Final lookahead limiter | real reduction; 20 ms ceiling |
| `bypass` | Global latency-matched dry crossfade | 7 ms |
| `dim` | Monitor attenuation | 20 ms to -18 dB |
| `quality` | Eco/Normal/High/Ultra choice | High uses 2x interpolated color path; Ultra is TODO |

Visible controls are either wired or disabled with an honest tooltip. Routing buttons focus sections and right-click section bypasses for implemented groups. The tube chamber, VU, level meters, limiter meter, EQ analyzer, EQ nodes, and Sweet Zone all use real processor data.

## Metering

- Input/output: actual block peak atomics plus real bottom L/R peak atomics.
- Compressor GR: actual main plus bus compressor reduction.
- Limiter GR: actual limiter gain reduction.
- VU: selectable input, GR, or output with UI-side analog ballistics at 60 Hz.
- Tube glow: actual post-tube activity plus drive contribution.
- Sweet Zone: input level, output headroom, compressor GR, limiter GR, and tube activity.
- EQ analyzer: fixed-size real signal bands derived from the post-processed output buffer.

## Presets

Thirteen factory programs are present. Loading a program writes all legacy parameters and deterministically resets/overrides every new parameter family. DAW state recall stores the complete APVTS tree.

## Builds And Tests

- Standard Release: Standalone, AU, VST3 built after official UI map / EQ routing pass.
- AAX Release: built after official UI map / EQ routing pass; bundle remains unsigned/PACE-blocked for public Pro Tools.
- Standard and AAX build trees: `StadiumAuraDSP` passed.
- RC behavioral harness (`build/rc_verify`) passed 15/15 after premium UI RC fix.
- Bypass uses 80 ms latency-matched crossfade; mono uses 50 ms width/mid blend.
- Tube Drive contributes independent saturation blend plus 30 dB drive scale.

## Honest TODOs And Risks

- `Analyze Source` learned broad-tone profile and persistence are not implemented; the button is disabled.
- Preset save/favorite and undo/redo are not implemented; visible controls are disabled.
- Phase invert is not implemented and is not shown as an active control.
- Expanded EQ exists and is real, but bell Q is not yet node-draggable, and pre/post toggle/reset are not separate controls.
- 8-12 band/dynamic EQ remains TODO; current EQ is a real five-band static design.
- High quality uses interpolation, not reconstruction-filtered oversampling. Ultra remains TODO.
- Limiter is sample-peak, not true-peak/inter-sample.
- Relative mic profiles are broad original voicings, not measured microphone inverses.
- Manual host scans, live Standalone audio, automation drawing, listening tests, HiDPI inspection, and Avid developer validation remain.
