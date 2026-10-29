#pragma once

#include "ArtworkStage.h"
#include "../preset/Preset.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>

namespace synthset::ui
{
class HeroPanel final : public juce::Component
{
public:
    HeroPanel();
    ~HeroPanel() override;

    void setPreset (const Preset& preset, bool animate = true);
    void setNavigationCallbacks (std::function<void()> previous,
                                 std::function<void()> next);
    void setFrozen (bool frozen);

    void resized() override;

private:
    class Overlay;
    ArtworkStage artwork;
    std::unique_ptr<Overlay> overlay;
    const Preset* currentPreset = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeroPanel)
};
} // namespace synthset::ui
