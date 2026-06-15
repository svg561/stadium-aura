# Stadium Aura EQ Engine

Updated 2026-06-15 for the official UI map pass.

Implemented now:

- Real smoothed compact `tone` tilt remains in the master stage.
- Real five-band APVTS EQ follows bus glue and precedes stereo width/mono/limiter:
  HPF, low shelf, bell, high shelf, LPF.
- EQ parameters are preset/session safe and automatable:
  `eqEnable`, `eqHpfHz`, `eqLowShelfHz`, `eqLowShelfGainDb`, `eqBellHz`,
  `eqBellGainDb`, `eqBellQ`, `eqHighShelfHz`, `eqHighShelfGainDb`, `eqLpfHz`.
- The small EQ panel is audio-driven from real analyzer bins published by the processor.
- Clicking the small EQ panel opens an expanded EQ editor overlay with close control, analyzer, curve,
  five colored nodes, and band readouts.
- Nodes are draggable for implemented frequency/gain destinations and write APVTS parameters.

Real-time notes:

- Analyzer state is preallocated in the processor and updated in `processBlock()` with fixed-size arrays.
- No file I/O, locks, logging, heap allocation, or UI calls occur in the audio callback.
- The visual spectrum is intentionally generic and not a FabFilter-style clone.

Honest TODO:

- Bell Q is wired, smoothed, displayed, automatable, and preset-safe, but Q is not yet draggable from the node.
- The EQ is five-band static EQ, not dynamic EQ.
- Pre/post analyzer toggle and reset button are not yet separate controls.
- Future expansion to 8-12 bands/24-band architecture remains planned, not presented as active.
