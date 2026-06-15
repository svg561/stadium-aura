# Stadium Aura AAX / Pro Tools Notes

## Current Status

The AAX target builds as `Stadium Aura.aaxplugin` when configured with the official SDK. The bundle
exports `ACFRegisterPlugin` and `ACFRegisterComponent`, supports matching mono-to-mono and
stereo-to-stereo layouts, and uses stable IDs `StAu` / `SdAu` with bundle identifier
`com.stadiumaudio.stadiumaura`.

Current SDK: project-local AAX SDK with interfaces through 2.9.0. JUCE version: 8.0.13.

Configure with:

```sh
./.tools/cmake-3.31.8-macos-universal/CMake.app/Contents/bin/cmake \
  -S . -B build-aax -G "Unix Makefiles" \
  -DSTADIUM_AURA_AAX_SDK_PATH="$PWD/third_party/AAX_SDK"
./.tools/cmake-3.31.8-macos-universal/CMake.app/Contents/bin/cmake \
  --build build-aax --config Release
```

Output: `build-aax/StadiumAura_artefacts/Release/AAX/Stadium Aura.aaxplugin`

Install path: `/Library/Application Support/Avid/Audio/Plug-Ins/`

## Signing Gate

The current development bundle is unsigned. Shipping Pro Tools requires a PACE Eden signature;
Apple `codesign` or ad-hoc signing alone is insufficient. Request PACE tools from
`audiosdk@avid.com` with subject `Pace Tools Request`. Eden signing must be the final bundle step.

Unsigned testing requires either:

- Pro Tools Developer Build plus a Pro Tools Developer iLok license requested from
  `devauth@avid.com`, or
- DigiShell/AAX Validator from Avid's My Toolkits and Downloads page.

DigiShell Describe validation:

```text
load_dish aaxh
loadpi "/absolute/path/Stadium Aura.aaxplugin"
getdescriptionvalidationinfo 0
```

Validator alternative:

```text
load_dish aaxval
runtest [test.describe_validation, "/absolute/path/Stadium Aura.aaxplugin"]
```

## Troubleshooting

1. Confirm the SDK root contains `Interfaces/ACF`.
2. Confirm `.aaxplugin/Contents/MacOS/Stadium Aura` exists.
3. Confirm the host architecture matches. Current local build is x86_64; installed Pro Tools is
   universal and can run x86_64 on this Intel Mac.
4. Remove stale copies from `Plug-Ins (Unused)` and clear the Pro Tools plug-in cache before rescanning.
5. Use a Developer Build for unsigned testing.
6. Run Describe validation and inspect DigiTrace logs.
7. For public Pro Tools, apply the PACE Eden signature after every other bundle modification.

What cannot be tested without Avid credentials: shipping-host loading of an unsigned bundle,
PACE signing, commercial authorization, and final Avid certification.

Verified 2026-06-15: AAX SDK 2.9 (`AAX_SDK_VERSION 0x0209`) configured and compiled an x86_64 bundle. `codesign -dv` reports `code object is not signed at all`. Build success therefore does not change the Pro Tools/PACE gate.
