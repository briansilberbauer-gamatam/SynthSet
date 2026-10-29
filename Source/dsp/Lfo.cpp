#include "Lfo.h"

namespace synthset
{
namespace
{
constexpr float twoPi = 6.28318530717958647692f;
}

void Lfo::prepare (double newSampleRate) noexcept
{
    sampleRate = std::max (8000.0, newSampleRate);
}

void Lfo::setParams (const LfoParams& newParams) noexcept
{
    params = newParams;
    params.fadeInMs = std::max (0.0f, params.fadeInMs);
    fadeIncrement = params.fadeInMs <= 0.01f
                        ? 1.0
                        : 1.0 / (static_cast<double> (params.fadeInMs) * 0.001 * sampleRate);
}

std::uint32_t Lfo::splitMix (std::uint32_t x) noexcept
{
    x += 0x9e3779b9u;
    x = (x ^ (x >> 16)) * 0x85ebca6bu;
    x = (x ^ (x >> 13)) * 0xc2b2ae35u;
    return x ^ (x >> 16);
}

void Lfo::reset (std::uint32_t seed, float startPhase) noexcept
{
    phase = static_cast<double> (startPhase + params.phase);
    phase -= std::floor (phase);
    fadeProgress = 0.0;
    rngState = splitMix (seed ^ 0x9e3779b9u);
    heldValue = static_cast<float> (splitMix (rngState = splitMix (rngState)) >> 8) * (1.0f / 8388608.0f) - 1.0f;
    lastValue = shapeValue();

    if (params.fadeInMs <= 0.01f)
        fadeProgress = 1.0;
}

float Lfo::shapeValue() noexcept
{
    const float t = static_cast<float> (phase - std::floor (phase));

    switch (params.shape)
    {
        case LfoShape::Sine:
            return std::sin (twoPi * t);
        case LfoShape::Triangle:
            return 1.0f - 4.0f * std::abs (t - 0.5f);
        case LfoShape::Saw:
            return 2.0f * t - 1.0f;
        case LfoShape::Square:
            return t < 0.5f ? 1.0f : -1.0f;
        case LfoShape::SampleHold:
            return heldValue;
        default:
            return 0.0f;
    }
}

float Lfo::advance (int numSamples, float rateHz) noexcept
{
    const double increment = static_cast<double> (std::max (0.001f, rateHz))
                             * static_cast<double> (numSamples) / sampleRate;

    const double oldPhase = phase;
    phase += increment;
    const bool wrapped = phase >= 1.0;

    if (wrapped)
        phase -= std::floor (phase);

    if (params.shape == LfoShape::SampleHold && (wrapped || oldPhase < 1.0e-12))
    {
        rngState = splitMix (rngState);
        heldValue = static_cast<float> (splitMix (rngState) >> 8) * (1.0f / 8388608.0f) - 1.0f;
    }

    if (fadeProgress < 1.0)
    {
        fadeProgress = std::min (1.0, fadeProgress + fadeIncrement * numSamples);
    }

    lastValue = shapeValue() * static_cast<float> (fadeProgress);
    return lastValue;
}
} // namespace synthset
