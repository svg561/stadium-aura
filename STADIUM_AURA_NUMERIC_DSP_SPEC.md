# Stadium Aura Numeric DSP Specification

All parameters are host-automatable and saved in APVTS unless marked TODO. Percent controls use linear UI normalization; frequency/time controls use skewed or logarithmic mapping.

## Core And Monitoring

| ID | Label | Range / Default | Smoothing | Stage | Status / Test |
|---|---|---|---|---|---|
| `input` | Input | -24..+24 dB / 0 | 20 ms | input | Implemented; sine gain test |
| `output` | Output | -24..+12 dB / 0 | 20 ms | output | Implemented; sine gain test |
| `mix` | Mix | 0..100% / 100 | 30 ms | latency-aligned wet/dry | Implemented; null/blend test |
| `bypass` | Global Bypass | off/on / off | 7 ms crossfade | final | Implemented; discontinuity test |
| `monoCheck` | Mono | off/on / off | safe compensated sum | width/monitor | Implemented; mono test |
| `dim` | Dim | off/on / off | 20 ms, -18 dB | monitor | Implemented; intended for Standalone |
| `quality` | Quality | Eco/Normal/High/Ultra / Normal | mode-safe | color stages | High uses 2x interpolation; Ultra remains TODO |

## Mic Matching

| ID | Label | Range / Default | Smoothing | Stage | Status / Test |
|---|---|---|---|---|---|
| `sourceMicMode` | Source Mic | 11 choices / Unknown | 50 ms contours | source compensation | Foundation implemented |
| `targetMicMode` | Target Mic | 7 choices / 87 Balanced | 50 ms contours | target character | Implemented |
| `micCorrectionAmount` | Correction | 0..100% / 35 | 50 ms | source compensation | Implemented |
| `micTargetAmount` | Target | 0..100% / 50 | 50 ms | target character | Implemented |
| `badFrequencyTamer` | Bad Freq Tamer | 0..100% / 35 | 50 ms | dynamic broad zones | Foundation implemented |
| `airProtection` | Air Protect | 0..100% / 50 | 50 ms | target guard | Implemented |
| `bodyProtection` | Body Protect | 0..100% / 50 | 50 ms | target guard | Implemented |
| `hardwareSafeMode` | Hardware Safe | off/on / on | bounded targets | mic engine | Implemented |
| `analyzeSourceTone` | Analyze Source | off/on action / off | slow learner | broad-tone learner | TODO: persistent learned profile; no fake control shown |

Legacy `micCharacter` remains saved and active as an additional subtle character layer for recall compatibility.

## Preamp, Compression, Harmonics, And Console

| ID | Label | Range / Default | Smoothing | Stage | Status |
|---|---|---|---|---|---|
| `preampMode` | Preamp | 73/API/Clean / 73 | mode contour smoothing | preamp | Implemented |
| `preampDrive` | Preamp Drive | 0..100% / 25 | 35 ms | preamp | Implemented |
| `compressorEnable` | Compressor | off/on / on | gain ramp | compressor | Implemented |
| `compressorMode` | Compressor Mode | Opto/Fast 76/Kid670 / Opto | protected envelope | compressor | Implemented family behavior |
| `compThresholdDb` | Threshold | -40..+10 dB / -18 | 20 ms | compressor | Implemented |
| `compRatio` | Ratio | 1..20:1 / 4 | 30 ms | compressor | Implemented |
| `compAttackMs` | Attack | 0.02..300 ms / 20 | 40 ms | compressor | Implemented |
| `compReleaseMs` | Release | 50..10000 ms / 400 | 50 ms | compressor | Implemented |
| `compMakeupDb` | Comp Makeup | -12..+24 dB / 0 | 20 ms | compressor | Implemented |
| `compBleedPercent` | Bleed | 0..100% / 0 | 20 ms | parallel blend | Implemented, phase aligned |
| `compSidechainHpfHz` | SC HPF | 20..300 Hz / 80 | 50 ms | detector | Implemented one-pole |
| `compTimingMode` | Timing | Manual/Fixed/Fixed-Manual | protected | compressor | Implemented |
| `vuMeterMode` | VU Mode | Input/GR/Output / GR | UI ballistic | meter | Implemented source selection |
| `tubeSwap` | Legacy Tube | 3 choices | mode-safe | tube | Retained and active |
| `tubeType` | Tube Type | 6 choices / Warm Triode | mode-safe | tube | Implemented family curves |
| `tubeDrive` | Tube Drive | 0..100% / 25 | 30 ms | tube | Implemented |
| `harmonicBias` | Tube Bias | -100..100% / 0 | 30 ms | tube | Implemented |
| `tubeOutputDb` | Tube Output | -12..+12 dB / 0 | 20 ms | tube | Implemented |
| `saturation` | Saturation | 0..100% / 20 | 30 ms | saturation | Implemented |
| `transformer` | Transformer | 0..100% / 20 | 40 ms | iron | Implemented |
| `consoleMode` | Console | 4 choices / Clean | mode-safe | summing | Implemented family density |
| `trackCount` | Track Count | 1/8/16/24/32 / 16 | 40 ms | density macro | Implemented, not true multitrack summing |
| `consoleDensity` | Console Density | 0..100% / 20 | 40 ms | summing | Implemented |
| `summing` | Legacy Summing | 0..100% / 20 | 40 ms | summing | Retained and active |
| `glue` | Bus Glue | 0..100% / 20 | 40 ms | bus compressor | Implemented |

## Master

| ID | Label | Range / Default | Smoothing | Stage | Status |
|---|---|---|---|---|---|
| `aura` | Aura | 0..100% / 50 | 70 ms | multi-stage macro | Implemented, gain compensated |
| `tone` | Tone | -100..100% / 0 | 40 ms | compact tilt EQ | Implemented |
| `width` | Width | 0..150% / 100 | 50 ms | bass-safe M/S | Implemented |
| `limiter` | Limiter | off/on / on | click-safe | ceiling limiter | Implemented |
| `ceiling` | Ceiling | -12..0 dBFS / -1 | 20 ms | limiter | Implemented |
| `makeup` | Legacy Makeup | -12..+12 dB / 0 | 20 ms | post color | Retained and active |

## EQ Expansion TODO

The current real compact EQ is the smoothed `tone` tilt stage. Eight-to-twelve APVTS bands, dynamic EQ, expanded editing, analyzer, and 24-band architecture are explicitly TODO and are not represented by fake controls or curves.
