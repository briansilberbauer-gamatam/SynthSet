cmake_minimum_required(VERSION 3.22)

project(SynthSet VERSION 1.0.3 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_SKIP_RPATH TRUE)

if(MSVC)
    add_compile_options(/bigobj /utf-8)
endif()

option(SYNTHSET_BUILD_TESTS "Build offline DSP/preset tests" ON)
option(SYNTHSET_BUILD_SCREENSHOT "Build headless UI renderer" ON)
option(SYNTHSET_BUILD_HOST_TEST "Build VST3 host smoke test" ON)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
    add_subdirectory(external/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.8
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_binary_data(SynthSetAssets
    SOURCES
        Resources/LatoRegular.ttf
        Resources/LatoSemibold.ttf
        Resources/LatoBlack.ttf
        Resources/Lato-OFL.txt)

set(SYNTHSET_SOURCES
    Source/Version.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/preset/SynthPatch.h
    Source/preset/Preset.h
    Source/preset/PresetBank.cpp
    Source/preset/PresetBank.h
    Source/preset/PresetManager.cpp
    Source/preset/PresetManager.h
    Source/dsp/Envelope.cpp
    Source/dsp/Envelope.h
    Source/dsp/Filters.cpp
    Source/dsp/Filters.h
    Source/dsp/Effects.cpp
    Source/dsp/Effects.h
    Source/dsp/Lfo.cpp
    Source/dsp/Lfo.h
    Source/dsp/ModMatrix.h
    Source/dsp/Oscillator.h
    Source/dsp/SynthEngine.cpp
    Source/dsp/SynthEngine.h
    Source/dsp/Voice.cpp
    Source/dsp/Voice.h
    Source/dsp/Wavetable.cpp
    Source/dsp/Wavetable.h
    Source/ui/ArtworkStage.cpp
    Source/ui/ArtworkStage.h
    Source/ui/EditorContent.cpp
    Source/ui/EditorContent.h
    Source/ui/HeroPanel.cpp
    Source/ui/HeroPanel.h
    Source/ui/PresetRail.cpp
    Source/ui/PresetRail.h
    Source/ui/StatusStrip.cpp
    Source/ui/StatusStrip.h
    Source/ui/Theme.cpp
    Source/ui/Theme.h)

set(SYNTHSET_DEFINITIONS
    SYNTHSET_VERSION="${PROJECT_VERSION}"
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_MODAL_LOOPS_PERMITTED=1
    JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS=0)

juce_add_plugin(SynthSet
    COMPANY_NAME "BLS Audio"
    BUNDLE_ID "com.blsaudio.synthset"
    PLUGIN_MANUFACTURER_CODE Blsa
    PLUGIN_CODE Sset
    FORMATS VST3 Standalone
    PRODUCT_NAME "SynthSet"
    DESCRIPTION "Preset-only 16-voice polyphonic synthesizer"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES "Instrument|Synth")

target_sources(SynthSet PRIVATE ${SYNTHSET_SOURCES})
target_include_directories(SynthSet PRIVATE Source)
target_compile_definitions(SynthSet PUBLIC ${SYNTHSET_DEFINITIONS})
target_link_libraries(SynthSet
    PRIVATE
        SynthSetAssets
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

if(SYNTHSET_BUILD_TESTS)
    enable_testing()

    juce_add_console_app(SynthSetTests PRODUCT_NAME "SynthSetTests")
    target_sources(SynthSetTests PRIVATE Tests/DspTests.cpp ${SYNTHSET_SOURCES})
    target_include_directories(SynthSetTests PRIVATE Source)
    target_compile_definitions(SynthSetTests PRIVATE ${SYNTHSET_DEFINITIONS})
    target_link_libraries(SynthSetTests
        PRIVATE
            SynthSetAssets
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_gui_extra
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

    add_test(NAME synthset_dsp COMMAND $<TARGET_FILE:SynthSetTests>)
endif()

if(SYNTHSET_BUILD_HOST_TEST)
    juce_add_console_app(SynthSetHostSmokeTest PRODUCT_NAME "SynthSetHostSmokeTest")

    target_sources(SynthSetHostSmokeTest PRIVATE Tools/HostSmokeTest.cpp)
    target_compile_definitions(SynthSetHostSmokeTest PRIVATE
        ${SYNTHSET_DEFINITIONS}
        JUCE_PLUGINHOST_VST3=1)
    target_link_libraries(SynthSetHostSmokeTest
        PRIVATE
            juce::juce_audio_utils
            juce::juce_gui_extra
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

    add_dependencies(SynthSetHostSmokeTest SynthSet_VST3)
endif()

if(SYNTHSET_BUILD_SCREENSHOT)
    juce_add_gui_app(SynthSetScreenshot
        PRODUCT_NAME "SynthSetScreenshot"
        COMPANY_NAME "BLS Audio"
        BUNDLE_ID "com.blsaudio.synthset.screenshot")

    target_sources(SynthSetScreenshot PRIVATE ${SYNTHSET_SOURCES} Tools/RenderEditor.cpp)
    target_include_directories(SynthSetScreenshot PRIVATE Source)
    target_compile_definitions(SynthSetScreenshot PRIVATE ${SYNTHSET_DEFINITIONS})
    target_link_libraries(SynthSetScreenshot
        PRIVATE
            SynthSetAssets
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_gui_extra
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)
endif()
