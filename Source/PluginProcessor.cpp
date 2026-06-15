#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "dsp/EQBand.h"
#include "dsp/AuraCompressorEngine.h"

// Set to true to bypass all DSP and pass mic input directly to output.
// Flip to false once standalone audio I/O is confirmed working.
static constexpr bool FORCE_RAW_PASSTHROUGH = true;

namespace Param
{
constexpr auto input = "input"; constexpr auto output = "output"; constexpr auto mix = "mix";
constexpr auto aura = "aura"; constexpr auto tubeDrive = "tubeDrive"; constexpr auto saturation = "saturation";
constexpr auto harmonicBias = "harmonicBias"; constexpr auto transformer = "transformer";
constexpr auto summing = "summing"; constexpr auto glue = "glue"; constexpr auto trackCount = "trackCount";
constexpr auto width = "width"; constexpr auto monoCheck = "monoCheck"; constexpr auto limiter = "limiter";
constexpr auto ceiling = "ceiling"; constexpr auto compressorMode = "compressorMode";
constexpr auto compressorAmount = "compressorAmount"; constexpr auto makeup = "makeup";
constexpr auto bypass = "bypass"; constexpr auto quality = "quality";
constexpr auto micCharacter = "micCharacter"; constexpr auto preampMode = "preampMode";
constexpr auto tubeSwap = "tubeSwap"; constexpr auto dim = "dim";
constexpr auto tone = "tone";
constexpr auto eqEnable = "eqEnable"; constexpr auto eqHpfHz = "eqHpfHz";
constexpr auto eqLowShelfHz = "eqLowShelfHz"; constexpr auto eqLowShelfGainDb = "eqLowShelfGainDb";
constexpr auto eqBellHz = "eqBellHz"; constexpr auto eqBellGainDb = "eqBellGainDb"; constexpr auto eqBellQ = "eqBellQ";
constexpr auto eqHighShelfHz = "eqHighShelfHz"; constexpr auto eqHighShelfGainDb = "eqHighShelfGainDb";
constexpr auto eqLpfHz = "eqLpfHz";
constexpr auto sourceMicMode = "sourceMicMode"; constexpr auto targetMicMode = "targetMicMode";
constexpr auto micCorrectionAmount = "micCorrectionAmount"; constexpr auto micTargetAmount = "micTargetAmount";
constexpr auto badFrequencyTamer = "badFrequencyTamer"; constexpr auto airProtection = "airProtection";
constexpr auto bodyProtection = "bodyProtection"; constexpr auto hardwareSafeMode = "hardwareSafeMode";
constexpr auto preampDrive = "preampDrive"; constexpr auto tubeType = "tubeType";
constexpr auto tubeOutputDb = "tubeOutputDb"; constexpr auto consoleMode = "consoleMode";
constexpr auto consoleDensity = "consoleDensity"; constexpr auto compressorEnable = "compressorEnable";
constexpr auto compThresholdDb = "compThresholdDb"; constexpr auto compRatio = "compRatio";
constexpr auto compAttackMs = "compAttackMs"; constexpr auto compReleaseMs = "compReleaseMs";
constexpr auto compMakeupDb = "compMakeupDb"; constexpr auto compBleedPercent = "compBleedPercent";
constexpr auto compSidechainHpfHz = "compSidechainHpfHz"; constexpr auto compTimingMode = "compTimingMode";
constexpr auto vuMeterMode = "vuMeterMode";
constexpr auto micSectionEnable = "micSectionEnable";
constexpr auto preampSectionEnable = "preampSectionEnable";
constexpr auto harmonicsSectionEnable = "harmonicsSectionEnable";
constexpr auto sumSectionEnable = "sumSectionEnable";
constexpr auto masterSectionEnable = "masterSectionEnable";
}

