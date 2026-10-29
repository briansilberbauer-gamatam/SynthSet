#pragma once

#include "../preset/SynthPatch.h"
#include "Envelope.h"
#include "Filters.h"
#include "Lfo.h"
#include "ModMatrix.h"
#include "Oscillator.h"
#include "Wavetable.h"

#include <array>
#include <cstdint>

namespace synthset
{
class Voice
{
public:
    void prepare (double sampleRate);
    void reset() noexcept;

    void start (const SynthPatch& patch, int midiNote, float newVelocity,
                std::uint64_t newStartOrder, int layerCount, float glideFromNote,
                float pitchBend, float modWheel, float aftertouch) noexcept;
    void retarget (int midiNote, float newVelocity) noexcept;
    void release() noexcept;
    void forceStop() noexcept;

    void process (const SynthPatch& patch, float* left, float* right, int numSamples,
                  float globalLfo1, float globalLfo2, double bpm) noexcept;

    bool isActive() const noexcept { return active; }
    bool isReleasing() const noexcept { return releasing; }
    int getNote() const noexcept { return note; }
    float getVelocity() const noexcept { return velocity; }
    float getAmpLevel() const noexcept { return ampEnv.getLevel(); }
    std::uint64_t getStartOrder() const noexcept { return startOrder; }
    int getLayerCount() const noexcept { return numLayers; }

    float getDelaySendMod() const noexcept { return lastDelaySend; }
    float getReverbSendMod() const noexcept { return lastReverbSend; }
    float getChorusDepthMod() const noexcept { return lastChorusDepth; }

private:
    struct Layer
    {
        OscillatorState osc1;
        OscillatorState osc2;
        double subPhase = 0.0;
        std::uint32_t noiseState = 0x1234567u;
        float detuneCents = 0.0f;
        float drift1 = 0.0f;
        float drift2 = 0.0f;
        float pan = 0.0f;
        float gainL1 = 0.707f, gainR1 = 0.707f;
        float gainL2 = 0.707f, gainR2 = 0.707f;
    };

    void updateControl (const SynthPatch& patch, float globalLfo1, float globalLfo2,
                        double bpm, int chunkSamples) noexcept;
    void updateLayerPan (const SynthPatch& patch, int layerIndex,
                         float panModulation) noexcept;
    float midiToHz (float midiNote) const noexcept;
    static std::uint32_t splitMix (std::uint32_t x) noexcept;
    static float hashToBipolar (std::uint32_t x) noexcept;

    double sampleRate = 48000.0;
    const SynthPatch* patch = nullptr;
    bool active = false;
    bool releasing = false;
    int note = 60;
    float velocity = 0.8f;
    std::uint64_t startOrder = 0;
    int numLayers = 1;

    std::array<Layer, kMaxUnisonLayers> layers {};
    Adsr ampEnv, filterEnv;
    Lfo lfo1, lfo2;

    float currentNote = 60.0f;
    float targetNote = 60.0f;
    float glideCoefficient = 1.0f;
    float pitchBend = 0.0f;
    float modWheel = 0.0f;
    float aftertouch = 0.0f;

    double perVoiceLfo1Rate = 0.35;
    double perVoiceLfo2Rate = 0.21;
    FilterState filterL, filterR;
    FilterCoeffs filterCoeffs {};
    float smoothedCutoff = 1200.0f;
    float smoothedResonance = 0.2f;
    float smoothedPan = 0.0f;
    float gainL = 0.707f;
    float gainR = 0.707f;
    ModValues modValues {};
    float globalLfo1Value = 0.0f;
    float globalLfo2Value = 0.0f;
    float noteRandom = 0.0f;

    float lastDelaySend = 0.0f;
    float lastReverbSend = 0.0f;
    float lastChorusDepth = 0.0f;
};
} // namespace synthset
