# Stadium Aura UI Layout Plan

Reference: `design-references/stadium-aura-premium-ui-target.png`

## Regions

1. Top bar (58 px): logo, subtitle, preset, A/B, route buttons, SA badge.
2. Main rack (flex height):
   - Left 24%: matching engine + mic character + input RMS
   - Center-left 20%: preamp architecture
   - Hero 34%: Aura macro, Sweet Zone, tube chamber
   - Right 22%: compressor + VU + limiter utility + output RMS
3. Lower rack (132 px): console/summing, EQ display, width/mix/quality.
4. Bottom strip (118 px): horizontal input/output faders, L/R meters, bypass/mono/dim, preset card.

## Control Placement

- Source/target mic combos in matching panel top.
- Correction, target, bad-freq, body, air knobs in matching panel.
- Mic character mode/amount/de-harsh in mic panel; proximity/HPF disabled.
- Preamp mode/drive/tone/output in preamp panel; phase disabled.
- Compressor knobs left of VU; timing combo along compressor bottom edge.
- Saturation/transformer/density/glue in console panel; EQ curve in eq panel.

## Resize Behavior

- Proportional column widths preserved at 1120–1920 px.
- Bottom faders and meters remain visible at minimum size.
- Hero Aura knob scales within centre column.

Updated 2026-06-15 for premium UI RC implementation.
