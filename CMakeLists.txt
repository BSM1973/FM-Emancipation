cmake_minimum_required(VERSION 3.24)
project(FMEmancipation VERSION 0.3.1 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.10 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)
juce_add_plugin(FMEmancipation
 COMPANY_NAME "Liberty Music Software"
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
target_compile_definitions(FMEmancipation PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(FMEmancipation PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
juce_generate_juce_header(FMEmancipation)

if(MSVC)
  target_compile_options(FMEmancipation PRIVATE /utf-8)
endif()

# Mandatory Unicode runtime smoke test; CI runs it before plugin packaging.
enable_testing()
juce_add_console_app(FMEmancipationTextTest PRODUCT_NAME "Liberty Unicode Test")
target_sources(FMEmancipationTextTest PRIVATE Source/UnicodeTest.cpp)
target_link_libraries(FMEmancipationTextTest PRIVATE juce::juce_core)
target_compile_definitions(FMEmancipationTextTest PRIVATE JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0)
if(MSVC)
  target_compile_options(FMEmancipationTextTest PRIVATE /utf-8)
endif()
add_test(NAME LibertyUnicode COMMAND $<TARGET_FILE:FMEmancipationTextTest>)
