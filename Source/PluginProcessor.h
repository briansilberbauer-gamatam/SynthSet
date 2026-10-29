#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "dsp/SynthEngine.h"
#include "preset/PresetManager.h"

class SynthSetAudioProcessor final : public juce::AudioProcessor,
                                     private juce::Timer
{
public:
    SynthSetAudioProcessor();
    ~SynthSetAudioProcessor() override;

    using juce::AudioProcessor::processBlock;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SynthSet"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    synthset::PresetManager& getPresetManager() noexcept { return presetManager; }
    const synthset::PresetManager& getPresetManager() const noexcept { return presetManager; }

    int getVoiceCount() const noexcept { return engine.getVoiceCount(); }
    float getPeakLeft() const noexcept { return engine.getPeakLeft(); }
    float getPeakRight() const noexcept { return engine.getPeakRight(); }
    std::uint32_t getMidiCounter() const noexcept { return engine.getMidiCounter(); }

private:
    void timerCallback() override;

    synthset::PresetManager presetManager;
    synthset::SynthEngine engine;
    double hostBpm = 120.0;

    int restoreGuardProgram = -1;
    double restoreGuardUntil = 0.0;
    int hostDisplayedProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthSetAudioProcessor)
};
