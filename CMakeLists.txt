cmake_minimum_required(VERSION 3.22)
project(Mellurator VERSION 0.1.0)

# JUCE 7/8. Clone it next to this folder:  git clone https://github.com/juce-framework/JUCE.git
add_subdirectory(../JUCE JUCE)

juce_add_binary_data(MelluAssets SOURCES Assets/face.jpg)

juce_add_plugin(Mellurator
    COMPANY_NAME            "Mellurator"
    PLUGIN_MANUFACTURER_CODE Mllr
    PLUGIN_CODE              Mlr1
    FORMATS                  VST3 Standalone
    PRODUCT_NAME             "Mellurator"
    IS_SYNTH                 FALSE
    NEEDS_MIDI_INPUT         FALSE
    NEEDS_MIDI_OUTPUT        FALSE
    COPY_PLUGIN_AFTER_BUILD  TRUE)

target_sources(Mellurator PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h)

target_compile_features(Mellurator PRIVATE cxx_std_17)

target_compile_definitions(Mellurator PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Mellurator
    PRIVATE
        MelluAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
