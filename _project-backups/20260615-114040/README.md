# Stadium Aura

Stadium Aura is an analog-inspired JUCE audio effect with smoothed controls, musical coloration, compressor modes, mono-safe width, lookahead limiting, and lock-free meter publishing.

## Build

Install JUCE so CMake can find it, or place a JUCE checkout in `JUCE/`, then run:

```sh
cmake -S . -B build -G Xcode
cmake --build build --config Release
```

The project builds AU, VST3, and Standalone targets. Normal quality is the default; High and Ultra enable oversampling for the nonlinear color stages.

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