StadiumAuraAudioProcessor::StadiumAuraAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout StadiumAuraAudioProcessor::createParameterLayout()
{
    using Float = juce::AudioParameterFloat;
    using Bool = juce::AudioParameterBool;
    using Choice = juce::AudioParameterChoice;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    auto percent = [] (float value, int) { return juce::String (value, 1) + " %"; };
    auto db = [] (float value, int) { return juce::String (value, 1) + " dB"; };
    auto hz = [] (float value, int) { return value >= 1000.0f ? juce::String (value / 1000.0f, 2) + " kHz" : juce::String (value, 0) + " Hz"; };
    auto milliseconds = [] (float value, int) { return juce::String (value, value < 10.0f ? 2 : 0) + " ms"; };

    layout.add (std::make_unique<Float> (Param::input, "Input", juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f, "dB", juce::AudioProcessorParameter::genericParameter, db));
    layout.add (std::make_unique<Float> (Param::output, "Output", juce::NormalisableRange<float> (-24.0f, 12.0f, 0.01f), 0.0f, "dB", juce::AudioProcessorParameter::genericParameter, db));
    layout.add (std::make_unique<Float> (Param::mix, "Mix", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 100.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::aura, "Aura", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 50.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::tubeDrive, "Tube Drive", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 25.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::saturation, "Saturation", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 20.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::harmonicBias, "Harmonic Bias", juce::NormalisableRange<float> (-100.0f, 100.0f, 0.01f), 0.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::transformer, "Transformer", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 20.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::summing, "Summing", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 20.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::glue, "Glue", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 20.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::tone, "Tone", juce::NormalisableRange<float> (-100.0f, 100.0f, 0.01f), 0.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Bool> (Param::eqEnable, "EQ Enable", true));
    layout.add (std::make_unique<Float> (Param::eqHpfHz, "EQ HPF", juce::NormalisableRange<float> (20.0f, 500.0f, 0.01f, 0.35f), 35.0f, "Hz", juce::AudioProcessorParameter::genericParameter, hz));
    layout.add (std::make_unique<Float> (Param::eqLowShelfHz, "EQ Low Shelf Frequency", juce::NormalisableRange<float> (40.0f, 500.0f, 0.01f, 0.45f), 120.0f, "Hz", juce::AudioProcessorParameter::genericParameter, hz));
    layout.add (std::make_unique<Float> (Param::eqLowShelfGainDb, "EQ Low Shelf Gain", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f, "dB", juce::AudioProcessorParameter::genericParameter, db));
    layout.add (std::make_unique<Float> (Param::eqBellHz, "EQ Bell Frequency", juce::NormalisableRange<float> (120.0f, 10000.0f, 0.01f, 0.32f), 650.0f, "Hz", juce::AudioProcessorParameter::genericParameter, hz));
    layout.add (std::make_unique<Float> (Param::eqBellGainDb, "EQ Bell Gain", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f, "dB", juce::AudioProcessorParameter::genericParameter, db));
    layout.add (std::make_unique<Float> (Param::eqBellQ, "EQ Bell Q", juce::NormalisableRange<float> (0.2f, 8.0f, 0.01f, 0.45f), 1.0f));
    layout.add (std::make_unique<Float> (Param::eqHighShelfHz, "EQ High Shelf Frequency", juce::NormalisableRange<float> (1500.0f, 16000.0f, 0.01f, 0.45f), 6500.0f, "Hz", juce::AudioProcessorParameter::genericParameter, hz));
    layout.add (std::make_unique<Float> (Param::eqHighShelfGainDb, "EQ High Shelf Gain", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f, "dB", juce::AudioProcessorParameter::genericParameter, db));
    layout.add (std::make_unique<Float> (Param::eqLpfHz, "EQ LPF", juce::NormalisableRange<float> (4000.0f, 20000.0f, 0.01f, 0.5f), 18000.0f, "Hz", juce::AudioProcessorParameter::genericParameter, hz));
    layout.add (std::make_unique<Choice> (Param::trackCount, "Track Count", juce::StringArray { "1", "8", "16", "24", "32" }, 2));
    layout.add (std::make_unique<Choice> (Param::micCharacter, "Mic Character", juce::StringArray { "67 Vintage Smooth", "C12 Open Air", "251 Classic", "87 Modern Balanced", "47 Velvet Tube", "251E Silky Air", "800G Air Pop" }, 3));
    layout.add (std::make_unique<Choice> (Param::sourceMicMode, "Source Mic", juce::StringArray {
        "Unknown / Auto", "Dynamic General", "Condenser General", "Ribbon General", "57-Style Dynamic",
        "7B-Style Dynamic", "C80-Style Condenser", "Bright Condenser", "Dark Condenser", "Warm Tube Mic", "Flat / Measurement" }, 0));
    layout.add (std::make_unique<Choice> (Param::targetMicMode, "Target Mic", juce::StringArray {
        "67 Vintage Smooth", "C12 Open Air", "251 Classic Silk", "87 Modern Balanced", "47 Velvet Tube", "251E Silky Presence", "800G Air Pop" }, 3));
    layout.add (std::make_unique<Float> (Param::micCorrectionAmount, "Mic Correction", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 35.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::micTargetAmount, "Mic Target", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 50.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::badFrequencyTamer, "Bad Frequency Tamer", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 35.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::airProtection, "Air Protection", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 50.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::bodyProtection, "Body Protection", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 50.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Bool> (Param::hardwareSafeMode, "Hardware Safe", true));
    layout.add (std::make_unique<Choice> (Param::preampMode, "Preamp Architecture", juce::StringArray { "73 Vintage Iron", "API Punch", "Avalon Clean" }, 0));
    layout.add (std::make_unique<Float> (Param::preampDrive, "Preamp Drive", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f, 0.55f), 25.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Choice> (Param::tubeSwap, "Tube Swap", juce::StringArray { "12AX7", "12AU7", "Clean Triode" }, 0));
    layout.add (std::make_unique<Choice> (Param::tubeType, "Tube Type", juce::StringArray { "Clean Triode", "Warm Triode", "Hot Triode", "Vintage Pentode", "Big Bottle", "Cream Opto Tube" }, 1));
    layout.add (std::make_unique<Float> (Param::tubeOutputDb, "Tube Output", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f, "dB", juce::AudioProcessorParameter::genericParameter, db));
    layout.add (std::make_unique<Choice> (Param::consoleMode, "Console Mode", juce::StringArray { "Clean Console", "Vintage Desk", "Modern Punch", "Tube Console" }, 0));
    layout.add (std::make_unique<Float> (Param::consoleDensity, "Console Density", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f, 0.65f), 20.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::width, "Width", juce::NormalisableRange<float> (0.0f, 150.0f, 0.01f), 100.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Bool> (Param::monoCheck, "Mono Check", false));
    layout.add (std::make_unique<Bool> (Param::limiter, "Limiter", true));
    layout.add (std::make_unique<Float> (Param::ceiling, "Limiter Ceiling", juce::NormalisableRange<float> (-12.0f, 0.0f, 0.01f), -1.0f, "dB", juce::AudioProcessorParameter::genericParameter, db));
    layout.add (std::make_unique<Bool> (Param::compressorEnable, "Compressor Enable", true));
    layout.add (std::make_unique<Choice> (Param::compressorMode, "Compressor Mode", juce::StringArray { "Smooth Opto", "Fast 76", "Kid670 Vari-Mu" }, 0));
    layout.add (std::make_unique<Float> (Param::compressorAmount, "Compressor Amount", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 0.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::compThresholdDb, "Compressor Threshold", juce::NormalisableRange<float> (-40.0f, 10.0f, 0.01f), -18.0f, "dB", juce::AudioProcessorParameter::genericParameter, db));
    layout.add (std::make_unique<Float> (Param::compRatio, "Compressor Ratio", juce::NormalisableRange<float> (1.0f, 20.0f, 0.01f, 0.55f), 4.0f));
    layout.add (std::make_unique<Float> (Param::compAttackMs, "Compressor Attack", juce::NormalisableRange<float> (0.02f, 300.0f, 0.01f, 0.28f), 20.0f, "ms", juce::AudioProcessorParameter::genericParameter, milliseconds));
    layout.add (std::make_unique<Float> (Param::compReleaseMs, "Compressor Release", juce::NormalisableRange<float> (50.0f, 10000.0f, 0.1f, 0.32f), 400.0f, "ms", juce::AudioProcessorParameter::genericParameter, milliseconds));
    layout.add (std::make_unique<Float> (Param::compMakeupDb, "Compressor Makeup", juce::NormalisableRange<float> (-12.0f, 24.0f, 0.01f), 0.0f, "dB", juce::AudioProcessorParameter::genericParameter, db));
    layout.add (std::make_unique<Float> (Param::compBleedPercent, "Compressor Bleed", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 0.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::compSidechainHpfHz, "Sidechain HPF", juce::NormalisableRange<float> (20.0f, 300.0f, 0.1f, 0.45f), 80.0f, "Hz"));
    layout.add (std::make_unique<Choice> (Param::compTimingMode, "Compressor Timing", juce::StringArray { "Manual", "Fixed", "Fixed / Manual" }, 0));
    layout.add (std::make_unique<Choice> (Param::vuMeterMode, "VU Meter Mode", juce::StringArray { "Input", "Gain Reduction", "Output" }, 1));
    layout.add (std::make_unique<Bool> (Param::micSectionEnable, "Mic Section Enable", true));
    layout.add (std::make_unique<Bool> (Param::preampSectionEnable, "Preamp Section Enable", true));
    layout.add (std::make_unique<Bool> (Param::harmonicsSectionEnable, "Harmonics Section Enable", true));
    layout.add (std::make_unique<Bool> (Param::sumSectionEnable, "Sum Section Enable", true));
    layout.add (std::make_unique<Bool> (Param::masterSectionEnable, "Master Section Enable", true));
    layout.add (std::make_unique<Float> (Param::makeup, "Makeup Gain", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f, "dB", juce::AudioProcessorParameter::genericParameter, db));
    layout.add (std::make_unique<Bool> (Param::bypass, "Bypass", false));
    layout.add (std::make_unique<Bool> (Param::dim, "Dim", false));
    layout.add (std::make_unique<Choice> (Param::quality, "Quality", juce::StringArray { "Eco", "Normal", "High", "Ultra" }, 1));

    // 24-band EQ parameters
    static const float defaultFreqs[24] = {
        20.0f, 30.0f, 50.0f, 80.0f, 120.0f, 200.0f, 300.0f, 500.0f,
        800.0f, 1200.0f, 2000.0f, 3000.0f, 4000.0f, 5000.0f, 6000.0f, 7000.0f,
        8000.0f, 9000.0f, 10000.0f, 11000.0f, 12000.0f, 14000.0f, 16000.0f, 18000.0f
    };
    for (int b = 1; b <= 24; ++b)
    {
        const auto bs  = juce::String (b).paddedLeft ('0', 2);
        const auto pfx = "EQ_BAND_" + bs + "_";
        const auto bn  = juce::String (b);
        layout.add (std::make_unique<Bool>   (pfx + "ENABLED",        "EQ Band " + bn + " Enable",   true));
        layout.add (std::make_unique<Choice> (pfx + "TYPE",           "EQ Band " + bn + " Type",
            juce::StringArray { "Bell","Low Cut","High Cut","Low Shelf","High Shelf","Notch","Tilt" }, 0));
        layout.add (std::make_unique<Float>  (pfx + "FREQ",           "EQ Band " + bn + " Freq",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.01f, 0.3f), defaultFreqs[b - 1],
            "Hz", juce::AudioProcessorParameter::genericParameter, hz));
        layout.add (std::make_unique<Float>  (pfx + "GAIN",           "EQ Band " + bn + " Gain",
            juce::NormalisableRange<float> (-30.0f, 30.0f, 0.01f), 0.0f,
            "dB", juce::AudioProcessorParameter::genericParameter, db));
        layout.add (std::make_unique<Float>  (pfx + "Q",              "EQ Band " + bn + " Q",
            juce::NormalisableRange<float> (0.025f, 40.0f, 0.001f, 0.3f), 1.0f));
        layout.add (std::make_unique<Choice> (pfx + "SLOPE",          "EQ Band " + bn + " Slope",
            juce::StringArray { "6","12","18","24","36","48","72","96" }, 1));
        layout.add (std::make_unique<Choice> (pfx + "CHANNEL_MODE",   "EQ Band " + bn + " Channel",
            juce::StringArray { "Stereo","Mid","Side","Left","Right" }, 0));
        layout.add (std::make_unique<Bool>   (pfx + "DYNAMIC_ENABLED","EQ Band " + bn + " Dynamic",  false));
        layout.add (std::make_unique<Float>  (pfx + "DYNAMIC_RANGE",  "EQ Band " + bn + " Dyn Range",
            juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f,
            "dB", juce::AudioProcessorParameter::genericParameter, db));
        layout.add (std::make_unique<Float>  (pfx + "THRESHOLD",      "EQ Band " + bn + " Threshold",
            juce::NormalisableRange<float> (-60.0f, 0.0f, 0.01f), -24.0f,
            "dB", juce::AudioProcessorParameter::genericParameter, db));
        layout.add (std::make_unique<Float>  (pfx + "ATTACK",         "EQ Band " + bn + " Attack",
            juce::NormalisableRange<float> (0.1f, 200.0f, 0.1f), 10.0f,
            "ms", juce::AudioProcessorParameter::genericParameter, milliseconds));
        layout.add (std::make_unique<Float>  (pfx + "RELEASE",        "EQ Band " + bn + " Release",
            juce::NormalisableRange<float> (10.0f, 2000.0f, 0.1f), 120.0f,
            "ms", juce::AudioProcessorParameter::genericParameter, milliseconds));
    }

    // ── New compressor engine parameters (COMP_* / EMOTION_LOCK_*) ──────────────
    auto ratio01  = [] (float v, int) { return juce::String (v, 2) + ":1"; };
    layout.add (std::make_unique<Bool>   ("COMP_ENABLED",          "Comp Engine Enable",          true));
    layout.add (std::make_unique<Choice> ("COMP_MODEL",            "Comp Model",
        juce::StringArray { "Lightning FET", "Velvet Opto", "Crown Mu", "Punch Cell", "Glue Bus", "Modern Clean" }, 1));
    layout.add (std::make_unique<Float>  ("COMP_INPUT",            "Comp Input",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f,    "dB",  juce::AudioProcessorParameter::genericParameter, db));
    layout.add (std::make_unique<Float>  ("COMP_THRESHOLD",        "Comp Threshold",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.01f), -24.0f,   "dB",  juce::AudioProcessorParameter::genericParameter, db));
    layout.add (std::make_unique<Float>  ("COMP_AMOUNT",           "Comp Amount",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 50.0f,    "%",   juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float>  ("COMP_RATIO",            "Comp Ratio",
        juce::NormalisableRange<float> (1.0f, 20.0f, 0.01f, 0.4f), 3.0f, "",   juce::AudioProcessorParameter::genericParameter, ratio01));
    layout.add (std::make_unique<Float>  ("COMP_ATTACK",           "Comp Attack",
        juce::NormalisableRange<float> (0.02f, 100.0f, 0.01f, 0.3f), 10.0f, "ms", juce::AudioProcessorParameter::genericParameter, milliseconds));
    layout.add (std::make_unique<Float>  ("COMP_RELEASE",          "Comp Release",
        juce::NormalisableRange<float> (20.0f, 5000.0f, 0.1f, 0.3f), 300.0f, "ms", juce::AudioProcessorParameter::genericParameter, milliseconds));
    layout.add (std::make_unique<Float>  ("COMP_MAKEUP",           "Comp Makeup",
        juce::NormalisableRange<float> (-12.0f, 24.0f, 0.01f), 0.0f,    "dB",  juce::AudioProcessorParameter::genericParameter, db));
    layout.add (std::make_unique<Float>  ("COMP_MIX",              "Comp Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 100.0f,   "%",   juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float>  ("COMP_SIDECHAIN_HPF",    "Comp SC HPF",
        juce::NormalisableRange<float> (20.0f, 300.0f, 0.1f, 0.3f), 90.0f, "Hz", juce::AudioProcessorParameter::genericParameter, hz));
    layout.add (std::make_unique<Bool>   ("COMP_AUTO_GAIN",        "Comp Auto Gain",              false));
    layout.add (std::make_unique<Bool>   ("COMP_AURA_LEVEL",       "Aura Level",                  false));
    layout.add (std::make_unique<Choice> ("COMP_DETECTOR_MODE",    "Comp Detector",
        juce::StringArray { "Peak", "RMS", "Vocal Focus", "Mid", "Side" }, 1));
    layout.add (std::make_unique<Bool>   ("COMP_LINK",             "Comp Stereo Link",            true));
    layout.add (std::make_unique<Float>  ("COMP_SATURATION",       "Comp Saturation",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 0.0f,     "%",   juce::AudioProcessorParameter::genericParameter, percent));

    layout.add (std::make_unique<Bool>   ("EMOTION_LOCK_ENABLED",          "Emotion Lock Enable",    false));
    layout.add (std::make_unique<Float>  ("EMOTION_LOCK_AMOUNT",           "Emotion Lock Amount",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<Float>  ("EMOTION_LOCK_BREATH_PROTECT",   "Emotion Lock Breath",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f));
    layout.add (std::make_unique<Float>  ("EMOTION_LOCK_PRESENCE_PROTECT", "Emotion Lock Presence",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<Float>  ("EMOTION_LOCK_AIR_PROTECT",      "Emotion Lock Air",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f));
    layout.add (std::make_unique<Float>  ("EMOTION_LOCK_HARSH_TAME",       "Emotion Lock Harsh",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));
    layout.add (std::make_unique<Float>  ("EMOTION_LOCK_POCKET_LOCK",      "Emotion Lock Pocket",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.4f));
    layout.add (std::make_unique<Float>  ("EMOTION_LOCK_INTENSITY",        "Emotion Lock Intensity",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

    return layout;
}

void StadiumAuraAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    auraProcessor.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    setLatencySamples (auraProcessor.getLatencySamples());
}

void StadiumAuraAudioProcessor::releaseResources() { auraProcessor.reset(); }

bool StadiumAuraAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    if (output != juce::AudioChannelSet::mono() && output != juce::AudioChannelSet::stereo())
        return false;
    // Also accept disabled input so the standalone can open before a mic is selected.
    return input == juce::AudioChannelSet::disabled()
        || input == output
        || (input == juce::AudioChannelSet::mono() && output == juce::AudioChannelSet::stereo());
}

void StadiumAuraAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());
    if (getTotalNumInputChannels() == 1 && getTotalNumOutputChannels() > 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, buffer.getNumSamples());

    inputMeter.store (peakForBuffer (buffer), std::memory_order_relaxed);
    inputLeftMeter.store (peakForChannel (buffer, 0), std::memory_order_relaxed);
    inputRightMeter.store (peakForChannel (buffer, juce::jmin (1, buffer.getNumChannels() - 1)), std::memory_order_relaxed);

    DBG ("INPUT RMS: " + juce::String (buffer.getMagnitude (0, 0, buffer.getNumSamples())));

    if (FORCE_RAW_PASSTHROUGH)
    {
        // Raw pass-through active: buffer goes unmodified from input to output.
        outputMeter.store (peakForBuffer (buffer), std::memory_order_relaxed);
        outputLeftMeter.store (peakForChannel (buffer, 0), std::memory_order_relaxed);
        outputRightMeter.store (peakForChannel (buffer, juce::jmin (1, buffer.getNumChannels() - 1)), std::memory_order_relaxed);
        gainReductionMeter.store (0.0f, std::memory_order_relaxed);
        newCompGainReduction.store (0.0f, std::memory_order_relaxed);
        limiterReductionMeter.store (0.0f, std::memory_order_relaxed);
        tubeActivityMeter.store (0.0f, std::memory_order_relaxed);
        updateAnalyzer (buffer);
        if (buffer.getNumChannels() > 0 && buffer.getNumSamples() > 0)
            spectrumAnalyzer.pushSamples (buffer.getReadPointer (0), buffer.getNumSamples());
        return;
    }

    auraProcessor.process (buffer, readParameters());
    outputMeter.store (peakForBuffer (buffer), std::memory_order_relaxed);
    outputLeftMeter.store (peakForChannel (buffer, 0), std::memory_order_relaxed);
    outputRightMeter.store (peakForChannel (buffer, juce::jmin (1, buffer.getNumChannels() - 1)), std::memory_order_relaxed);
    gainReductionMeter.store (-auraProcessor.getGainReductionDb(), std::memory_order_relaxed);
    newCompGainReduction.store (auraProcessor.getNewCompressorGainReductionDb(), std::memory_order_relaxed);
    limiterReductionMeter.store (-auraProcessor.getLimiterReductionDb(), std::memory_order_relaxed);
    tubeActivityMeter.store (auraProcessor.getTubeActivity(), std::memory_order_relaxed);
    updateAnalyzer (buffer);

    // Feed spectrum analyser (mono mix of first two channels)
    if (buffer.getNumChannels() > 0 && buffer.getNumSamples() > 0)
        spectrumAnalyzer.pushSamples (buffer.getReadPointer (0), buffer.getNumSamples());
}

AuraParameters StadiumAuraAudioProcessor::readParameters() const noexcept
{
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    AuraParameters p;
    p.inputGainDb = get (Param::input); p.outputGainDb = get (Param::output);
    p.mix = get (Param::mix) * 0.01f; p.aura = get (Param::aura) * 0.01f;
    p.tubeDrive = get (Param::tubeDrive) * 0.01f; p.saturation = get (Param::saturation) * 0.01f;
    p.harmonicBias = get (Param::harmonicBias) * 0.01f; p.transformer = get (Param::transformer) * 0.01f;
    p.summing = get (Param::summing) * 0.01f; p.glue = get (Param::glue) * 0.01f;
    p.tone = get (Param::tone) * 0.01f;
    p.eqEnabled = get (Param::eqEnable) > 0.5f;
    p.eqHpfHz = get (Param::eqHpfHz);
    p.eqLowShelfHz = get (Param::eqLowShelfHz); p.eqLowShelfGainDb = get (Param::eqLowShelfGainDb);
    p.eqBellHz = get (Param::eqBellHz); p.eqBellGainDb = get (Param::eqBellGainDb); p.eqBellQ = get (Param::eqBellQ);
    p.eqHighShelfHz = get (Param::eqHighShelfHz); p.eqHighShelfGainDb = get (Param::eqHighShelfGainDb);
    p.eqLpfHz = get (Param::eqLpfHz);
    constexpr int tracks[] { 1, 8, 16, 24, 32 };
    p.trackCount = tracks[juce::jlimit (0, 4, static_cast<int> (get (Param::trackCount)))];
    p.micCharacter = static_cast<MicCharacterProcessor::Mode> (juce::jlimit (0, 6, static_cast<int> (get (Param::micCharacter))));
    p.sourceMicMode = static_cast<MicCharacterProcessor::SourceMode> (juce::jlimit (0, 10, static_cast<int> (get (Param::sourceMicMode))));
    p.targetMicMode = static_cast<MicCharacterProcessor::Mode> (juce::jlimit (0, 6, static_cast<int> (get (Param::targetMicMode))));
    p.micCorrectionAmount = get (Param::micCorrectionAmount) * 0.01f;
    p.micTargetAmount = get (Param::micTargetAmount) * 0.01f;
    p.badFrequencyTamer = get (Param::badFrequencyTamer) * 0.01f;
    p.airProtection = get (Param::airProtection) * 0.01f;
    p.bodyProtection = get (Param::bodyProtection) * 0.01f;
    p.hardwareSafeMode = get (Param::hardwareSafeMode) > 0.5f;
    p.preampMode = static_cast<PreampArchitecture::Mode> (juce::jlimit (0, 2, static_cast<int> (get (Param::preampMode))));
    p.preampDrive = get (Param::preampDrive) * 0.01f;
    p.tubeSwap = juce::jlimit (0, 2, static_cast<int> (get (Param::tubeSwap)));
    p.tubeType = static_cast<TubeSaturation::Type> (juce::jlimit (0, 5, static_cast<int> (get (Param::tubeType))));
    p.tubeOutputDb = get (Param::tubeOutputDb);
    p.consoleMode = static_cast<SummingGlue::Mode> (juce::jlimit (0, 3, static_cast<int> (get (Param::consoleMode))));
    p.consoleDensity = get (Param::consoleDensity) * 0.01f;
    p.width = get (Param::width) * 0.01f; p.monoCheck = get (Param::monoCheck) > 0.5f;
    p.limiterEnabled = get (Param::limiter) > 0.5f; p.limiterCeilingDb = get (Param::ceiling);
    p.compressorMode = static_cast<CompressorSection::Mode> (juce::jlimit (0, 2, static_cast<int> (get (Param::compressorMode))));
    p.compressorAmount = get (Param::compressorAmount) * 0.01f; p.makeupGainDb = get (Param::makeup);
    p.compressorEnabled = get (Param::compressorEnable) > 0.5f;
    p.compressorThresholdDb = get (Param::compThresholdDb); p.compressorRatio = get (Param::compRatio);
    p.compressorAttackMs = get (Param::compAttackMs); p.compressorReleaseMs = get (Param::compReleaseMs);
    p.compressorMakeupDb = get (Param::compMakeupDb); p.compressorBleed = get (Param::compBleedPercent) * 0.01f;
    p.compressorSidechainHpfHz = get (Param::compSidechainHpfHz);
    p.compressorTimingMode = static_cast<int> (get (Param::compTimingMode));
    p.bypassed = get (Param::bypass) > 0.5f; p.dimmed = get (Param::dim) > 0.5f;
    p.quality = static_cast<int> (get (Param::quality));
    p.micSectionEnabled = get (Param::micSectionEnable) > 0.5f;
    p.preampSectionEnabled = get (Param::preampSectionEnable) > 0.5f;
    p.harmonicsSectionEnabled = get (Param::harmonicsSectionEnable) > 0.5f;
    p.sumSectionEnabled = get (Param::sumSectionEnable) > 0.5f;
    p.masterSectionEnabled = get (Param::masterSectionEnable) > 0.5f;

    static const float slopeTable[8] = { 6.f,12.f,18.f,24.f,36.f,48.f,72.f,96.f };
    for (int b = 0; b < 24; ++b)
    {
        const auto bs  = juce::String (b + 1).paddedLeft ('0', 2);
        const auto pfx = "EQ_BAND_" + bs + "_";
        auto& s = p.eqBands[static_cast<size_t> (b)];
        s.enabled       = apvts.getRawParameterValue (pfx + "ENABLED")->load()        > 0.5f;
        s.type          = static_cast<EQBandType>    (juce::jlimit (0, 6, (int) apvts.getRawParameterValue (pfx + "TYPE")->load()));
        s.frequencyHz   = apvts.getRawParameterValue (pfx + "FREQ")->load();
        s.gainDb        = apvts.getRawParameterValue (pfx + "GAIN")->load();
        s.q             = apvts.getRawParameterValue (pfx + "Q")->load();
        const int si    = juce::jlimit (0, 7, (int) apvts.getRawParameterValue (pfx + "SLOPE")->load());
        s.slopeDbPerOct = slopeTable[si];
        s.channelMode   = static_cast<EQChannelMode> (juce::jlimit (0, 4, (int) apvts.getRawParameterValue (pfx + "CHANNEL_MODE")->load()));
        s.dynamicEnabled  = apvts.getRawParameterValue (pfx + "DYNAMIC_ENABLED")->load() > 0.5f;
        s.dynamicRangeDb  = apvts.getRawParameterValue (pfx + "DYNAMIC_RANGE")->load();
        s.thresholdDb     = apvts.getRawParameterValue (pfx + "THRESHOLD")->load();
        s.attackMs        = apvts.getRawParameterValue (pfx + "ATTACK")->load();
        s.releaseMs       = apvts.getRawParameterValue (pfx + "RELEASE")->load();
    }

    // ── New compressor engine params ─────────────────────────────────────────────
    p.compressorParams.enabled      = get ("COMP_ENABLED") > 0.5f;
    p.compressorParams.model        = static_cast<AuraCompressorModel> (juce::jlimit (0, 5, static_cast<int> (get ("COMP_MODEL"))));
    p.compressorParams.inputDb      = get ("COMP_INPUT");
    p.compressorParams.thresholdDb  = get ("COMP_THRESHOLD");
    p.compressorParams.amount       = get ("COMP_AMOUNT") * 0.01f;
    p.compressorParams.ratio        = get ("COMP_RATIO");
    p.compressorParams.attackMs     = get ("COMP_ATTACK");
    p.compressorParams.releaseMs    = get ("COMP_RELEASE");
    p.compressorParams.makeupDb     = get ("COMP_MAKEUP");
    p.compressorParams.mix          = get ("COMP_MIX") * 0.01f;
    p.compressorParams.sidechainHPF = get ("COMP_SIDECHAIN_HPF");
    p.compressorParams.autoGain     = get ("COMP_AUTO_GAIN") > 0.5f;
    p.compressorParams.auraLevel    = get ("COMP_AURA_LEVEL") > 0.5f;
    p.compressorParams.detectorMode = static_cast<CompressorParams::DetectorMode> (
        juce::jlimit (0, 4, static_cast<int> (get ("COMP_DETECTOR_MODE"))));
    p.compressorParams.linkedStereo = get ("COMP_LINK") > 0.5f;
    p.compressorParams.saturation   = get ("COMP_SATURATION") * 0.01f;

    p.compressorParams.emotionLock.enabled         = get ("EMOTION_LOCK_ENABLED") > 0.5f;
    p.compressorParams.emotionLock.amount          = get ("EMOTION_LOCK_AMOUNT");
    p.compressorParams.emotionLock.breathProtect   = get ("EMOTION_LOCK_BREATH_PROTECT");
    p.compressorParams.emotionLock.presenceProtect = get ("EMOTION_LOCK_PRESENCE_PROTECT");
    p.compressorParams.emotionLock.airProtect      = get ("EMOTION_LOCK_AIR_PROTECT");
    p.compressorParams.emotionLock.harshTame       = get ("EMOTION_LOCK_HARSH_TAME");
    p.compressorParams.emotionLock.pocketLock      = get ("EMOTION_LOCK_POCKET_LOCK");
    p.compressorParams.emotionLock.intensity       = get ("EMOTION_LOCK_INTENSITY");

    return p;
}

float StadiumAuraAudioProcessor::peakForBuffer (const juce::AudioBuffer<float>& buffer) noexcept
{
    float peak = 0.0f;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        peak = juce::jmax (peak, buffer.getMagnitude (channel, 0, buffer.getNumSamples()));
    return peak;
}

float StadiumAuraAudioProcessor::peakForChannel (const juce::AudioBuffer<float>& buffer, int channel) noexcept
{
    if (buffer.getNumChannels() <= 0 || buffer.getNumSamples() <= 0)
        return 0.0f;
    channel = juce::jlimit (0, buffer.getNumChannels() - 1, channel);
    return buffer.getMagnitude (channel, 0, buffer.getNumSamples());
}

void StadiumAuraAudioProcessor::updateAnalyzer (const juce::AudioBuffer<float>& buffer) noexcept
{
    if (buffer.getNumSamples() <= 0 || buffer.getNumChannels() <= 0)
        return;

    constexpr float minFrequency = 35.0f;
    constexpr float maxFrequency = 18000.0f;
    const auto sampleRate = static_cast<float> (juce::jmax (1.0, getSampleRate()));

    for (int bin = 0; bin < analyzerBinCount; ++bin)
    {
        const auto norm = static_cast<float> (bin) / static_cast<float> (analyzerBinCount - 1);
        const auto frequency = minFrequency * std::pow (maxFrequency / minFrequency, norm);
        const auto coeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * frequency / sampleRate);
        auto low = analyzerLowpassState[static_cast<size_t> (bin)];
        auto previous = analyzerPreviousLowpass[static_cast<size_t> (bin)];
        float energy = 0.0f;

        for (int sample = 0; sample < buffer.getNumSamples(); sample += 2)
        {
            float mono = 0.0f;
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                mono += buffer.getSample (channel, sample);
            mono /= static_cast<float> (buffer.getNumChannels());

            low += coeff * (mono - low);
            const auto band = low - previous;
            previous = low;
            energy += band * band;
        }

        analyzerLowpassState[static_cast<size_t> (bin)] = low;
        analyzerPreviousLowpass[static_cast<size_t> (bin)] = previous;
        const auto rms = std::sqrt (energy / static_cast<float> (juce::jmax (1, buffer.getNumSamples() / 2)));
        const auto db = juce::Decibels::gainToDecibels (rms, -80.0f);
        const auto normalised = juce::jlimit (0.0f, 1.0f, juce::jmap (db, -72.0f, -12.0f, 0.0f, 1.0f));
        auto& smoothed = analyzerSmoothed[static_cast<size_t> (bin)];
        smoothed += (normalised - smoothed) * (normalised > smoothed ? 0.35f : 0.10f);
        analyzerBins[static_cast<size_t> (bin)].store (smoothed, std::memory_order_relaxed);
    }
}

