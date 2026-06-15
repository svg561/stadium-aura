# Stadium Aura UI Direction

Official reference: `design-references/stadium-aura-premium-ui-target.png`

## Identity

- Product: **STADIUM AURA**
- Subtitle: **ANALOG CREATIVE PROCESSOR**
- Tone: dark brushed-metal rack, orange/gold glow, premium analog console hardware

## Layout (implemented 2026-06-15)

1. **Top bar:** logo, subtitle, preset selector, A/B/copy, route buttons (MIC/PRE/COMP/HARMONICS/SUM/MASTER/OUTPUT), SA badge.
2. **Left column:** source/target mic matching, correction/target/bad-freq/body/air controls, hardware safe, input RMS meter, mic character mode/amount/de-harsh.
3. **Center-left:** preamp architecture mode, drive, tone, output.
4. **Center hero:** Aura macro with percent readout, Sweet Zone LOW/SWEET/HOT, tube chamber, tube drive slider, bias, tube type.
5. **Right:** compressor mode, VU meter selector, analog VU, threshold/ratio/attack/release/makeup/bleed/SC HPF, limiter utility.
6. **Lower rack:** saturation, transformer, console mode/tracks/density, bus glue, EQ display, width/mix/quality.
7. **Bottom strip:** horizontal input/output faders, L/R meters, bypass/mono/dim, preset card, disabled favorite/undo/redo placeholders.

## Wiring Policy

- Every active control maps to an existing APVTS parameter.
- Unimplemented features use disabled buttons with explicit tooltips:
  - Analyze Source, Proximity, HPF, Phase Invert, Compressor In/Out, Favorite, Undo, Redo.

## Resize Targets

- Default: 1440 × 860
- Minimum: 1120 × 680
- Maximum: 1920 × 1150

## Visual QA Status

Premium rack UI is compiled into Release Standalone. Automated window capture was blocked by macOS assistive-access restrictions in the agent session. Manual visual inspection at all three sizes remains required.
