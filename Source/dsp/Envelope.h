#pragma once

#include "../preset/SynthPatch.h"

#include <algorithm>
#include <cmath>

namespace synthset
{
/**
    Finite-time analog-style ADSR. Each stage follows an exponential-flavoured
    curve and reaches its destination exactly at the end of the stage, which
    keeps note timing deterministic and avoids denormal tails.
*/
class Adsr
{
public:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    void prepare (double sampleRate) noexcept;
    void setParams (const AdsrParams& params) noexcept;

    void noteOn() noexcept;
    void noteOff() noexcept;
    void reset() noexcept;

    float process() noexcept;
    float getLevel() const noexcept { return level; }
    Stage getStage() const noexcept { return stage; }
    bool isActive() const noexcept { return stage != Stage::Idle; }
    bool isReleasing() const noexcept { return stage == Stage::Release; }

private:
    static float curveOut (float progress, int power) noexcept;
    static float curveIn (float progress, int power) noexcept;

    double sampleRate = 48000.0;
    AdsrParams params {};
    Stage stage = Stage::Idle;
    float level = 0.0f;
    float stageStart = 0.0f;
    float progress = 0.0f;
    float progressInc = 0.0f;
    int curvePower = 3;
};

} // namespace synthset
