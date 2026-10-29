#pragma once

#include <array>
#include <cstddef>

namespace synthset
{
/**
    A small, immutable band-limited wavetable set. Tables are rendered once on
    first use (message/prepare thread), then read lock-free by the audio thread.
*/
class WavetableBank
{
public:
    static constexpr int tableSize = 2048;
    static constexpr int numFrames = 4;
    static constexpr int numMips = 8;

    void prepare() noexcept;
    float sample (float position, float phase, float frequency,
                  double sampleRate) const noexcept;
    bool isPrepared() const noexcept { return prepared; }

private:
    using Table = std::array<float, tableSize>;
    using Frame = std::array<Table, numMips>;
    std::array<Frame, numFrames> tables {};
    std::array<int, numMips> harmonicCaps { 512, 256, 128, 64, 32, 16, 8, 4 };
    bool prepared = false;
};

const WavetableBank& getSharedWavetableBank() noexcept;
} // namespace synthset
