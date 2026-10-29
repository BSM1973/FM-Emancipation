cmake_minimum_required(VERSION 3.24)
project(FMEmancipation VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.10 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)
juce_add_plugin(FMEmancipation
 COMPANY_NAME "BSM"
 BUNDLE_ID "com.bsm.fmemancipation"
 PLUGIN_MANUFACTURER_CODE BSM1
 PLUGIN_CODE FME1
 FORMATS VST3
 PRODUCT_NAME "FM Emancipation"
 IS_SYNTH TRUE
 NEEDS_MIDI_INPUT TRUE
 NEEDS_MIDI_OUTPUT FALSE
 IS_MIDI_EFFECT FALSE
 EDITOR_WANTS_KEYBOARD_FOCUS FALSE
 COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(FMEmancipation PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(FMEmancipation PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(FMEmancipation PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
juce_generate_juce_header(FMEmancipation)
