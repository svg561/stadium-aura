#pragma once

#include <JuceHeader.h>

#if JucePlugin_Build_Standalone
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

namespace StandaloneAudio
{
inline bool isStandaloneBuild() noexcept
{
   #if JucePlugin_Build_Standalone
    return true;
   #else
    return false;
   #endif
}

inline void configureEditorControls (juce::Button& settingsButton, juce::Button& helpButton)
{
   #if JucePlugin_Build_Standalone
    if (auto* holder = juce::StandalonePluginHolder::getInstance())
    {
        holder->getMuteInputValue().setValue (false);
        holder->saveAudioDeviceState();
        settingsButton.onClick = [holder] { holder->showAudioSettingsDialog(); };
        settingsButton.setTooltip ("Audio / MIDI settings — choose input mic, output headphones, and unmute input.");
        helpButton.setTooltip ("No sound? Press G → pick your mic as Input, headphones as Output, "
                               "uncheck 'Mute audio input', and grant Microphone access in macOS Privacy settings.");
    }
   #else
    settingsButton.setEnabled (false);
    settingsButton.setTooltip ("Audio device settings are Standalone-only.");
    helpButton.setTooltip ("Use your DAW's audio settings for routing.");
   #endif
}

inline juce::String getMonitorStatusText()
{
   #if JucePlugin_Build_Standalone
    if (auto* holder = juce::StandalonePluginHolder::getInstance())
    {
        if (holder->getMuteInputValue().getValue())
            return "INPUT MUTED — press G and uncheck Mute audio input";

        if (holder->getProcessorHasPotentialFeedbackLoop())
            return "LIVE MONITOR — use headphones";
    }
    return "LIVE MONITOR";
   #else
    return {};
   #endif
}

inline bool isInputMuted()
{
   #if JucePlugin_Build_Standalone
    if (auto* holder = juce::StandalonePluginHolder::getInstance())
        return holder->getMuteInputValue().getValue();
   #endif
    return false;
}
}
