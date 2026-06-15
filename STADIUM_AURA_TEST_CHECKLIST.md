# Stadium Aura Test Checklist

## Standalone

- [ ] Select input/output devices and confirm audio pass.
- [ ] Confirm safe monitoring level and avoid open-mic feedback.
- [ ] Exercise every knob, selector and button.
- [ ] Verify real input/output/GR/limiter meters and tube glow.
- [ ] Load all thirteen factory presets.
- [ ] Test bypass, mono, dim and limiter transitions during audio.
- [ ] Run 10 minutes at Eco, Normal, High and Ultra; observe CPU and dropouts.

## AU / Logic

- [x] Run `auval -v aufx SdAu StAu` — passed 2026-06-15 after premium UI RC rebuild.
- [ ] Confirm Logic scan, load, audio pass and UI open.
- [ ] Automate continuous and choice parameters.
- [ ] Save/reopen a session and verify complete recall.
- [ ] Confirm preset changes do not click or exceed the ceiling.

## VST3

- [ ] Scan in an available VST3 host.
- [ ] Load mono and stereo tracks.
- [ ] Verify audio, automation, state recall and all presets.
- [ ] Confirm no crash during rescan or UI resize.

## AAX / Pro Tools

- [x] Configure and build with `STADIUM_AURA_AAX_SDK_PATH`.
- [ ] Install in `/Library/Application Support/Avid/Audio/Plug-Ins/`.
- [ ] Use Pro Tools Developer Build for unsigned tests.
- [ ] Run DigiShell/AAX Validator Describe validation.
- [ ] Check DigiTrace and plug-in scan logs.
- [ ] Confirm mono and stereo inserts.
- [ ] Record PACE Eden signing status; public Pro Tools requires it.

## Automated Result — 2026-06-15 Premium UI RC

- [x] Release build completed for Standalone, AU, and VST3.
- [x] AAX Release build completed with SDK 2.9.
- [x] `StadiumAuraDSP` unit test passed.
- [x] RC behavioral harness 15/15 passed (`build/rc_verify`).
- [x] Bypass transition bounded (`maxDelta=0.1078`).
- [x] Mono transition bounded (`maxDelta=0.0471`).
- [x] Tube drive activity confirmed (`harmRatio=1.1370`).
- [x] `auval` succeeded after AU reinstall.
- [x] Premium rack UI compiled into Release Standalone.
- [x] Routing buttons highlight focused section and dim others (2026-06-15 master codex UI pass).
- [x] Standalone app launches (direct binary exec confirmed).
- [ ] Manual Standalone live audio/device test.
- [ ] UI screenshot/visual compare at 1120/1440/1536/1920 widths.
- [ ] Logic host validation beyond auval.
- [ ] VST3 host scan.
- [ ] Developer Pro Tools/DigiShell validation and PACE workflow.

## Automated Result — 2026-06-15 Official UI Map / EQ Routing Pass

- [x] Release build completed for Standalone, AU, and VST3.
- [x] AAX Release build completed using the existing AAX build tree.
- [x] `ctest --test-dir build -C Release --output-on-failure` passed.
- [x] `ctest --test-dir build-aax -C Release --output-on-failure` passed.
- [x] Five-band EQ DSP exercised in `StadiumAuraDSP`.
- [x] Section bypasses exercised without accidental mute in `StadiumAuraDSP`.
- [x] Mono-to-stereo bus handling compiled into Release.
- [ ] Manual Standalone live audio/device test after UI map pass.
- [ ] Settings gear Audio/MIDI dialog manual check after UI map pass.
- [ ] UI screenshot/visual compare against official screenshot after UI map pass.
- [ ] Logic/VST3 host scan and automation/session recall after EQ parameter addition.
- [ ] Developer Pro Tools/DigiShell validation and PACE workflow after AAX rebuild.