StadiumAuraAudioProcessor::AnalyzerSnapshot StadiumAuraAudioProcessor::getAnalyzerSnapshot() const noexcept
{
    AnalyzerSnapshot snapshot {};
    for (int i = 0; i < analyzerBinCount; ++i)
        snapshot[static_cast<size_t> (i)] = analyzerBins[static_cast<size_t> (i)].load (std::memory_order_relaxed);
    return snapshot;
}

juce::AudioProcessorEditor* StadiumAuraAudioProcessor::createEditor()
{
    return new StadiumAuraAudioProcessorEditor (*this);
}

void StadiumAuraAudioProcessor::setCurrentProgram (int index)
{
    index = juce::jlimit (0, getNumPrograms() - 1, index);
    currentProgram = index;
    constexpr const char* ids[] { Param::input, Param::output, Param::mix, Param::aura, Param::tubeDrive,
        Param::saturation, Param::harmonicBias, Param::transformer, Param::summing, Param::glue,
        Param::tone, Param::trackCount, Param::micCharacter, Param::preampMode, Param::tubeSwap, Param::width,
        Param::monoCheck, Param::limiter, Param::ceiling, Param::compressorMode, Param::compressorAmount,
        Param::makeup, Param::bypass, Param::dim, Param::quality };
    for (size_t i = 0; i < std::size (ids); ++i)
        if (auto* parameter = apvts.getParameter (ids[i]))
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (factoryPresets[static_cast<size_t> (index)].values[i]));

    auto set = [this] (const char* id, float value)
    {
        if (auto* parameter = apvts.getParameter (id))
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
    };
    set (Param::sourceMicMode, 0); set (Param::targetMicMode, 3);
    set (Param::micCorrectionAmount, 35); set (Param::micTargetAmount, 50);
    set (Param::badFrequencyTamer, 35); set (Param::airProtection, 50); set (Param::bodyProtection, 50);
    set (Param::hardwareSafeMode, 1); set (Param::preampDrive, 25);
    set (Param::tubeType, 1); set (Param::tubeOutputDb, 0);
    set (Param::consoleMode, 0); set (Param::consoleDensity, 20);
    set (Param::eqEnable, 1); set (Param::eqHpfHz, 35);
    set (Param::eqLowShelfHz, 120); set (Param::eqLowShelfGainDb, 0);
    set (Param::eqBellHz, 650); set (Param::eqBellGainDb, 0); set (Param::eqBellQ, 1);
    set (Param::eqHighShelfHz, 6500); set (Param::eqHighShelfGainDb, 0); set (Param::eqLpfHz, 18000);
    set (Param::compressorEnable, 1); set (Param::compThresholdDb, -18); set (Param::compRatio, 4);
    set (Param::compAttackMs, 20); set (Param::compReleaseMs, 400); set (Param::compMakeupDb, 0);
    set (Param::compBleedPercent, 0); set (Param::compSidechainHpfHz, 80);
    set (Param::compTimingMode, 0); set (Param::vuMeterMode, 1);

    if (index == 1 || index == 7 || index == 8 || index == 9)
    {
        set (Param::sourceMicMode, index == 7 ? 7.0f : 1.0f);
        set (Param::targetMicMode, index == 8 ? 4.0f : (index == 7 ? 6.0f : 2.0f));
        set (Param::micCorrectionAmount, 48); set (Param::micTargetAmount, 62);
        set (Param::badFrequencyTamer, 48);
    }
    if (index == 2 || index == 10) { set (Param::compressorMode, 1); set (Param::compAttackMs, 1.0f); set (Param::compReleaseMs, 110); set (Param::compBleedPercent, 18); }
    if (index == 6) { set (Param::tubeType, 2); set (Param::tubeOutputDb, -2); set (Param::preampDrive, 65); }
    if (index == 11) { set (Param::consoleMode, 1); set (Param::consoleDensity, 46); set (Param::preampDrive, 38); }
    if (index == 12) { set (Param::aura, 50); set (Param::tubeType, 1); set (Param::consoleMode, 3); set (Param::consoleDensity, 32); }
}

const juce::String StadiumAuraAudioProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, getNumPrograms()) ? factoryPresets[static_cast<size_t> (index)].name : juce::String();
}

void StadiumAuraAudioProcessor::getStateInformation (juce::MemoryBlock& destinationData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destinationData);
}

void StadiumAuraAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StadiumAuraAudioProcessor();
}
