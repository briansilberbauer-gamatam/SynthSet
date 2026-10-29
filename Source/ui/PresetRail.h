#pragma once

#include "../preset/Preset.h"
#include "../preset/PresetBank.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>
#include <vector>

namespace synthset::ui
{
class PresetRail final : public juce::Component
{
public:
    enum class Filter { All = 0, Classic, Modern };

    PresetRail();

    void setCurrentPreset (int bankIndex, bool reveal = true);
    int getCurrentPreset() const noexcept { return currentIndex; }

    void setFilter (Filter newFilter);
    Filter getFilter() const noexcept { return filter; }

    void step (int direction);
    void stepWithinGroup (int direction);
    void jumpToStart();
    void jumpToEnd();

    std::function<void (int)> onPresetSelected;

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    struct Layout
    {
        juce::Rectangle<int> filterTrack;
        std::array<juce::Rectangle<int>, 3> filterSegments;
        juce::Rectangle<int> groupsArea;
        juce::Rectangle<int> presetsArea;
        juce::Rectangle<int> footerArea;
    };

    Layout layout() const;
    int groupAt (juce::Point<int>) const;
    int presetRowAt (juce::Point<int>) const;
    int filterSegmentAt (juce::Point<int>) const;
    int groupRowHeight() const { return 34; }
    int presetRowHeight() const { return 34; }
    int firstGroupY() const { return 146; }
    int firstPresetY() const { return 450; }

    std::vector<int> visiblePresetsForGroup() const;
    std::vector<int> filteredBankIndices() const;
    bool matchesFilter (const Preset&) const;
    void selectIndex (int index, bool notify);
    void chooseFirstVisible();
    juce::Colour accent() const;
    const Preset& currentPreset() const;

    int currentIndex = 0;
    Group selectedGroup = Group::Keys;
    Filter filter = Filter::All;
    int hoveredGroup = -1;
    int hoveredPreset = -1;
    int hoveredFilter = -1;
    int pressedGroup = -1;
    int pressedPreset = -1;
    int pressedFilter = -1;
};
} // namespace synthset::ui
