# Stadium Aura Mic Character Data

These are original tonal voicings, not microphone clones. Public-facing names deliberately describe
character rather than claiming measured equivalence.

| Mode | Intent and public anchor | Implemented curve/harmonics | Later modeling |
|---|---|---|---|
| 67 Vintage Smooth | Forgiving tube-style mids and controlled presence | 55 Hz gentle HP behavior, +0.8 dB body, -0.8 dB presence, +0.2 dB air, moderate even bias, -0.35 dB compensation | Dynamic 4–8 kHz de-harsh and calibrated proximity |
| C12 Open Air | Leaner low mids, open presence and tube gloss | 72 Hz HP, -0.7 dB body, +1 dB presence, +1.8 dB air, light even sheen, -0.7 dB compensation | Multi-biquad air shelf and adaptive 7–9 kHz control |
| 251 Classic | Smooth mids and polished air | 65 Hz HP, -0.5 dB body, +0.7 dB presence, +1.5 dB air, light even harmonics | Refined capsule/transient model |
| 87 Modern Balanced | Controlled low end and clear modern mids | 78 Hz HP, -0.6 dB body, +0.8 dB presence, +0.6 dB air, near-clean harmonics | Dynamic mud cleanup and de-harsh |
| 47 Velvet Tube | Large body, rounded top and dense even harmonics | 45 Hz HP, +1.3 dB body, -0.7 dB presence, -0.3 dB air, strongest even bias, -0.55 dB compensation | Drive-dependent proximity and transformer hysteresis |
| 251E Silky Air | Elegant air, smooth upper mids, lighter body | 68 Hz HP, -0.6 dB body, +0.9 dB presence, +2 dB air, refined even sheen | Adaptive sibilance control and multi-stage tube model |
| 800G Air Pop | Forward, glossy and modern with controlled lows | 82 Hz HP, -1 dB body, +1.5 dB presence, +2.3 dB air, low distortion, -0.95 dB compensation | Dynamic 6.5–9 kHz control and transient detector |

## DSP Design

`MicCharacterProcessor` uses per-channel preallocated states. A gentle high-pass component, body,
presence and air bands are derived from minimum-phase one-pole filters, then combined with a safe
asymmetric tanh stage and per-mode gain compensation. All profile values interpolate over 35 ms, so
automation and mode changes do not reset filters or click.

Affected parameters are `micCharacter`; the stage also interacts musically with downstream preamp,
tube, transformer, Aura and output compensation. The choice is automatable and stored by APVTS.

## Relative Source Foundation

`sourceMicMode` adds conservative broad profiles for Unknown, general dynamic/condenser/ribbon, 57-style, 7B-style, C80-style, bright condenser, dark condenser, warm tube, and flat/measurement families. Source contours are inverted proportionally by `micCorrectionAmount`, followed by broad signal-dependent harshness control and then `targetMicMode` at `micTargetAmount`.

`airProtection`, `bodyProtection`, and `hardwareSafeMode` bound boosts and nonlinear density. These are broad musical families, not measured inverses. Persistent learned broad-tone data for `Analyze Source` remains TODO and no fake analysis button is displayed.
