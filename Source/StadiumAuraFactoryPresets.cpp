#include "StadiumAuraFactoryPresets.h"

namespace StadiumAuraPresets
{
namespace
{
constexpr auto kMicCharProfile     = "MIC_CHAR_PROFILE";
constexpr auto kMicCorrection      = "micCorrectionAmount";
constexpr auto kTubeDrive          = "tubeDrive";
constexpr auto kMicBody            = "MIC_CHAR_BODY";
constexpr auto kMicAir             = "MIC_CHAR_AIR";
constexpr auto kMicPresence        = "MIC_CHAR_PRESENCE";
constexpr auto kPreampDrive        = "preampDrive";
constexpr auto kMix                = "mix";
constexpr auto kAuraBigAmount      = "AURA_BIG_AMOUNT";
constexpr auto kTubeType           = "tubeType";
constexpr auto kPreampMode         = "preampMode";
constexpr auto kConsoleMode        = "consoleMode";
constexpr auto kConsoleDensity     = "consoleDensity";
constexpr auto kCompModel          = "COMP_MODEL";
constexpr auto kCompressorMode     = "compressorMode";
constexpr auto kCompAmount         = "COMP_AMOUNT";
constexpr auto kLegacyCompAmount   = "compressorAmount";
constexpr auto kAuraLevel          = "COMP_AURA_LEVEL";
constexpr auto kEqEnable           = "eqEnable";
constexpr auto kEqLowShelfGain     = "eqLowShelfGainDb";
constexpr auto kEqHighShelfGain    = "eqHighShelfGainDb";
constexpr auto kEqBellHz           = "eqBellHz";
constexpr auto kEqBellGain         = "eqBellGainDb";
constexpr auto kEqBellQ            = "eqBellQ";
constexpr auto kOutput             = "output";
constexpr auto kCeiling            = "ceiling";
constexpr auto kLimiter            = "limiter";
constexpr auto kAura               = "aura";
constexpr auto kMicCharEnabled     = "MIC_CHAR_ENABLED";
constexpr auto kMicCharBypass      = "MIC_CHAR_BYPASS";
constexpr auto kMicCharDeHarsh     = "MIC_CHAR_DEHARSH";
constexpr auto kMicCharSibilance   = "MIC_CHAR_SIBILANCE";
constexpr auto kMicCharSimpleMode  = "MIC_CHAR_SIMPLE_MODE";
constexpr auto kCompEnabled        = "COMP_ENABLED";
constexpr auto kCompBypass         = "COMP_BYPASS";
constexpr auto kAuraBigBypass      = "AURA_BIG_BYPASS";
constexpr auto kEqGlobalBypass     = "EQ_GLOBAL_BYPASS";
constexpr auto kMicSectionEnable   = "micSectionEnable";
constexpr auto kPreampSectionEnable = "preampSectionEnable";
constexpr auto kHarmonicsSectionEnable = "harmonicsSectionEnable";
constexpr auto kSumSectionEnable   = "sumSectionEnable";
constexpr auto kMasterSectionEnable = "masterSectionEnable";
constexpr auto kGlue               = "glue";
constexpr auto kSumming            = "summing";

void setParam (juce::AudioProcessorValueTreeState& apvts, const char* id, float value)
{
    if (auto* parameter = apvts.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

void resetEqBands (juce::AudioProcessorValueTreeState& apvts)
{
    static const float defaultFreqs[24] = {
        20.f, 30.f, 50.f, 80.f, 120.f, 200.f, 300.f, 500.f,
        800.f, 1200.f, 2000.f, 3000.f, 4000.f, 5000.f, 6000.f, 7000.f,
        8000.f, 9000.f, 10000.f, 11000.f, 12000.f, 14000.f, 16000.f, 18000.f
    };

    for (int b = 1; b <= 24; ++b)
    {
        const auto bs  = juce::String (b).paddedLeft ('0', 2);
        const auto pfx = "EQ_BAND_" + bs + "_";
        setParam (apvts, (pfx + "ENABLED").toRawUTF8(), 1.0f);
        setParam (apvts, (pfx + "TYPE").toRawUTF8(), 0.0f);
        setParam (apvts, (pfx + "FREQ").toRawUTF8(), defaultFreqs[static_cast<size_t> (b - 1)]);
        setParam (apvts, (pfx + "GAIN").toRawUTF8(), 0.0f);
        setParam (apvts, (pfx + "Q").toRawUTF8(), 1.0f);
        setParam (apvts, (pfx + "SLOPE").toRawUTF8(), 1.0f);
        setParam (apvts, (pfx + "CHANNEL_MODE").toRawUTF8(), 0.0f);
        setParam (apvts, (pfx + "DYNAMIC_ENABLED").toRawUTF8(), 0.0f);
    }
}

float pctToMacro (float pct) noexcept
{
    return juce::jlimit (0.0f, 1.0f, pct * 0.01f);
}
} // namespace

const std::array<VocalChainPreset, 6>& getVocalChainPresets() noexcept
{
    // Mic profiles: 0=Vintage87, 1=SilkTube, 2=Golden251, 3=Crystal12, 4=Broadcast7, 9=CleanCapture
    // Preamp: 0=73VintageIron, 1=APIPunch, 2=AvalonClean
    // Console: 0=CleanConsole, 1=VintageDesk, 2=ModernPunch, 3=TubeConsole
    // tubeType: 0=CleanTriode, 1=WarmTriode, 3=VintagePentode, 4=BigBottle, 5=CreamOptoTube
    // compModel: 0=Aura2A, 2=AuraTube | compressorMode: 0=SmoothOpto, 2=Kid670
    static const std::array<VocalChainPreset, 6> presets {{
        { "Soul Lead",
          1, 2, 1, 34.0f,
          45.0f, 38.0f, pctToMacro (58.0f), pctToMacro (26.0f), pctToMacro (42.0f), 24.0f, 100.0f,
          0.18f, 1,
          0, 0, 35.0f, false,
          1.2f, -0.8f, 650.0f, 0.0f, 1.0f, false,
          0.25f, 0.20f, 0.0f, -1.0f, false },

        { "Warm Male Lead",
          1, 0, 3, 40.0f,
          42.0f, 42.0f, pctToMacro (62.0f), pctToMacro (22.0f), pctToMacro (38.0f), 28.0f, 100.0f,
          0.22f, 4,
          0, 0, 38.0f, false,
          0.8f, -0.5f, 650.0f, 0.0f, 1.0f, false,
          0.25f, 0.20f, 0.0f, -1.0f, false },

        { "Tube Warmth",
          0, 1, 1, 36.0f,
          35.0f, 55.0f, pctToMacro (60.0f), pctToMacro (18.0f), pctToMacro (34.0f), 35.0f, 100.0f,
          0.28f, 3,
          0, 2, 25.0f, false,
          1.5f, -0.4f, 650.0f, 0.0f, 1.0f, false,
          0.22f, 0.18f, 0.0f, -1.0f, false },

        { "Male De-Ess Control",
          4, 2, 2, 28.0f,
          65.0f, 18.0f, pctToMacro (46.0f), pctToMacro (12.0f), pctToMacro (30.0f), 12.0f, 100.0f,
          0.08f, 0,
          2, 0, 25.0f, false,
          0.0f, -0.6f, 6500.0f, -2.5f, 2.0f, true,
          0.55f, 0.45f, 0.0f, -1.0f, false },

        { "Vocal Leveler",
          9, 2, 2, 32.0f,
          50.0f, 25.0f, pctToMacro (50.0f), pctToMacro (28.0f), pctToMacro (44.0f), 18.0f, 100.0f,
          0.12f, 5,
          0, 0, 45.0f, true,
          0.5f, 0.0f, 650.0f, 0.0f, 1.0f, false,
          0.25f, 0.20f, 0.0f, -1.0f, false },

        { "Clean Vocal Start",
          9, 2, 0, 0.0f,
          35.0f, 10.0f, pctToMacro (50.0f), pctToMacro (30.0f), pctToMacro (45.0f), 8.0f, 100.0f,
          0.0f, 0,
          2, 0, 20.0f, false,
          0.0f, 0.0f, 650.0f, 0.0f, 1.0f, false,
          0.20f, 0.18f, 0.0f, -1.0f, true }
    }};

    return presets;
}

void applyVocalChainPreset (juce::AudioProcessorValueTreeState& apvts, const VocalChainPreset& preset)
{
    setParam (apvts, kMicCharEnabled, 1.0f);
    setParam (apvts, kMicCharBypass, 0.0f);
    setParam (apvts, kMicCharSimpleMode, 1.0f);
    setParam (apvts, kMicSectionEnable, 1.0f);
    setParam (apvts, kPreampSectionEnable, 1.0f);
    setParam (apvts, kHarmonicsSectionEnable, 1.0f);
    setParam (apvts, kSumSectionEnable, 1.0f);
    setParam (apvts, kMasterSectionEnable, 1.0f);
    setParam (apvts, kCompEnabled, 1.0f);
    setParam (apvts, kCompBypass, 0.0f);
    setParam (apvts, kAuraBigBypass, 0.0f);
    setParam (apvts, kEqGlobalBypass, 0.0f);
    setParam (apvts, kEqEnable, 1.0f);
    setParam (apvts, kLimiter, 1.0f);
    setParam (apvts, "bypass", 0.0f);

    setParam (apvts, kMicCharProfile, static_cast<float> (preset.micCharProfile));
    setParam (apvts, kMicCorrection, preset.micCorrection);
    setParam (apvts, kTubeDrive, preset.tubeDrive);
    setParam (apvts, kMicBody, preset.micBody);
    setParam (apvts, kMicAir, preset.micAir);
    setParam (apvts, kMicPresence, preset.micPresence);
    setParam (apvts, kPreampDrive, preset.preampDrive);
    setParam (apvts, kMix, preset.mix);
    setParam (apvts, kAuraBigAmount, preset.auraBigAmount);
    setParam (apvts, kAura, preset.auraBigAmount * 100.0f);
    setParam (apvts, kTubeType, static_cast<float> (preset.tubeType));
    setParam (apvts, kPreampMode, static_cast<float> (preset.preampMode));
    setParam (apvts, kConsoleMode, static_cast<float> (preset.consoleMode));
    setParam (apvts, kConsoleDensity, preset.consoleDensity);
    setParam (apvts, kCompModel, static_cast<float> (preset.compModel));
    setParam (apvts, kCompressorMode, static_cast<float> (preset.compressorMode));
    setParam (apvts, kCompAmount, preset.compAmount);
    setParam (apvts, kLegacyCompAmount, preset.compAmount);
    setParam (apvts, kAuraLevel, preset.auraLevel ? 1.0f : 0.0f);
    setParam (apvts, kMicCharDeHarsh, preset.micDeHarsh);
    setParam (apvts, kMicCharSibilance, preset.micSibilance);
    setParam (apvts, kOutput, preset.outputDb);
    setParam (apvts, kCeiling, preset.ceilingDb);

    setParam (apvts, "eqHpfHz", 35.0f);
    setParam (apvts, "eqLowShelfHz", 120.0f);
    setParam (apvts, kEqLowShelfGain, preset.eqLowShelfGainDb);
    setParam (apvts, kEqBellHz, preset.eqBellHz);
    setParam (apvts, kEqBellGain, preset.eqBellGainDb);
    setParam (apvts, kEqBellQ, preset.eqBellQ);
    setParam (apvts, "eqHighShelfHz", 6500.0f);
    setParam (apvts, kEqHighShelfGain, preset.eqHighShelfGainDb);
    setParam (apvts, "eqLpfHz", 18000.0f);

    setParam (apvts, "sourceMicMode", 0.0f);
    setParam (apvts, "targetMicMode", 3.0f);
    setParam (apvts, "micTargetAmount", 50.0f);
    setParam (apvts, "badFrequencyTamer", preset.micCorrection);
    setParam (apvts, "airProtection", 50.0f);
    setParam (apvts, "bodyProtection", 50.0f);
    setParam (apvts, "hardwareSafeMode", 1.0f);
    setParam (apvts, "tubeOutputDb", 0.0f);
    setParam (apvts, "compressorEnable", 1.0f);
    setParam (apvts, "compThresholdDb", -18.0f);
    setParam (apvts, "compRatio", 4.0f);
    setParam (apvts, "compAttackMs", 20.0f);
    setParam (apvts, "compReleaseMs", 400.0f);
    setParam (apvts, "compMakeupDb", 0.0f);
    setParam (apvts, "compBleedPercent", 0.0f);
    setParam (apvts, "compSidechainHpfHz", 80.0f);
    setParam (apvts, "compTimingMode", 0.0f);
    setParam (apvts, "vuMeterMode", 1.0f);
    setParam (apvts, "width", 100.0f);
    setParam (apvts, "monoCheck", 0.0f);
    setParam (apvts, "makeup", 0.0f);
    setParam (apvts, "dim", 0.0f);
    setParam (apvts, "quality", 1.0f);
    setParam (apvts, "COMP_MIX", 100.0f);
    setParam (apvts, "COMP_TIMING_MODE", 0.0f);
    setParam (apvts, "COMP_PROFILE", 0.0f);
    setParam (apvts, "COMP_DETECTOR_MODE", 1.0f);
    setParam (apvts, "COMP_LINK", 1.0f);

    if (preset.minimalConsole)
    {
        setParam (apvts, kGlue, 0.0f);
        setParam (apvts, kSumming, 0.0f);
    }
    else
    {
        setParam (apvts, kGlue, 18.0f);
        setParam (apvts, kSumming, 16.0f);
    }

    resetEqBands (apvts);

    if (preset.deEssBand6500)
    {
        setParam (apvts, "EQ_BAND_14_ENABLED", 1.0f);
        setParam (apvts, "EQ_BAND_14_TYPE", 0.0f);
        setParam (apvts, "EQ_BAND_14_FREQ", 6500.0f);
        setParam (apvts, "EQ_BAND_14_GAIN", -2.5f);
        setParam (apvts, "EQ_BAND_14_Q", 2.0f);
    }
}

} // namespace StadiumAuraPresets
