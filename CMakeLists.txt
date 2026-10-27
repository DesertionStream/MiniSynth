cmake_minimum_required(VERSION 3.22)
project(MiniSynth VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: use a local checkout if -DMS_JUCE_PATH=/path/to/JUCE is given, otherwise download it.
set(MS_JUCE_PATH "" CACHE PATH "Path to a local JUCE checkout (optional)")
if(MS_JUCE_PATH)
  add_subdirectory(${MS_JUCE_PATH} JUCE EXCLUDE_FROM_ALL)
else()
  include(FetchContent)
  FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.8 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(MiniSynth
  PLUGIN_MANUFACTURER_CODE Mnsy
  PLUGIN_CODE Msyn
  COMPANY_NAME "MiniSynth"
  PRODUCT_NAME "Mini Synthe"
  FORMATS VST3
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  VST3_CATEGORIES Instrument Synth
  COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(MiniSynth PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(MiniSynth PUBLIC
  JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(MiniSynth PRIVATE
  juce::juce_audio_processors juce::juce_gui_basics
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
