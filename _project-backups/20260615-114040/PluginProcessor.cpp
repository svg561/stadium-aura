#include "PluginProcessor.h"
#include "PluginEditor.h"

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

    layout.add (std::make_unique<Float> (Param::input, "Input", juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f, "dB", juce::AudioProcessorParameter::genericParameter, db));
    layout.add (std::make_unique<Float> (Param::output, "Output", juce::NormalisableRange<float> (-24.0f, 12.0f, 0.01f), 0.0f, "dB", juce::AudioProcessorParameter::genericParameter, db));
    layout.add (std::make_unique<Float> (Param::mix, "Mix", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 100.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::aura, "Aura", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 35.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::tubeDrive, "Tube Drive", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 20.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::saturation, "Saturation", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 20.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::harmonicBias, "Harmonic Bias", juce::NormalisableRange<float> (-100.0f, 100.0f, 0.01f), 0.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::transformer, "Transformer", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 15.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::summing, "Summing", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 20.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::glue, "Glue", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 20.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::tone, "Tone", juce::NormalisableRange<float> (-100.0f, 100.0f, 0.01f), 0.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Choice> (Param::trackCount, "Track Count", juce::StringArray { "1", "8", "16", "24", "32" }, 2));
    layout.add (std::make_unique<Choice> (Param::micCharacter, "Mic Character", juce::StringArray { "67 Vintage Smooth", "C12 Open Air", "251 Classic", "87 Modern Balanced", "47 Velvet Tube", "251E Silky Air", "800G Air Pop" }, 3));
    layout.add (std::make_unique<Choice> (Param::preampMode, "Preamp Architecture", juce::StringArray { "73 Vintage Iron", "Clean Class A", "2520 Punch" }, 0));
    layout.add (std::make_unique<Choice> (Param::tubeSwap, "Tube Swap", juce::StringArray { "12AX7", "12AU7", "Clean Triode" }, 0));
    layout.add (std::make_unique<Float> (Param::width, "Width", juce::NormalisableRange<float> (0.0f, 150.0f, 0.01f), 100.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Bool> (Param::monoCheck, "Mono Check", false));
    layout.add (std::make_unique<Bool> (Param::limiter, "Limiter", true));
    layout.add (std::make_unique<Float> (Param::ceiling, "Limiter Ceiling", juce::NormalisableRange<float> (-2.0f, -0.1f, 0.01f), -0.3f, "dB", juce::AudioProcessorParameter::genericParameter, db));
    layout.add (std::make_unique<Choice> (Param::compressorMode, "Compressor Mode", juce::StringArray { "Fast FET", "Smooth Opto", "Tube Leveler" }, 1));
    layout.add (std::make_unique<Float> (Param::compressorAmount, "Compressor Amount", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 0.0f, "%", juce::AudioProcessorParameter::genericParameter, percent));
    layout.add (std::make_unique<Float> (Param::makeup, "Makeup Gain", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f, "dB", juce::AudioProcessorParameter::genericParameter, db));
    layout.add (std::make_unique<Bool> (Param::bypass, "Bypass", false));
    layout.add (std::make_unique<Bool> (Param::dim, "Dim", false));
    layout.add (std::make_unique<Choice> (Param::quality, "Quality", juce::StringArray { "Eco", "Normal", "High", "Ultra" }, 1));
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
    const auto output = layouts.getMainOutputChannelSet();
    return (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo())
        && output == layouts.getMainInputChannelSet();
}

void StadiumAuraAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    inputMeter.store (peakForBuffer (buffer), std::memory_order_relaxed);
    auraProcessor.process (buffer, readParameters());
    outputMeter.store (peakForBuffer (buffer), std::memory_order_relaxed);
    gainReductionMeter.store (-auraProcessor.getGainReductionDb(), std::memory_order_relaxed);
    limiterReductionMeter.store (-auraProcessor.getLimiterReductionDb(), std::memory_order_relaxed);
    tubeActivityMeter.store (auraProcessor.getTubeActivity(), std::memory_order_relaxed);
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
    constexpr int tracks[] { 1, 8, 16, 24, 32 };
    p.trackCount = tracks[juce::jlimit (0, 4, static_cast<int> (get (Param::trackCount)))];
    p.micCharacter = static_cast<MicCharacterProcessor::Mode> (juce::jlimit (0, 6, static_cast<int> (get (Param::micCharacter))));
    p.preampMode = static_cast<PreampArchitecture::Mode> (juce::jlimit (0, 2, static_cast<int> (get (Param::preampMode))));
    p.tubeSwap = juce::jlimit (0, 2, static_cast<int> (get (Param::tubeSwap)));
    p.width = get (Param::width) * 0.01f; p.monoCheck = get (Param::monoCheck) > 0.5f;
    p.limiterEnabled = get (Param::limiter) > 0.5f; p.limiterCeilingDb = get (Param::ceiling);
    p.compressorMode = static_cast<CompressorSection::Mode> (juce::jlimit (0, 2, static_cast<int> (get (Param::compressorMode))));
    p.compressorAmount = get (Param::compressorAmount) * 0.01f; p.makeupGainDb = get (Param::makeup);
    p.bypassed = get (Param::bypass) > 0.5f; p.dimmed = get (Param::dim) > 0.5f;
    p.quality = static_cast<int> (get (Param::quality));
    return p;
}

float StadiumAuraAudioProcessor::peakForBuffer (const juce::AudioBuffer<float>& buffer) noexcept
{
    float peak = 0.0f;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        peak = juce::jmax (peak, buffer.getMagnitude (channel, 0, buffer.getNumSamples()));
    return peak;
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
