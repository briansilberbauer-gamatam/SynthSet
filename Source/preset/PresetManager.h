#pragma once

#include "PresetBank.h"

#include <juce_core/juce_core.h>

#include <atomic>

namespace synthset
{
/**
    Owns the atomic request/applied program indices. The audio thread reads the
    request and writes the applied index; the message thread owns host state IO.
*/
class PresetManager
{
public:
    PresetManager() noexcept;

    int getPendingProgram() const noexcept;
    int getAppliedProgram() const noexcept;

    void requestProgram (int index) noexcept;
    void requestNext() noexcept;
    void requestPrevious() noexcept;
    void markApplied (int index) noexcept;

    const Preset& getAppliedPreset() const noexcept;
    const Preset& getPendingPreset() const noexcept;

    void getStateInformation (juce::MemoryBlock& destination) const;
    void setStateInformation (const void* data, int sizeInBytes);

    static constexpr int schemaVersion = 1;

private:
    std::atomic<int> pending { 0 };
    std::atomic<int> applied { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
} // namespace synthset
