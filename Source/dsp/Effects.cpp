#include "Effects.h"

#include <algorithm>
#include <cmath>

namespace synthset
{
namespace
{
constexpr double pi = 3.14159265358979323846;
constexpr double twoPi = 6.28318530717958647692;

float flush (float value) noexcept
{
    return std::abs (value) < 1.0e-20f ? 0.0f : value;
}
} // namespace

//==============================================================================
void Chorus::prepare (double newSampleRate)
{
    sampleRate = std::max (8000.0, newSampleRate);
    reset();
}

void Chorus::reset() noexcept
{
    bufferL.fill (0.0f);
    bufferR.fill (0.0f);
    phase.fill (0.0);
    writePosition = 0;
}

void Chorus::setParams (const ChorusParams& params) noexcept
{
    mode = std::clamp (params.mode, 0, 3);
    depth = std::clamp (params.depth, 0.0f, 1.0f);
    mix = mode == 0 ? 0.0f : std::clamp (params.mix, 0.0f, 1.0f);
}

void Chorus::process (float& left, float& right, float depthModulation) noexcept
{
    if (mode == 0 || mix <= 0.0001f)
        return;

    const float inputL = left;
    const float inputR = right;
    bufferL[static_cast<size_t> (writePosition)] = inputL;
    bufferR[static_cast<size_t> (writePosition)] = inputR;

    const std::array<double, 3> rates = mode == 1
        ? std::array<double, 3> { 0.513, 0.863, 0.337 }
        : mode == 2
        ? std::array<double, 3> { 0.863, 0.513, 1.317 }
        : std::array<double, 3> { 1.713, 1.137, 2.311 };
    const std::array<float, 3> baseMs { 8.2f, 13.7f, 20.3f };
    const float depthMs = (2.8f + depth * 6.4f) * (1.0f + depthModulation * 0.5f);

    float wetL = 0.0f;
    float wetR = 0.0f;

    for (int tap = 0; tap < 3; ++tap)
    {
        phase[static_cast<size_t> (tap)] += rates[static_cast<size_t> (tap)] / sampleRate;
        phase[static_cast<size_t> (tap)] -= std::floor (phase[static_cast<size_t> (tap)]);

        const float lfoL = std::sin (static_cast<float> (twoPi * phase[static_cast<size_t> (tap)]));
        const float lfoR = std::sin (static_cast<float> (twoPi * (phase[static_cast<size_t> (tap)] + 0.31)));

        const float delayL = sampleRate * 0.001f
                             * (baseMs[static_cast<size_t> (tap)] + depthMs * (0.5f + 0.5f * lfoL));
        const float delayR = sampleRate * 0.001f
                             * (baseMs[static_cast<size_t> (tap)] + depthMs * (0.5f + 0.5f * lfoR));

        auto read = [&] (const auto& buffer, float delaySamples)
        {
            float position = static_cast<float> (writePosition) - delaySamples;
            while (position < 0.0f)
                position += static_cast<float> (bufferSize);
            while (position >= static_cast<float> (bufferSize))
                position -= static_cast<float> (bufferSize);

            const int index0 = static_cast<int> (position) & (bufferSize - 1);
            const int index1 = (index0 + 1) & (bufferSize - 1);
            const float fraction = position - std::floor (position);
            return buffer[static_cast<size_t> (index0)]
                   + (buffer[static_cast<size_t> (index1)] - buffer[static_cast<size_t> (index0)]) * fraction;
        };

        wetL += read (bufferL, delayL);
        wetR += read (bufferR, delayR);
    }

    wetL *= 0.333333f;
    wetR *= 0.333333f;
    const float dry = 1.0f - mix * 0.55f;

    writePosition = (writePosition + 1) & (bufferSize - 1);
    left = flush (inputL * dry + wetL * mix);
    right = flush (inputR * dry + wetR * mix);
}

//==============================================================================
void Phaser::prepare (double newSampleRate) noexcept
{
    sampleRate = std::max (8000.0, newSampleRate);
    reset();
}

void Phaser::reset() noexcept
{
    for (auto& channel : state)
        channel.fill (0.0f);

    feedbackState.fill (0.0f);
    phase = 0.0;
}

void Phaser::setParams (const PhaserParams& params) noexcept
{
    stages = params.stages == 4 ? 4 : (params.stages >= 6 ? 6 : 0);
    depth = std::clamp (params.depth, 0.0f, 1.0f);
    feedback = std::clamp (params.feedback, 0.0f, 0.9f);
    mix = stages == 0 ? 0.0f : std::clamp (params.mix, 0.0f, 1.0f);
    rate = std::clamp (params.rateHz, 0.01f, 20.0f);
}

void Phaser::process (float& left, float& right) noexcept
{
    if (stages == 0 || mix <= 0.0001f)
        return;

    phase += static_cast<double> (rate) / sampleRate;
    phase -= std::floor (phase);

    const float dryL = left;
    const float dryR = right;

    for (int channel = 0; channel < 2; ++channel)
    {
        const float lfo = std::sin (static_cast<float> (twoPi * (phase + (channel == 0 ? 0.0 : 0.27))));
        const float centreHz = 280.0f + depth * 1150.0f
                               + lfo * (180.0f + depth * 900.0f);
        const float clampedHz = std::clamp (centreHz, 45.0f, 6000.0f);
        const float w = static_cast<float> (std::tan (pi * clampedHz / sampleRate));
        const float a = (1.0f - w) / (1.0f + w);

        float signal = (channel == 0 ? dryL : dryR)
                       + feedbackState[static_cast<size_t> (channel)] * feedback;

        for (int stage = 0; stage < stages; ++stage)
        {
            auto& z = state[static_cast<size_t> (channel)][static_cast<size_t> (stage)];
            const float y = -a * signal + z;
            z = signal + a * y;
            signal = y;
        }

        feedbackState[static_cast<size_t> (channel)] = std::tanh (signal * 0.8f) * 0.72f;

        if (channel == 0)
            left = dryL * (1.0f - mix) + signal * mix;
        else
            right = dryR * (1.0f - mix) + signal * mix;
    }

    left = flush (left);
    right = flush (right);
}

//==============================================================================
void Delay::prepare (double newSampleRate)
{
    sampleRate = std::max (8000.0, newSampleRate);
    const auto size = static_cast<size_t> (std::ceil (sampleRate * 2.2) + 8.0);
    bufferL.assign (size, 0.0f);
    bufferR.assign (size, 0.0f);
    reset();
}

void Delay::reset() noexcept
{
    std::fill (bufferL.begin(), bufferL.end(), 0.0f);
    std::fill (bufferR.begin(), bufferR.end(), 0.0f);
    writePosition = 0;
    filterL = filterR = 0.0f;
}

void Delay::setParams (const DelayParams& params, double bpm) noexcept
{
    const double seconds = params.tempoSync
        ? (60.0 / std::max (20.0, bpm)) * static_cast<double> (params.syncBeats)
        : static_cast<double> (params.timeMs) * 0.001;
    delaySamples = static_cast<float> (std::clamp (seconds, 0.005, 2.0) * sampleRate);
    feedback = std::clamp (params.feedback, 0.0f, 0.92f);
    mix = std::clamp (params.mix, 0.0f, 1.0f);
    pingPong = params.pingPong;
    toneCoefficient = 1.0f - std::exp (-static_cast<float> (twoPi)
                                       * std::clamp (params.toneHz, 200.0f, 18000.0f)
                                       / static_cast<float> (sampleRate));
    toneCoefficient = std::clamp (toneCoefficient, 0.001f, 1.0f);
}

void Delay::process (float& left, float& right) noexcept
{
    if (bufferL.empty() || bufferR.empty())
        return;

    const int size = static_cast<int> (bufferL.size());
    float readPosition = static_cast<float> (writePosition) - delaySamples;

    while (readPosition < 0.0f)
        readPosition += static_cast<float> (size);
    while (readPosition >= static_cast<float> (size))
        readPosition -= static_cast<float> (size);

    const int index0 = static_cast<int> (readPosition);
    const int index1 = (index0 + 1) % size;
    const float fraction = readPosition - std::floor (readPosition);
    const float delayedL = bufferL[static_cast<size_t> (index0)]
                           + (bufferL[static_cast<size_t> (index1)] - bufferL[static_cast<size_t> (index0)]) * fraction;
    const float delayedR = bufferR[static_cast<size_t> (index0)]
                           + (bufferR[static_cast<size_t> (index1)] - bufferR[static_cast<size_t> (index0)]) * fraction;

    filterL += toneCoefficient * (delayedL - filterL);
    filterR += toneCoefficient * (delayedR - filterR);

    const float inL = left;
    const float inR = right;
    const float crossL = pingPong ? filterR : filterL;
    const float crossR = pingPong ? filterL : filterR;

    bufferL[static_cast<size_t> (writePosition)] = flush (inL + crossL * feedback);
    bufferR[static_cast<size_t> (writePosition)] = flush (inR + crossR * feedback);

    writePosition = (writePosition + 1) % size;
    left = flush (inL * (1.0f - mix) + delayedL * mix);
    right = flush (inR * (1.0f - mix) + delayedR * mix);
}

//==============================================================================
void PlateReverb::prepare (double newSampleRate)
{
    sampleRate = std::max (8000.0, newSampleRate);
    const double scale = sampleRate / 48000.0 * 1.35;

    const std::array<int, numCombs> baseComb { 1116, 1188, 1277, 1356 };
    const std::array<int, numAllpasses> baseAllpass { 556, 441 };

    for (int i = 0; i < numCombs; ++i)
    {
        combLengthL[static_cast<size_t> (i)] = static_cast<int> (baseComb[static_cast<size_t> (i)] * scale);
        combLengthR[static_cast<size_t> (i)] = combLengthL[static_cast<size_t> (i)] + 23;
        combL[static_cast<size_t> (i)].assign (static_cast<size_t> (combLengthL[static_cast<size_t> (i)] + 2), 0.0f);
        combR[static_cast<size_t> (i)].assign (static_cast<size_t> (combLengthR[static_cast<size_t> (i)] + 2), 0.0f);
        combWriteL[static_cast<size_t> (i)] = 0;
        combWriteR[static_cast<size_t> (i)] = 0;
    }

    for (int i = 0; i < numAllpasses; ++i)
    {
        allpassLengthL[static_cast<size_t> (i)] = static_cast<int> (baseAllpass[static_cast<size_t> (i)] * scale);
        allpassLengthR[static_cast<size_t> (i)] = allpassLengthL[static_cast<size_t> (i)] + 17;
        allpassL[static_cast<size_t> (i)].assign (static_cast<size_t> (allpassLengthL[static_cast<size_t> (i)] + 2), 0.0f);
        allpassR[static_cast<size_t> (i)].assign (static_cast<size_t> (allpassLengthR[static_cast<size_t> (i)] + 2), 0.0f);
        allpassWriteL[static_cast<size_t> (i)] = 0;
        allpassWriteR[static_cast<size_t> (i)] = 0;
    }

    preDelayL.assign (static_cast<size_t> (std::ceil (sampleRate * 0.14) + 4), 0.0f);
    preDelayR.assign (preDelayL.size(), 0.0f);
    reset();
}

void PlateReverb::reset() noexcept
{
    for (auto& buffer : combL) std::fill (buffer.begin(), buffer.end(), 0.0f);
    for (auto& buffer : combR) std::fill (buffer.begin(), buffer.end(), 0.0f);
    for (auto& buffer : allpassL) std::fill (buffer.begin(), buffer.end(), 0.0f);
    for (auto& buffer : allpassR) std::fill (buffer.begin(), buffer.end(), 0.0f);
    std::fill (preDelayL.begin(), preDelayL.end(), 0.0f);
    std::fill (preDelayR.begin(), preDelayR.end(), 0.0f);
    combFilterL.fill (0.0f);
    combFilterR.fill (0.0f);
    preDelayWrite = 0;
}

void PlateReverb::setParams (const ReverbParams& params) noexcept
{
    mix = std::clamp (params.mix, 0.0f, 1.0f);
    width = std::clamp (params.width, 0.0f, 1.0f);
    feedback = std::clamp (0.58f + params.decay * 0.39f, 0.0f, 0.975f);
    damping = 1.0f - std::exp (-static_cast<float> (twoPi)
                               * std::clamp (params.dampingHz, 400.0f, 16000.0f)
                               / static_cast<float> (sampleRate));
    damping = std::clamp (damping, 0.002f, 0.999f);
    inputGain = 0.34f + params.size * 0.12f;
    preDelayLength = std::min (static_cast<int> (preDelayL.size()) - 2,
                              static_cast<int> (params.predelayMs * 0.001f * sampleRate));
    preDelayLength = std::max (0, preDelayLength);
}

void PlateReverb::process (float& left, float& right) noexcept
{
    if (preDelayL.empty() || combL[0].empty())
        return;

    const float dryL = left;
    const float dryR = right;
    const float input = (dryL + dryR) * 0.5f * inputGain;

    preDelayL[static_cast<size_t> (preDelayWrite)] = input;
    preDelayR[static_cast<size_t> (preDelayWrite)] = input;
    preDelayWrite = (preDelayWrite + 1) % static_cast<int> (preDelayL.size());

    int readPosition = preDelayWrite - preDelayLength;
    if (readPosition < 0)
        readPosition += static_cast<int> (preDelayL.size());

    const float delayed = preDelayL[static_cast<size_t> (readPosition)];

    float sumL = 0.0f;
    float sumR = 0.0f;

    for (int i = 0; i < numCombs; ++i)
    {
        auto& buffer = combL[static_cast<size_t> (i)];
        const int length = combLengthL[static_cast<size_t> (i)];
        int& write = combWriteL[static_cast<size_t> (i)];
        const int read = (write + length - static_cast<int> (length * 0.83f)) % length;
        const float tap = buffer[static_cast<size_t> (read)];
        combFilterL[static_cast<size_t> (i)] += damping * (tap - combFilterL[static_cast<size_t> (i)]);
        buffer[static_cast<size_t> (write)] = flush (delayed + combFilterL[static_cast<size_t> (i)] * feedback);
        write = (write + 1) % length;
        sumL += tap;

        auto& bufferR = combR[static_cast<size_t> (i)];
        const int lengthR = combLengthR[static_cast<size_t> (i)];
        int& writeR = combWriteR[static_cast<size_t> (i)];
        const int readR = (writeR + lengthR - static_cast<int> (lengthR * 0.83f)) % lengthR;
        const float tapR = bufferR[static_cast<size_t> (readR)];
        combFilterR[static_cast<size_t> (i)] += damping * (tapR - combFilterR[static_cast<size_t> (i)]);
        bufferR[static_cast<size_t> (writeR)] = flush (delayed + combFilterR[static_cast<size_t> (i)] * feedback);
        writeR = (writeR + 1) % lengthR;
        sumR += tapR;
    }

    sumL *= 0.25f;
    sumR *= 0.25f;

    for (int i = 0; i < numAllpasses; ++i)
    {
        auto& buffer = allpassL[static_cast<size_t> (i)];
        const int length = allpassLengthL[static_cast<size_t> (i)];
        int& write = allpassWriteL[static_cast<size_t> (i)];
        const float stored = buffer[static_cast<size_t> (write)];
        const float output = stored - sumL;
        buffer[static_cast<size_t> (write)] = flush (sumL + stored * 0.5f);
        write = (write + 1) % length;
        sumL = output;

        auto& bufferR = allpassR[static_cast<size_t> (i)];
        const int lengthR = allpassLengthR[static_cast<size_t> (i)];
        int& writeR = allpassWriteR[static_cast<size_t> (i)];
        const float storedR = bufferR[static_cast<size_t> (writeR)];
        const float outputR = storedR - sumR;
        bufferR[static_cast<size_t> (writeR)] = flush (sumR + storedR * 0.5f);
        writeR = (writeR + 1) % lengthR;
        sumR = outputR;
    }

    const float wetL = sumL * width + sumR * (1.0f - width) * 0.35f;
    const float wetR = sumR * width + sumL * (1.0f - width) * 0.35f;
    left = flush (dryL * (1.0f - mix) + wetL * mix);
    right = flush (dryR * (1.0f - mix) + wetR * mix);
}

//==============================================================================
void OutputStage::prepare (double sampleRate) noexcept
{
    dcCoefficient = std::clamp (static_cast<float> (1.0 - twoPi * 7.0 / std::max (8000.0, sampleRate)),
                                0.99f, 0.99995f);
    reset();
}

void OutputStage::reset() noexcept
{
    dcXL = dcYL = dcXR = dcYR = 0.0f;
    trim = 1.0f;
}

void OutputStage::setTrimDb (float trimDb) noexcept
{
    trim = std::pow (10.0f, std::clamp (trimDb, -36.0f, 30.0f) / 20.0f);
}

float OutputStage::safetyClip (float x) noexcept
{
    if (! std::isfinite (x))
        return 0.0f;

    const float magnitude = std::abs (x);
    constexpr float knee = 0.70f;

    if (magnitude <= knee)
        return x;

    const float headroom = peakLimit - knee;
    const float limited = knee + headroom * std::tanh ((magnitude - knee) / headroom);
    return std::copysign (std::min (limited, peakLimit), x);
}

void OutputStage::process (float& left, float& right) noexcept
{
    left *= trim;
    right *= trim;

    dcYL = left - dcXL + dcCoefficient * dcYL;
    dcXL = left;
    dcYR = right - dcXR + dcCoefficient * dcYR;
    dcXR = right;

    left = safetyClip (dcYL);
    right = safetyClip (dcYR);
    dcYL = flush (dcYL);
    dcYR = flush (dcYR);
}
} // namespace synthset
