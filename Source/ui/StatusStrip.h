#pragma once

#include "../preset/Preset.h"
#include "../preset/PresetBank.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace synthset::ui
{
class StatusStrip final : public juce::Component
{
public:
    StatusStrip() = default;

    void setPreset (const Preset& preset);
    void setLive (int voiceCount, bool midiActive);

    void paint (juce::Graphics&) override;

private:
    const Preset* current = nullptr;
    int voices = 0;
    bool midi = false;
};
} // namespace synthset::ui
