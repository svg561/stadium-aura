# Stadium Aura Research Notes

Research date: 2026-06-15. Values below are product-design ranges, not circuit-accurate clone claims.

## Engineering Sources And Boundaries

- JUCE `AudioProcessorValueTreeState`, attachments, `SmoothedValue`, DSP oversampling, compressor, and limiter documentation: https://docs.juce.com/
- JUCE CMake plug-in examples and AAX path handling: https://github.com/juce-framework/JUCE
- Avid AAX SDK license, getting-started, distribution, real-time, parameter automation, and troubleshooting documentation in `third_party/AAX_SDK/Documentation/` and https://developer.avid.com/aax
- Public manufacturer manuals/specifications should be used for high-level timing/range orientation. Stadium Aura names its modes generically and does not claim circuit identity or measured transfer matching.

## Compressor Families

- Smooth optical behavior: soft knee, slower/program-sensitive recovery, RMS/peak hybrid detection, useful broad range. Stadium Aura uses bounded detector and envelope behavior rather than a named hardware transfer curve.
- Fast FET behavior: peak-sensitive detector, very fast attack, fast-to-medium release, stepped ratio choices, and controlled odd-harmonic edge. Approximate useful design range: 0.02–0.8 ms attack and 50–1100 ms release.
- Vari-mu behavior: very soft knee, level-dependent effective ratio, linked stereo action, and program-dependent recovery. Time constants are presented as original Stadium Aura timing families.

## Microphone Families

Published microphone charts vary by revision, capsule, polar pattern, distance, room, and source. Therefore the mic engine uses broad conservative zones rather than claiming exact response inversion:

- Dynamics commonly need optional presence/body correction with guarded air enhancement.
- Condensers vary from neutral to bright; source compensation must avoid stacking presence/air boosts.
- Ribbons generally require careful high-frequency treatment and body protection.
- Tube vocal targets use small broad body/presence/air contours plus low-level nonlinear density.
- `Analyze Source` may learn broad spectral balance only. It must never claim to identify an exact microphone.

## Real-Time Safety

- APVTS atomics are read on the audio thread; state serialization stays off that thread.
- Continuous gain, blend, tone, dynamics, color, width, and limiter values are ramped.
- Meter values cross to the UI via atomics; UI ballistics and repaint occur at 30–60 Hz.
- Buffers and DSP state are prepared before playback. No allocation, file access, logging, or locks are introduced in `processBlock()`.

## Formats And AAX

- AU, VST3, Standalone, and AAX can share the JUCE processor/editor implementation.
- JUCE 8.0.13 supports AAX when `JUCE_GLOBAL_AAX_SDK_PATH` points to a valid licensed SDK.
- A successful `.aaxplugin` compile is not equivalent to public Pro Tools authorization. Local developer testing requires the Avid developer/evaluation workflow; distribution requires the applicable PACE Eden signing process and Avid requirements.
- Stadium Aura keeps AAX conditional and does not claim public Pro Tools compatibility until that workflow is completed.

## Explicit TODO Research

- Measured source/target microphone datasets and controlled test fixtures
- Circuit-derived tube, transformer, FET, optical, and vari-mu models
- Listening-panel calibration and loudness-matched preset validation
- Expanded dynamic EQ and analyzer validation
