#pragma once

#include "../preset/Preset.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace synthset::ui
{
class ArtworkStage final : public juce::Component,
                           private juce::Timer
{
public:
    ArtworkStage();
    ~ArtworkStage() override = default;

    void setSpec (const ArtworkSpec& newSpec, bool animate = true);
    void setFrozen (bool shouldFreeze);

    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;

private:
    void timerCallback() override;
    void rebuildImages();
    juce::Image renderArtwork (const ArtworkSpec& spec, int width, int height) const;

    ArtworkSpec spec {};
    juce::Image currentImage;
    juce::Image outgoingImage;
    float fade = 1.0f;
    float time = 0.5f;
    bool frozen = false;
    bool haveSpec = false;
};
} // namespace synthset::ui
