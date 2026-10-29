#pragma once

#include "../PluginProcessor.h"
#include "HeroPanel.h"
#include "PresetRail.h"
#include "StatusStrip.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace synthset::ui
{
class EditorContent final : public juce::Component,
                            private juce::Timer
{
public:
    explicit EditorContent (SynthSetAudioProcessor& processor);
    ~EditorContent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;

    void setFrozen (bool frozen);

private:
    void timerCallback() override;
    void showPreset (int bankIndex);
    void rebuildBackdrop();
    void paintDetailStrip (juce::Graphics&);
    const Preset& currentPreset() const;

    SynthSetAudioProcessor& processor;
    PresetRail rail;
    HeroPanel hero;
    StatusStrip status;
    juce::Image backdrop;
    int displayedIndex = 0;
    std::uint32_t lastMidiCounter = 0;
    double midiFlashUntil = 0.0;
    bool frozen = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EditorContent)
};
} // namespace synthset::ui
