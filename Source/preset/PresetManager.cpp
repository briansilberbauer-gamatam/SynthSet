#include "PresetManager.h"

#include <cstring>

namespace synthset
{
PresetManager::PresetManager() noexcept
{
    const int defaultIndex = getDefaultPresetIndex();
    pending.store (defaultIndex, std::memory_order_relaxed);
    applied.store (defaultIndex, std::memory_order_relaxed);
}

int PresetManager::getPendingProgram() const noexcept
{
    return pending.load (std::memory_order_acquire);
}

int PresetManager::getAppliedProgram() const noexcept
{
    return applied.load (std::memory_order_acquire);
}

void PresetManager::requestProgram (int index) noexcept
{
    if (index >= 0 && index < kNumPresets)
        pending.store (index, std::memory_order_release);
}

void PresetManager::requestNext() noexcept
{
    requestProgram ((getPendingProgram() + 1) % kNumPresets);
}

void PresetManager::requestPrevious() noexcept
{
    requestProgram ((getPendingProgram() + kNumPresets - 1) % kNumPresets);
}

void PresetManager::markApplied (int index) noexcept
{
    if (index >= 0 && index < kNumPresets)
        applied.store (index, std::memory_order_release);
}

const Preset& PresetManager::getAppliedPreset() const noexcept
{
    return getFactoryBank()[static_cast<size_t> (getAppliedProgram())];
}

const Preset& PresetManager::getPendingPreset() const noexcept
{
    return getFactoryBank()[static_cast<size_t> (getPendingProgram())];
}

void PresetManager::getStateInformation (juce::MemoryBlock& destination) const
{
    juce::XmlElement state ("SYNTHSET");
    state.setAttribute ("schema", schemaVersion);
    state.setAttribute ("presetId", getPendingPreset().id);

    const auto text = state.toString (juce::XmlElement::TextFormat().singleLine());
    destination.replaceAll (text.toRawUTF8(), static_cast<size_t> (text.getNumBytesAsUTF8()));
}

void PresetManager::setStateInformation (const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0)
    {
        requestProgram (getDefaultPresetIndex());
        return;
    }

    const auto text = juce::String::fromUTF8 (static_cast<const char*> (data), sizeInBytes);
    auto xml = juce::XmlDocument::parse (text);

    if (xml == nullptr || ! xml->hasTagName ("SYNTHSET"))
    {
        requestProgram (getDefaultPresetIndex());
        return;
    }

    const int schema = xml->getIntAttribute ("schema", 0);

    if (schema > schemaVersion)
    {
        requestProgram (getDefaultPresetIndex());
        return;
    }

    const auto presetId = xml->getStringAttribute ("presetId");
    const int index = indexOfPresetId (presetId.toRawUTF8());
    requestProgram (index >= 0 ? index : getDefaultPresetIndex());
}
} // namespace synthset
