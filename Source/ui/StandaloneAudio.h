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

// Call this repeatedly for the first few seconds after launch to override
// any saved mute state that JUCE loads asynchronously.
inline void forceUnmuteInput()
{
   #if JucePlugin_Build_Standalone
    if (auto* holder = juce::StandalonePluginHolder::getInstance())
        if ((bool) holder->getMuteInputValue().getValue())
            holder->getMuteInputValue().setValue (false);
   #endif
}

// Returns true if no input device is currently active (first launch / not configured).
inline bool hasNoInputDevice()
{
   #if JucePlugin_Build_Standalone
    if (auto* holder = juce::StandalonePluginHolder::getInstance())
        if (auto* dm = holder->deviceManager.getCurrentAudioDevice())
            return dm->getActiveInputChannels().isZero();
    return true;
   #else
    return false;
   #endif
}

// Open the audio settings dialog immediately (call once on first launch).
inline void showAudioSetup()
{
   #if JucePlugin_Build_Standalone
    if (auto* holder = juce::StandalonePluginHolder::getInstance())
        holder->showAudioSettingsDialog();
   #endif
}

// Try to activate the first available input device automatically.
// Returns true if an input was found and activated.
inline bool autoSelectDefaultInput()
{
   #if JucePlugin_Build_Standalone
    if (auto* holder = juce::StandalonePluginHolder::getInstance())
    {
        auto& dm = holder->deviceManager;
        if (auto* currentDevice = dm.getCurrentAudioDevice())
        {
            // Already have a device — just make sure input is enabled
            auto setup = dm.getAudioDeviceSetup();
            if (setup.inputDeviceName.isEmpty())
            {
                // Try to pick any available input device
                for (auto* type : dm.getAvailableDeviceTypes())
                {
                    auto inputs = type->getDeviceNames (true);
                    if (! inputs.isEmpty())
                    {
                        setup.inputDeviceName = inputs[0];
                        setup.useDefaultInputChannels = true;
                        dm.setAudioDeviceSetup (setup, true);
                        holder->getMuteInputValue().setValue (false);
                        holder->saveAudioDeviceState();
                        return true;
                    }
                }
            }
            else
            {
                // Device name is set — enable all input channels and unmute
                auto active = currentDevice->getActiveInputChannels();
                if (active.isZero())
                {
                    setup.inputChannels.setRange (0, currentDevice->getInputChannelNames().size(), true);
                    dm.setAudioDeviceSetup (setup, true);
                }
                holder->getMuteInputValue().setValue (false);
                holder->saveAudioDeviceState();
                return true;
            }
        }
    }
   #endif
    return false;
}
}
