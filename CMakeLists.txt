cmake_minimum_required(VERSION 3.22)

# Must be set before project()
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal 64-bit binary")

project(BLG VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE 8.x as a git submodule:
#   git submodule add https://github.com/juce-framework/JUCE.git JUCE
#   git -C JUCE checkout 8.0.8      (any 8.0.x tag)
add_subdirectory(JUCE)

if(APPLE)
    set(BLG_FORMATS VST3 AU)
    set(BLG_COPY_DEFAULT ON)
else()
    set(BLG_FORMATS VST3)
    set(BLG_COPY_DEFAULT OFF)   # copying into Program Files needs admin rights
endif()

option(BLG_COPY_AFTER_BUILD "Copy the plugin to the system plug-in folder after build" ${BLG_COPY_DEFAULT})

juce_add_plugin(blg
    COMPANY_NAME            "blackghanistan"
    PLUGIN_MANUFACTURER_CODE Bkgh
    PLUGIN_CODE             Blg1
    BUNDLE_ID               com.blackghanistan.blg
    PRODUCT_NAME            "blg"
    FORMATS                 ${BLG_FORMATS}
    VST3_CATEGORIES         Fx Reverb
    AU_MAIN_TYPE            kAudioUnitType_Effect
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    COPY_PLUGIN_AFTER_BUILD ${BLG_COPY_AFTER_BUILD})

juce_generate_juce_header(blg)

target_sources(blg PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/PresetManager.cpp)

target_compile_definitions(blg PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(blg
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
