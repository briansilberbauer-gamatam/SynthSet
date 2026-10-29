#pragma once

#include "../preset/SynthPatch.h"

#include <array>
#include <vector>

namespace synthset
{
class Chorus
{
public:
    void prepare (double sampleRate);
    void reset() noexcept;
    void setParams (const ChorusParams& params) noexcept;
    void process (float& left, float& right, float depthModulation = 0.0f) noexcept;

private:
    static constexpr int bufferSize = 8192;
    std::array<float, bufferSize> bufferL {};
    std::array<float, bufferSize> bufferR {};
    std::array<double, 3> phase {};
    double sampleRate = 48000.0;
    int writePosition = 0;
    float depth = 0.0f;
    float mix = 0.0f;
    int mode = 0;
};

class Phaser
{
public:
    void prepare (double sampleRate) noexcept;
    void reset() noexcept;
    void setParams (const PhaserParams& params) noexcept;
    void process (float& left, float& right) noexcept;

private:
    std::array<std::array<float, 6>, 2> state {};
    std::array<float, 2> feedbackState {};
    double sampleRate = 48000.0;
    double phase = 0.0;
    int stages = 0;
    float depth = 0.0f;
    float feedback = 0.0f;
    float mix = 0.0f;
    float rate = 0.0f;
};

class Delay
{
public:
    void prepare (double sampleRate);
    void reset() noexcept;
    void setParams (const DelayParams& params, double bpm) noexcept;
    void process (float& left, float& right) noexcept;

private:
    std::vector<float> bufferL;
    std::vector<float> bufferR;
    double sampleRate = 48000.0;
    int writePosition = 0;
    float delaySamples = 1.0f;
    float feedback = 0.0f;
    float mix = 0.0f;
    float toneCoefficient = 1.0f;
    bool pingPong = true;
    float filterL = 0.0f;
    float filterR = 0.0f;
};

class PlateReverb
{
public:
    void prepare (double sampleRate);
    void reset() noexcept;
    void setParams (const ReverbParams& params) noexcept;
    void process (float& left, float& right) noexcept;

private:
    static constexpr int numCombs = 4;
    static constexpr int numAllpasses = 2;
    std::array<std::vector<float>, numCombs> combL;
    std::array<std::vector<float>, numCombs> combR;
    std::array<int, numCombs> combLengthL {};
    std::array<int, numCombs> combLengthR {};
    std::array<int, numCombs> combWriteL {};
    std::array<int, numCombs> combWriteR {};
    std::array<float, numCombs> combFilterL {};
    std::array<float, numCombs> combFilterR {};

    std::array<std::vector<float>, numAllpasses> allpassL;
    std::array<std::vector<float>, numAllpasses> allpassR;
    std::array<int, numAllpasses> allpassLengthL {};
    std::array<int, numAllpasses> allpassLengthR {};
    std::array<int, numAllpasses> allpassWriteL {};
    std::array<int, numAllpasses> allpassWriteR {};

    std::vector<float> preDelayL;
    std::vector<float> preDelayR;
    int preDelayLength = 0;
    int preDelayWrite = 0;

    double sampleRate = 48000.0;
    float feedback = 0.0f;
    float damping = 0.5f;
    float mix = 0.0f;
    float width = 0.8f;
    float inputGain = 0.5f;
};

class OutputStage
{
public:
    void prepare (double sampleRate) noexcept;
    void reset() noexcept;
    void setTrimDb (float trimDb) noexcept;
    void process (float& left, float& right) noexcept;

    static constexpr float peakLimit = 0.86f;

private:
    float dcCoefficient = 0.999f;
    float dcXL = 0.0f, dcYL = 0.0f;
    float dcXR = 0.0f, dcYR = 0.0f;
    float trim = 1.0f;

    static float safetyClip (float x) noexcept;
};
} // namespace synthset
