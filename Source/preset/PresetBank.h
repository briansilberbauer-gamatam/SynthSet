#pragma once

#include "Preset.h"

#include <array>

namespace synthset
{
inline constexpr const char* defaultPresetId = "classic.keys.five-voice-keys";

const std::array<Preset, kNumPresets>& getFactoryBank() noexcept;
int getNumFactoryPresets() noexcept;
int getDefaultPresetIndex() noexcept;

const Preset* findPresetById (const char* id) noexcept;
int indexOfPresetId (const char* id) noexcept;

} // namespace synthset
