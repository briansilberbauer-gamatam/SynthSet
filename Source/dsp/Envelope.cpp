#include "Envelope.h"

namespace synthset
{
namespace
{
float samplesForMs (float milliseconds, double sampleRate) noexcept
{
    return std::max (1.0f, milliseconds * 0.001f * static_cast<float> (sampleRate));
}
} // namespace

void Adsr::prepare (double newSampleRate) noexcept
{
    sampleRate = std::max (8000.0, newSampleRate);
}

void Adsr::setParams (const AdsrParams& newParams) noexcept
{
    params = newParams;
    params.sustain = std::clamp (params.sustain, 0.0f, 1.0f);
    params.curve = std::clamp (params.curve, 0.0f, 1.0f);
    curvePower = 2 + static_cast<int> (std::round (params.curve * 3.0f));
}

void Adsr::noteOn() noexcept
{
    stageStart = std::clamp (level, 0.0f, 1.0f);

    if (stageStart < 1.0e-5f)
        stageStart = 0.0f;

    progress = 0.0f;
    progressInc = 1.0f / samplesForMs (params.attackMs, sampleRate);

    if (params.attackMs <= 0.01f)
    {
        level = 1.0f;
        stage = Stage::Decay;
        stageStart = 1.0f;
        progress = 0.0f;
        progressInc = 1.0f / samplesForMs (params.decayMs, sampleRate);
    }
    else
    {
        stage = Stage::Attack;
    }
}

void Adsr::noteOff() noexcept
{
    if (stage == Stage::Idle || stage == Stage::Release)
        return;

    stageStart = std::clamp (level, 0.0f, 1.0f);
    progress = 0.0f;
    progressInc = 1.0f / samplesForMs (params.releaseMs, sampleRate);
    stage = Stage::Release;
}

void Adsr::reset() noexcept
{
    stage = Stage::Idle;
    level = 0.0f;
    stageStart = 0.0f;
    progress = 0.0f;
    progressInc = 0.0f;
}

float Adsr::curveIn (float p, int power) noexcept
{
    p = std::clamp (p, 0.0f, 1.0f);
    const float remaining = 1.0f - p;
    float result = 1.0f;

    for (int i = 0; i < power; ++i)
        result *= remaining;

    return 1.0f - result;
}

float Adsr::curveOut (float p, int power) noexcept
{
    p = std::clamp (p, 0.0f, 1.0f);
    const float remaining = 1.0f - p;
    float result = 1.0f;

    for (int i = 0; i < power; ++i)
        result *= remaining;

    return result;
}

float Adsr::process() noexcept
{
    switch (stage)
    {
        case Stage::Idle:
            level = 0.0f;
            break;

        case Stage::Attack:
        {
            if (progress >= 1.0f)
            {
                level = 1.0f;
                stage = Stage::Decay;
                stageStart = 1.0f;
                progress = 0.0f;
                progressInc = 1.0f / samplesForMs (params.decayMs, sampleRate);
            }
            else
            {
                level = stageStart + (1.0f - stageStart) * curveIn (progress, curvePower);
                progress += progressInc;
            }
            break;
        }

        case Stage::Decay:
        {
            if (progress >= 1.0f)
            {
                level = params.sustain;
                stage = Stage::Sustain;
            }
            else
            {
                level = params.sustain + (stageStart - params.sustain)
                                             * curveOut (progress, curvePower);
                progress += progressInc;
            }
            break;
        }

        case Stage::Sustain:
            level = params.sustain;
            break;

        case Stage::Release:
        {
            if (progress >= 1.0f)
            {
                level = 0.0f;
                stage = Stage::Idle;
            }
            else
            {
                level = stageStart * curveOut (progress, curvePower);
                progress += progressInc;
            }
            break;
        }
    }

    if (! std::isfinite (level))
    {
        level = 0.0f;
        stage = Stage::Idle;
    }

    return std::clamp (level, 0.0f, 1.0f);
}
} // namespace synthset
