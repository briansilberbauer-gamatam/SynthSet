#include "PresetRail.h"
#include "Theme.h"
#include "../Version.h"

#include <algorithm>
#include <cmath>

namespace synthset::ui
{
PresetRail::PresetRail()
{
    currentIndex = std::max (0, getDefaultPresetIndex());
    selectedGroup = getFactoryBank()[static_cast<size_t> (currentIndex)].group;
    setMouseCursor (juce::MouseCursor::NormalCursor);
}

const Preset& PresetRail::currentPreset() const
{
    return getFactoryBank()[static_cast<size_t> (juce::jlimit (0, kNumPresets - 1, currentIndex))];
}

juce::Colour PresetRail::accent() const
{
    return colourFrom (currentPreset().artwork.accent);
}

bool PresetRail::matchesFilter (const Preset& preset) const
{
    return filter == Filter::All
           || (filter == Filter::Classic && preset.collection == Collection::Classic)
           || (filter == Filter::Modern && preset.collection == Collection::Modern);
}

std::vector<int> PresetRail::visiblePresetsForGroup() const
{
    std::vector<int> indices;
    const auto& bank = getFactoryBank();

    for (int i = 0; i < static_cast<int> (bank.size()); ++i)
        if (bank[static_cast<size_t> (i)].group == selectedGroup && matchesFilter (bank[static_cast<size_t> (i)]))
            indices.push_back (i);

    return indices;
}

std::vector<int> PresetRail::filteredBankIndices() const
{
    std::vector<int> indices;
    const auto& bank = getFactoryBank();

    for (int i = 0; i < static_cast<int> (bank.size()); ++i)
        if (matchesFilter (bank[static_cast<size_t> (i)]))
            indices.push_back (i);

    return indices;
}

void PresetRail::selectIndex (int index, bool notify)
{
    const auto& bank = getFactoryBank();
    currentIndex = juce::jlimit (0, kNumPresets - 1, index);
    selectedGroup = bank[static_cast<size_t> (currentIndex)].group;

    if (notify && onPresetSelected)
        onPresetSelected (currentIndex);

    repaint();
}

void PresetRail::chooseFirstVisible()
{
    const auto visible = visiblePresetsForGroup();

    if (! visible.empty())
        selectIndex (visible.front(), true);
}

void PresetRail::setCurrentPreset (int bankIndex, bool reveal)
{
    const auto& bank = getFactoryBank();
    currentIndex = juce::jlimit (0, kNumPresets - 1, bankIndex);
    selectedGroup = bank[static_cast<size_t> (currentIndex)].group;

    if (reveal && ! matchesFilter (bank[static_cast<size_t> (currentIndex)]))
        filter = Filter::All;

    repaint();
}

void PresetRail::setFilter (Filter newFilter)
{
    if (filter == newFilter)
        return;

    filter = newFilter;

    if (! matchesFilter (currentPreset()))
        chooseFirstVisible();

    repaint();
}

void PresetRail::step (int direction)
{
    if (direction == 0)
        return;

    auto indices = filteredBankIndices();

    if (indices.empty())
        return;

    auto it = std::find (indices.begin(), indices.end(), currentIndex);

    if (it == indices.end())
    {
        selectIndex (direction > 0 ? indices.front() : indices.back(), true);
        return;
    }

    const int position = static_cast<int> (std::distance (indices.begin(), it));
    const int count = static_cast<int> (indices.size());
    const int next = (position + direction + count) % count;
    selectIndex (indices[static_cast<size_t> (next)], true);
}

void PresetRail::stepWithinGroup (int direction)
{
    if (direction == 0)
        return;

    const auto visible = visiblePresetsForGroup();

    if (visible.empty())
        return;

    auto it = std::find (visible.begin(), visible.end(), currentIndex);

    if (it == visible.end())
    {
        selectIndex (direction > 0 ? visible.front() : visible.back(), true);
        return;
    }

    const int position = static_cast<int> (std::distance (visible.begin(), it));
    const int count = static_cast<int> (visible.size());
    const int next = (position + direction + count) % count;
    selectIndex (visible[static_cast<size_t> (next)], true);
}

void PresetRail::jumpToStart()
{
    const auto indices = filteredBankIndices();

    if (! indices.empty())
        selectIndex (indices.front(), true);
}

void PresetRail::jumpToEnd()
{
    const auto indices = filteredBankIndices();

    if (! indices.empty())
        selectIndex (indices.back(), true);
}

PresetRail::Layout PresetRail::layout() const
{
    Layout result;
    result.filterTrack = { 20, 74, 260, 32 };
    result.filterSegments = {
        result.filterTrack.withWidth (result.filterTrack.getWidth() / 3),
        result.filterTrack.withWidth (result.filterTrack.getWidth() / 3)
            .withX (result.filterTrack.getX() + result.filterTrack.getWidth() / 3),
        result.filterTrack.withWidth (result.filterTrack.getWidth() / 3)
            .withX (result.filterTrack.getX() + (result.filterTrack.getWidth() * 2) / 3)
    };
    result.groupsArea = { 18, 136, 264, 272 };
    result.presetsArea = { 18, 440, 264, 204 };
    result.footerArea = { 20, getHeight() - 30, 260, 16 };
    return result;
}

int PresetRail::filterSegmentAt (juce::Point<int> point) const
{
    const auto l = layout();

    for (int i = 0; i < 3; ++i)
        if (l.filterSegments[static_cast<size_t> (i)].contains (point))
            return i;

    return -1;
}

int PresetRail::groupAt (juce::Point<int> point) const
{
    for (int index = 0; index < kNumGroups; ++index)
    {
        const juce::Rectangle<int> row { 18, firstGroupY() + index * groupRowHeight(),
                                         264, groupRowHeight() };

        if (row.contains (point))
            return index;
    }

    return -1;
}

int PresetRail::presetRowAt (juce::Point<int> point) const
{
    const auto visible = visiblePresetsForGroup();

    for (int i = 0; i < static_cast<int> (visible.size()); ++i)
    {
        const juce::Rectangle<int> row { 18, firstPresetY() + i * presetRowHeight(),
                                         264, presetRowHeight() };

        if (row.contains (point))
            return visible[static_cast<size_t> (i)];
    }

    return -1;
}

void PresetRail::mouseMove (const juce::MouseEvent& event)
{
    const int newGroup = groupAt (event.getPosition());
    const int newPreset = presetRowAt (event.getPosition());
    const int newFilter = filterSegmentAt (event.getPosition());

    if (newGroup == hoveredGroup && newPreset == hoveredPreset && newFilter == hoveredFilter)
        return;

    hoveredGroup = newGroup;
    hoveredPreset = newPreset;
    hoveredFilter = newFilter;
    setMouseCursor (newGroup >= 0 || newPreset >= 0 || newFilter >= 0
                        ? juce::MouseCursor::PointingHandCursor
                        : juce::MouseCursor::NormalCursor);
    repaint();
}

void PresetRail::mouseExit (const juce::MouseEvent&)
{
    hoveredGroup = hoveredPreset = hoveredFilter = -1;
    setMouseCursor (juce::MouseCursor::NormalCursor);
    repaint();
}

void PresetRail::mouseDown (const juce::MouseEvent& event)
{
    pressedGroup = groupAt (event.getPosition());
    pressedPreset = presetRowAt (event.getPosition());
    pressedFilter = filterSegmentAt (event.getPosition());
}

void PresetRail::mouseUp (const juce::MouseEvent& event)
{
    const int group = groupAt (event.getPosition());
    const int preset = presetRowAt (event.getPosition());
    const int filterSegment = filterSegmentAt (event.getPosition());

    if (pressedFilter >= 0 && filterSegment == pressedFilter)
    {
        setFilter (static_cast<Filter> (pressedFilter));
    }
    else if (pressedGroup >= 0 && group == pressedGroup)
    {
        selectedGroup = allGroups[static_cast<size_t> (pressedGroup)];

        if (currentPreset().group != selectedGroup || ! matchesFilter (currentPreset()))
            chooseFirstVisible();

        repaint();
    }
    else if (pressedPreset >= 0 && preset == pressedPreset)
    {
        selectIndex (pressedPreset, true);
    }

    pressedGroup = pressedPreset = pressedFilter = -1;
}

void PresetRail::paint (juce::Graphics& g)
{
    const auto full = getLocalBounds().toFloat();
    const auto accentColour = accent();

    drawGlassPanel (g, full.reduced (0.5f), 26.0f, accentColour, 0.95f);
    drawGlassSheen (g, full.reduced (0.5f), 26.0f);
    drawGrain (g, full, 0.25f);

    // Wordmark.
    g.setColour (accentColour);
    g.fillEllipse (22.0f, 30.0f, 7.0f, 7.0f);
    g.setColour (textPrimary);
    g.setFont (font (15.2f, FontWeight::semibold, 0.20f));
    g.drawText ("SYNTHSET", 36, 23, 200, 20, juce::Justification::centredLeft, false);

    g.setColour (textMuted);
    g.setFont (font (8.3f, FontWeight::semibold, 0.17f));
    g.drawText ("PRESET INSTRUMENT", 36, 45, 220, 12,
                juce::Justification::centredLeft, false);

    g.setColour (textMuted.withAlpha (0.62f));
    g.setFont (font (8.0f, FontWeight::semibold, 0.1f));
    g.drawText ("48", 246, 27, 34, 14, juce::Justification::centredRight, false);

    // Collection filter.
    const auto l = layout();
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.fillRoundedRectangle (l.filterTrack.toFloat(), 10.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawRoundedRectangle (l.filterTrack.toFloat().reduced (0.5f), 10.0f, 0.8f);

    static const std::array<const char*, 3> filterNames { "ALL", "CLASSIC", "MODERN" };
    const int activeFilter = static_cast<int> (filter);

    for (int i = 0; i < 3; ++i)
    {
        const auto segment = l.filterSegments[static_cast<size_t> (i)].toFloat().reduced (3.0f);

        if (i == activeFilter)
        {
            g.setColour (accentColour.withAlpha (0.16f));
            g.fillRoundedRectangle (segment, 8.0f);
            g.setColour (accentColour.withAlpha (0.40f));
            g.drawRoundedRectangle (segment.reduced (0.5f), 8.0f, 0.8f);
        }
        else if (i == hoveredFilter)
        {
            g.setColour (juce::Colours::white.withAlpha (0.04f));
            g.fillRoundedRectangle (segment, 8.0f);
        }

        g.setColour (i == activeFilter ? textPrimary : textMuted);
        g.setFont (font (9.1f, FontWeight::semibold, 0.12f));
        g.drawText (filterNames[static_cast<size_t> (i)], segment.toNearestInt(),
                    juce::Justification::centred, false);
    }

    // Groups.
    g.setColour (textMuted);
    g.setFont (font (8.4f, FontWeight::semibold, 0.18f));
    g.drawText ("GROUPS", 24, 117, 160, 12, juce::Justification::centredLeft, false);

    const int selectedGroupIndex = static_cast<int> (selectedGroup);

    for (int index = 0; index < kNumGroups; ++index)
    {
        const juce::Rectangle<float> row (18.0f, static_cast<float> (firstGroupY() + index * groupRowHeight()),
                                          264.0f, static_cast<float> (groupRowHeight()));
        const auto& group = allGroups[static_cast<size_t> (index)];
        const bool selected = index == selectedGroupIndex;
        const bool hovered = index == hoveredGroup;
        int count = 0;

        for (const auto& preset : getFactoryBank())
            if (preset.group == group && matchesFilter (preset))
                ++count;

        if (selected)
        {
            g.setColour (accentColour.withAlpha (0.10f));
            g.fillRoundedRectangle (row.reduced (4.0f, 2.0f), 8.0f);
            g.setColour (accentColour.withAlpha (0.88f));
            g.fillRoundedRectangle (row.getX() + 4.0f, row.getY() + 6.0f, 1.8f,
                                    row.getHeight() - 12.0f, 1.0f);
        }
        else if (hovered)
        {
            g.setColour (juce::Colours::white.withAlpha (0.035f));
            g.fillRoundedRectangle (row.reduced (4.0f, 2.0f), 8.0f);
        }

        g.setColour (selected ? textPrimary : textSecondary.withAlpha (0.78f));
        g.setFont (font (9.8f, FontWeight::semibold, 0.11f));
        g.drawText (groupSlug (group), row.withTrimmedLeft (16.0f).withTrimmedRight (34.0f).toNearestInt(),
                    juce::Justification::centredLeft, false);

        g.setColour (selected ? accentColour.withAlpha (0.9f) : textMuted.withAlpha (0.55f));
        g.setFont (font (8.8f, FontWeight::semibold, 0.02f));
        g.drawText (juce::String (count), row.withTrimmedRight (12.0f).toNearestInt(),
                    juce::Justification::centredRight, false);
    }

    // Presets.
    g.setColour (textMuted);
    g.setFont (font (8.4f, FontWeight::semibold, 0.18f));
    g.drawText ("PRESETS", 24, 421, 160, 12, juce::Justification::centredLeft, false);

    const auto visible = visiblePresetsForGroup();
    const auto& bank = getFactoryBank();

    for (int row = 0; row < static_cast<int> (visible.size()); ++row)
    {
        const int bankIndex = visible[static_cast<size_t> (row)];
        const auto& preset = bank[static_cast<size_t> (bankIndex)];
        const bool selected = bankIndex == currentIndex;
        const bool hovered = bankIndex == hoveredPreset;
        const juce::Rectangle<float> rowBounds (
            18.0f, static_cast<float> (firstPresetY() + row * presetRowHeight()),
            264.0f, static_cast<float> (presetRowHeight()));

        if (selected)
        {
            g.setColour (accentColour.withAlpha (0.13f));
            g.fillRoundedRectangle (rowBounds.reduced (4.0f, 2.0f), 9.0f);
            g.setColour (accentColour.withAlpha (0.88f));
            g.fillRoundedRectangle (rowBounds.getX() + 4.0f, rowBounds.getY() + 6.0f, 2.0f,
                                    rowBounds.getHeight() - 12.0f, 1.0f);
        }
        else if (hovered)
        {
            g.setColour (juce::Colours::white.withAlpha (0.04f));
            g.fillRoundedRectangle (rowBounds.reduced (4.0f, 2.0f), 9.0f);
        }

        g.setColour (selected ? textPrimary : textSecondary.withAlpha (0.82f));
        g.setFont (font (12.2f, selected ? FontWeight::semibold : FontWeight::regular, 0.005f));
        g.drawFittedText (preset.name,
                          rowBounds.withTrimmedLeft (16.0f).withTrimmedRight (28.0f).toNearestInt(),
                          juce::Justification::centredLeft, 1, 0.85f);

        g.setColour ((preset.collection == Collection::Classic
                          ? juce::Colour (0xffffc46b) : accentColour)
                         .withAlpha (selected ? 0.95f : 0.55f));
        g.fillEllipse (rowBounds.getRight() - 18.0f, rowBounds.getCentreY() - 2.0f, 4.0f, 4.0f);
    }

    // Footer.
    g.setColour (hairline.withAlpha (0.32f));
    g.fillRect (20.0f, static_cast<float> (getHeight() - 38), 260.0f, 1.0f);
    g.setColour (textMuted.withAlpha (0.72f));
    g.setFont (font (8.0f, FontWeight::regular, 0.08f));
    g.drawText ("48 PRESETS  ·  8 GROUPS", 20, getHeight() - 30, 200, 14,
                juce::Justification::centredLeft, false);
    g.drawText (juce::String ("v") + version, 208, getHeight() - 30, 72, 14,
                juce::Justification::centredRight, false);
}
} // namespace synthset::ui
