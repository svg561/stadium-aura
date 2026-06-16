#pragma once

#include <JuceHeader.h>
#include <array>

namespace StadiumAuraPresets
{
inline constexpr const char* kVocalChainGroupName = "STADIUM VOCAL CHAINS";

// Original Stadium Aura vocal-chain preset values inspired by reference intent.
// Friendly names map to existing APVTS choice indices documented in StadiumAuraFactoryPresets.cpp.
struct VocalChainPreset
{
    juce::String name;
    int micCharProfile;
    int preampMode;
    int consoleMode;
    float consoleDensity;
    float micCorrection;
    float tubeDrive;
    float micBody;
    float micAir;
    float micPresence;
    float preampDrive;
    float mix;
    float auraBigAmount;
    int tubeType;
    int compModel;
    int compressorMode;
    float compAmount;
    bool auraLevel;
    float eqLowShelfGainDb;
    float eqHighShelfGainDb;
    float eqBellHz;
    float eqBellGainDb;
    float eqBellQ;
    bool deEssBand6500;
    float micDeHarsh;
    float micSibilance;
    float outputDb;
    float ceilingDb;
    bool minimalConsole;
};

const std::array<VocalChainPreset, 6>& getVocalChainPresets() noexcept;

void applyVocalChainPreset (juce::AudioProcessorValueTreeState& apvts, const VocalChainPreset& preset);

} // namespace StadiumAuraPresets
