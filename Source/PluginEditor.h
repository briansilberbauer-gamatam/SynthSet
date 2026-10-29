#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

#include <memory>

namespace synthset::ui { class EditorContent; }

class SynthSetAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit SynthSetAudioProcessorEditor (SynthSetAudioProcessor&);
    ~SynthSetAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    void setSnapshotMode (bool frozen);

private:
    SynthSetAudioProcessor& processor;
    std::unique_ptr<synthset::ui::EditorContent> content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthSetAudioProcessorEditor)
};
