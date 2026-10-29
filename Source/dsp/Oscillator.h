#pragma once

#include "../preset/SynthPatch.h"
#include "Wavetable.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace synthset
{
struct OscillatorState
{
    double phase = 0.0;
    float triangleState = 0.0f;
};

inline double wrapPhase (double phase) noexcept
{
    phase -= std::floor (phase);
    return phase;
}

inline float polyBlep (double t, double dt) noexcept
{
    if (dt <= 0.0)
        return 0.0f;

    if (t < dt)
    {
        t /= dt;
        return static_cast<float> (t + t - t * t - 1.0);
    }

    if (t > 1.0 - dt)
    {
        t = (t - 1.0) / dt;
        return static_cast<float> (t * t + t + t + 1.0);
    }

    return 0.0f;
}

inline float sawWave (double phase, double increment) noexcept
{
    return static_cast<float> (2.0 * phase - 1.0) - polyBlep (phase, increment);
}

inline float pulseWave (double phase, double increment, float width) noexcept
{
    const double p = std::clamp (static_cast<double> (width), 0.04, 0.96);
    float value = phase < p ? 1.0f : -1.0f;
    value += polyBlep (phase, increment);

    double t2 = phase - p;
    if (t2 < 0.0)
        t2 += 1.0;

    value -= polyBlep (t2, increment);
    return value;
}

inline float triangleWave (double phase, double increment, float& state) noexcept
{
    const float square = pulseWave (phase, increment, 0.5f);
    state = state * 0.9992f + square * static_cast<float> (4.0 * increment);
    state = std::clamp (state, -1.4f, 1.4f);
    return std::clamp (state * 0.78f, -1.0f, 1.0f);
}

inline float sineWave (double phase) noexcept
{
    return std::sin (static_cast<float> (phase) * 6.28318530717958647692f);
}

/**
    Render a morphing analog-style oscillator at the supplied phase. The caller
    advances phases so hard sync can be handled with sample-accurate master
    wrap detection.
*/
inline float renderAnalogShape (double phase, double increment, float shape,
                                float pulseWidth, float& triangleState) noexcept
{
    shape = std::clamp (shape, 0.0f, 1.0f);

    if (shape <= 0.3333333f)
    {
        const float mix = shape / 0.3333333f;
        return sawWave (phase, increment) * (1.0f - mix)
               + pulseWave (phase, increment, pulseWidth) * mix;
    }

    if (shape <= 0.6666667f)
    {
        const float mix = (shape - 0.3333333f) / 0.3333334f;
        return pulseWave (phase, increment, pulseWidth) * (1.0f - mix)
               + triangleWave (phase, increment, triangleState) * mix;
    }

    const float mix = (shape - 0.6666667f) / 0.3333333f;
    return triangleWave (phase, increment, triangleState) * (1.0f - mix)
           + sineWave (phase) * mix;
}

inline float renderOscillator (OscillatorState& state, const OscParams& osc,
                               double increment, float pulseWidth,
                               float wavetablePos, const WavetableBank& wavetable,
                               double sampleRate, float frequency) noexcept
{
    if (osc.wavetable)
        return wavetable.sample (wavetablePos, static_cast<float> (state.phase),
                                 frequency, sampleRate);

    return renderAnalogShape (state.phase, increment, osc.shape, pulseWidth,
                              state.triangleState);
}

inline float deterministicNoise (std::uint32_t& state) noexcept
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return static_cast<float> (state) * (2.0f / 4294967295.0f) - 1.0f;
}
} // namespace synthset
