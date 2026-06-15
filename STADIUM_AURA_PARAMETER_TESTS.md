# Stadium Aura Parameter Tests

- Exact input/output dB gain using a low-level 1 kHz sine with color stages neutral
- Source selector changes source compensation; target selector changes target contour
- Correction/target amounts scale their respective stages
- Hardware Safe reduces maximum correction and density
- Threshold/ratio increase computed GR; attack/release alter transient/recovery behavior
- Bleed crossfades compressor input and output without phase shift
- VU sources follow real input, GR, and output atomics
- Tube type and drive alter harmonic output while remaining finite
- Console density changes harmonic/cohesion behavior without a simple gain-only change
- Aura changes multiple internal targets and remains bounded
- Limiter output remains at/below ceiling within tolerance
- Bypass and monitoring transitions remain ramped
- Mono and stereo processing remain finite

Automated coverage is expanded in `Tests/DspTests.cpp`; host scan, live audio, automation drawing, and listening tests remain manual.
