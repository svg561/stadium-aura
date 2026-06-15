#pragma once
#include <JuceHeader.h>

class StadiumAuraRecallManager
{
public:
    explicit StadiumAuraRecallManager (juce::AudioProcessorValueTreeState& apvts,
                                       juce::AudioProcessor& processor);

    // Generate a short unique chain ID like "SA-20260615-A9F3"
    juce::String createChainId() const;

    // Capture full APVTS state + session metadata into a juce::var JSON object
    juce::var createSnapshot (const juce::String& presetName = "Untitled");

    // Save snapshot JSON to a file (returns false on error)
    bool saveSnapshotToFile (const juce::File& file, const juce::String& presetName = "Untitled");

    // Load snapshot from JSON file (returns false on error)
    bool loadSnapshotFromFile (const juce::File& file);

    // Apply a loaded snapshot to the APVTS (restores all parameter values)
    void applySnapshot (const juce::var& snapshot);

    // Generate human-readable recall text for clipboard copy
    juce::String createHumanReadableRecallText (const juce::String& presetName = "Untitled");

    // Returns the last generated/loaded chain ID
    juce::String getLastChainId() const noexcept { return lastChainId; }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::AudioProcessor& processor;
    mutable juce::String lastChainId;

    juce::var serializeParameters() const;
    void deserializeParameters (const juce::var& params);

    juce::String getMicSummary()     const;
    juce::String getPreampSummary()  const;
    juce::String getConsoleSummary() const;
    juce::String getCompSummary()    const;
    juce::String getEQSummary()      const;
    juce::String getLimiterSummary() const;
};
