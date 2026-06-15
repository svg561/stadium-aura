# Stadium Aura Visual Reference Audit

Primary reference: `design-references/stadium-aura-premium-ui-target.png`

## Match Closely (implemented)

- Dark graphite/brushed-metal rack hierarchy with rack screws
- Warm amber/gold edge lighting and active route-button glow
- Large central Aura macro with percentage readout
- Distinct left mic matching, center-left preamp, center tube hero, right compressor areas
- Real input/output/reduction metering and stereo L/R bars in bottom strip
- Two-tube chamber with signal-driven glow and 12AX7/12AU7 labels
- Upper routing strip and preset controls
- Horizontal bottom input/output faders
- EQ display with colored band nodes and tilt curve
- Sweet Zone LOW / SWEET / HOT status row

## Adapt In JUCE

- Vector-drawn controls for sharp resize
- System fonts with weight/tracking instead of bundled display fonts
- Disabled placeholders for unimplemented reference controls
- Route buttons are UI focus controls, not DSP bypasses

## Implemented Assessment — 2026-06-15

The Release editor now uses the premium rack structure above. It follows the reference hierarchy and material language rather than copying mockup pixels.

**Not yet visually confirmed:** macOS window capture was unavailable in the automated session. Manual compare at 1120×680, 1440×860, and 1920×1150 is required before public beta.
