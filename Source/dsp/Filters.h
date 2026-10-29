#pragma once

#include "../preset/SynthPatch.h"

#include <algorithm>
#include <cmath>

namespace synthset
{
struct FilterCoeffs
{
    // Four-pole ZDF ladder
    double ladderA = 0.0;
    double ladderB = 0.0;
    double ladderA4 = 0.0;
    double ladderK = 0.0;

    // Two-pole TPT state-variable filter
    double svfA1 = 0.0;
    double svfA2 = 0.0;
    double svfA3 = 0.0;
    double svfK = 1.0;

    // One-pole high-pass
    double hpfG = 0.0;

    float inputGain = 1.0f;
    float outputGain = 1.0f;
};

FilterCoeffs makeFilterCoeffs (const FilterParams& params, float cutoffHz,
                               float resonance, double sampleRate) noexcept;

struct FilterState
{
    double l1 = 0.0, l2 = 0.0, l3 = 0.0, l4 = 0.0;
    double svfIc1 = 0.0, svfIc2 = 0.0;
    double hpfState = 0.0;

    void reset() noexcept
    {
        l1 = l2 = l3 = l4 = 0.0;
        svfIc1 = svfIc2 = 0.0;
        hpfState = 0.0;
    }

    float process (float input, const FilterCoeffs& c,
                   FilterModel model, FilterType type) noexcept;
};

inline float flushDenormal (double value) noexcept
{
    return std::abs (value) < 1.0e-20 ? 0.0f : static_cast<float> (value);
}
} // namespace synthset
