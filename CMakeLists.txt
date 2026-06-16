cmake_minimum_required(VERSION 3.22)

project(StadiumAura VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(STADIUM_AURA_AAX_SDK_PATH "" CACHE PATH "Path to the official Avid AAX SDK root")
set(STADIUM_AURA_FORMATS AU VST3 Standalone)

if (STADIUM_AURA_AAX_SDK_PATH)
    if (NOT EXISTS "${STADIUM_AURA_AAX_SDK_PATH}/Interfaces/ACF")
        message(FATAL_ERROR
            "STADIUM_AURA_AAX_SDK_PATH does not point to a valid AAX SDK root. "
            "Expected Interfaces/ACF beneath: ${STADIUM_AURA_AAX_SDK_PATH}")
    endif()
    set(JUCE_GLOBAL_AAX_SDK_PATH "${STADIUM_AURA_AAX_SDK_PATH}" CACHE PATH "" FORCE)
    list(APPEND STADIUM_AURA_FORMATS AAX)
    message(STATUS "Stadium Aura AAX enabled: ${STADIUM_AURA_AAX_SDK_PATH}")
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    find_package(JUCE CONFIG REQUIRED)
endif()

juce_add_plugin(StadiumAura
    COMPANY_NAME "Stadium Audio"
    BUNDLE_ID "com.stadiumaudio.stadiumaura"
    PLUGIN_MANUFACTURER_CODE StAu
    PLUGIN_CODE SdAu
    FORMATS ${STADIUM_AURA_FORMATS}
    PRODUCT_NAME "Stadium Aura"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Stadium Aura needs microphone access to process your live vocal or instrument input."
    HARDENED_RUNTIME_ENABLED TRUE
    HARDENED_RUNTIME_OPTIONS "com.apple.security.device.audio-input")

juce_generate_juce_header(StadiumAura)

target_sources(StadiumAura PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/dsp/AuraProcessor.cpp
    Source/dsp/AuraBigEngine.cpp
    Source/dsp/AuraCompressorEngine.cpp
    Source/dsp/EQBand.cpp
    Source/dsp/EQProcessor.cpp
    Source/dsp/MicCharacterProcessor.cpp
    Source/dsp/CompressorSection.cpp
    Source/dsp/SafetyLimiter.cpp
    Source/ui/EQPanel.cpp
    Source/ui/ExpandedEQPanel.cpp
    Source/ui/PremiumLookAndFeel.cpp
    Source/ui/PremiumKnob.cpp
    Source/ui/PremiumFader.cpp
    Source/ui/RackMeter.cpp
    Source/ui/RackComponents.cpp)

target_compile_definitions(StadiumAura PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_MICROPHONE_PERMISSION_ENABLED=1)

target_link_libraries(StadiumAura PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

include(CTest)
if (BUILD_TESTING)
    juce_add_console_app(StadiumAuraTests PRODUCT_NAME "Stadium Aura DSP Tests")
    target_sources(StadiumAuraTests PRIVATE
        Tests/DspTests.cpp
        Source/dsp/AuraProcessor.cpp
        Source/dsp/AuraCompressorEngine.cpp
        Source/dsp/EQBand.cpp
        Source/dsp/EQProcessor.cpp
        Source/dsp/MicCharacterProcessor.cpp
        Source/dsp/CompressorSection.cpp
        Source/dsp/SafetyLimiter.cpp)
    target_include_directories(StadiumAuraTests PRIVATE Source)
    target_link_libraries(StadiumAuraTests PRIVATE
        juce::juce_audio_basics
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
    juce_generate_juce_header(StadiumAuraTests)
    add_test(NAME StadiumAuraDSP COMMAND StadiumAuraTests)
endif()
