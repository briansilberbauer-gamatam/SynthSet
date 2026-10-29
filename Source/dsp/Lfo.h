#pragma once

#include "../preset/SynthPatch.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace synthset
{
class Lfo
{
public:
    void prepare (double sampleRate) noexcept;
    void setParams (const LfoParams& params) noexcept;
    void reset (std::uint32_t seed, float startPhase = 0.0f) noexcept;

    float advance (int numSamples, float rateHz) noexcept;
    float getLastValue() const noexcept { return lastValue; }

private:
    float shapeValue() noexcept;
    static std::uint32_t splitMix (std::uint32_t x) noexcept;

    double sampleRate = 48000.0;
    LfoParams params {};
    double phase = 0.0;
    double fadeProgress = 0.0;
    double fadeIncrement = 0.0;
    float lastValue = 0.0f;
    float heldValue = 0.0f;
    std::uint32_t rngState = 0x53594e54u;
};

inline float lfoRateHz (const LfoParams& params, double bpm) noexcept
{
    if (! params.tempoSync)
        return std::max (0.001f, params.rateHz);

    const double beatsPerSecond = std::max (20.0, bpm) / 60.0;
    return static_cast<float> (beatsPerSecond / std::max (0.01f, params.syncBeats));
}
} // namespace synthset
