# Stadium Aura Build Notes

## Toolchain

- macOS Intel, Apple Command Line Tools
- Project-local CMake 3.31.8:
  `.tools/cmake-3.31.8-macos-universal/CMake.app/Contents/bin/cmake`
- JUCE 8.0.13 in `JUCE/`
- Command-line generator: `Unix Makefiles`

## Standard Build

```sh
export PATH="$PWD/.tools/cmake-3.31.8-macos-universal/CMake.app/Contents/bin:$PATH"
cmake -S . -B build -G "Unix Makefiles"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Artifacts:

- `build/StadiumAura_artefacts/Release/Standalone/Stadium Aura.app`
- `build/StadiumAura_artefacts/Release/AU/Stadium Aura.component`
- `build/StadiumAura_artefacts/Release/VST3/Stadium Aura.vst3`

## AAX Build

```sh
./.tools/cmake-3.31.8-macos-universal/CMake.app/Contents/bin/cmake \
  -S . -B build-aax -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release \
  -DSTADIUM_AURA_AAX_SDK_PATH="$PWD/third_party/AAX_SDK"
./.tools/cmake-3.31.8-macos-universal/CMake.app/Contents/bin/cmake \
  --build build-aax --config Release
```

AAX output: `build-aax/StadiumAura_artefacts/Release/AAX/Stadium Aura.aaxplugin`.

## Install Locations

- AU: `/Library/Audio/Plug-Ins/Components/Stadium Aura.component`
- VST3: `/Library/Audio/Plug-Ins/VST3/Stadium Aura.vst3`
- AAX: `/Library/Application Support/Avid/Audio/Plug-Ins/Stadium Aura.aaxplugin`

`scripts/setup_and_build_mac.sh` detects Xcode Command Line Tools, Homebrew, local CMake, Git and
JUCE, and falls back to project-local CMake and Unix Makefiles when appropriate.

## Known Build Notes

VST3 receives an ad-hoc macOS signature during the JUCE build. AAX has separate PACE Eden signing
requirements. Full Xcode is not required for the current AU/VST3/Standalone command-line build.

Verified 2026-06-15 official UI map / EQ routing pass:

- `cmake --build build --config Release -j4`: passed; Standalone, AU, VST3 artifacts produced.
- `ctest --test-dir build -C Release --output-on-failure`: passed (`StadiumAuraDSP`).
- `cmake --build build-aax --config Release -j4`: passed; AAX artifact produced.
- `ctest --test-dir build-aax -C Release --output-on-failure`: passed (`StadiumAuraDSP`).
- VST3 received the expected ad-hoc signature refresh during the build.
- AAX remains unsigned and still requires Avid/PACE validation and signing for public Pro Tools use.

Prior RC notes still apply: bypass/mono transitions are smoothed to 80 ms / 50 ms, `auval` previously succeeded after AU reinstall, and live Standalone audio plus host scans remain manual.
