# Stadium Aura

Stadium Aura is an analog-inspired JUCE audio effect with smoothed controls, musical coloration, compressor modes, mono-safe width, lookahead limiting, and lock-free meter publishing.

## Build

```sh
./.tools/cmake-3.31.8-macos-universal/CMake.app/Contents/bin/cmake -S . -B build -G "Unix Makefiles"
./.tools/cmake-3.31.8-macos-universal/CMake.app/Contents/bin/cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Release artifacts:

- `build/StadiumAura_artefacts/Release/Standalone/Stadium Aura.app`
- `build/StadiumAura_artefacts/Release/AU/Stadium Aura.component`
- `build/StadiumAura_artefacts/Release/VST3/Stadium Aura.vst3`

UI reference: `design-references/stadium-aura-premium-ui-target.png`

## AAX / Pro Tools

JUCE 8.0.13 supports AAX, but Avid's AAX SDK is not redistributable with this repository. Download
the AAX Evaluation Toolkit from https://developer.avid.com/aax/ after accepting Avid's license.
JUCE requires AAX SDK 2.6.1 or newer.

Extract the SDK so its root contains `Interfaces/ACF`. The recommended project-local location is:

```text
third_party/AAX_SDK/
  Interfaces/
    ACF/
  Libs/
  Utilities/
```

Configure AAX alongside AU, VST3, and Standalone with:

```sh
./.tools/cmake-3.31.8-macos-universal/CMake.app/Contents/bin/cmake \
  -S . -B build-aax -G "Unix Makefiles" \
  -DSTADIUM_AURA_AAX_SDK_PATH="$PWD/third_party/AAX_SDK"
./.tools/cmake-3.31.8-macos-universal/CMake.app/Contents/bin/cmake \
  --build build-aax --config Release
```

The unsigned development bundle will be generated at:

```text
build-aax/StadiumAura_artefacts/AAX/Stadium Aura.aaxplugin
```

Pro Tools requires appropriately signed AAX plug-ins for normal commercial distribution. Avid's
developer program provides the required signing tools and licensing process.
