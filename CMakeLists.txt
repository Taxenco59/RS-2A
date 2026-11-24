cmake_minimum_required(VERSION 3.22)

# Runtime de MSVC enlazado estáticamente: el plugin no necesita instalar Visual C++ Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(RS2A VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(RS2A
    COMPANY_NAME            "RS Audio"
    PLUGIN_MANUFACTURER_CODE RsAu
    PLUGIN_CODE             Rs2a
    FORMATS                 VST3 Standalone
    PRODUCT_NAME            "RS-2A"
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES         Fx Dynamics
    BUNDLE_ID               com.rsaudio.rs2a)

target_sources(RS2A PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/GuiComponents.h
    Source/OptoCompressorCore.h)

target_compile_definitions(RS2A PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

target_link_libraries(RS2A
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
