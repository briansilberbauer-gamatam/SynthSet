#include "Voice.h"

#include <algorithm>
#include <cmath>

namespace synthset
{
void Voice::prepare (double newSampleRate)
{
    sampleRate = std::max (8000.0, newSampleRate);
    ampEnv.prepare (sampleRate);
    filterEnv.prepare (sampleRate);
    lfo1.prepare (sampleRate);
    lfo2.prepare (sampleRate);
    reset();
}

void Voice::reset() noexcept
{
    active = false;
    releasing = false;
    patch = nullptr;
    ampEnv.reset();
    filterEnv.reset();
    filterL.reset();
    filterR.reset();
    numLayers = 1;

    for (auto& layer : layers)
    {
        layer.osc1 = {};
        layer.osc2 = {};
        layer.subPhase = 0.0;
        layer.noiseState = 0x1234567u;
        layer.detuneCents = 0.0f;
        layer.drift1 = 0.0f;
        layer.drift2 = 0.0f;
        layer.pan = 0.0f;
    }
}

std::uint32_t Voice::splitMix (std::uint32_t x) noexcept
{
    x += 0x9e3779b9u;
    x = (x ^ (x >> 16)) * 0x85ebca6bu;
    x = (x ^ (x >> 13)) * 0xc2b2ae35u;
    return x ^ (x >> 16);
}

float Voice::hashToBipolar (std::uint32_t x) noexcept
{
    return static_cast<float> (splitMix (x) >> 8) * (1.0f / 8388608.0f) - 1.0f;
}

void Voice::start (const SynthPatch& newPatch, int midiNote, float newVelocity,
                   std::uint64_t newStartOrder, int layerCount, float glideFromNote,
                   float newPitchBend, float newModWheel, float newAftertouch) noexcept
{
    reset();

    patch = &newPatch;
    note = std::clamp (midiNote, 0, 127);
    targetNote = static_cast<float> (note);
    currentNote = std::clamp (glideFromNote, 0.0f, 127.0f);
    velocity = std::clamp (newVelocity, 0.001f, 1.0f);
    startOrder = newStartOrder;
    numLayers = std::clamp (layerCount, 1, kMaxUnisonLayers);
    pitchBend = newPitchBend;
    modWheel = newModWheel;
    aftertouch = newAftertouch;
    releasing = false;
    active = true;

    noteRandom = hashToBipolar (newPatch.seed ^ static_cast<std::uint32_t> (note) * 0x9e3779b9u
                                ^ static_cast<std::uint32_t> (newStartOrder));

    ampEnv.setParams (newPatch.ampEnv);
    filterEnv.setParams (newPatch.filterEnv);
    ampEnv.noteOn();
    filterEnv.noteOn();

    lfo1.setParams (newPatch.lfo1);
    lfo2.setParams (newPatch.lfo2);

    if (newPatch.lfo1.perVoice && newPatch.lfo1.retrigger)
        lfo1.reset (newPatch.seed ^ 0x1111u, 0.0f);
    if (newPatch.lfo2.perVoice && newPatch.lfo2.retrigger)
        lfo2.reset (newPatch.seed ^ 0x2222u, 0.0f);

    const float layerCentre = static_cast<float> (numLayers - 1) * 0.5f;

    for (int i = 0; i < kMaxUnisonLayers; ++i)
    {
        auto& layer = layers[static_cast<size_t> (i)];
        const std::uint32_t hash = splitMix (newPatch.seed
                                             ^ static_cast<std::uint32_t> (note) * 0x85ebca6bu
                                             ^ static_cast<std::uint32_t> (i) * 0xc2b2ae35u);

        layer.osc1.phase = static_cast<double> (splitMix (hash) >> 8) / 4294967295.0 * 0.35;
        layer.osc2.phase = static_cast<double> (splitMix (hash ^ 0x5151u) >> 8) / 4294967295.0 * 0.35;
        layer.subPhase = 0.0;
        layer.noiseState = splitMix (hash ^ 0x9e3779b9u) | 1u;
        layer.detuneCents = (static_cast<float> (i) - layerCentre)
                            * (numLayers > 1 ? newPatch.unisonDetune : 0.0f);
        layer.drift1 = hashToBipolar (hash ^ 0x101u) * newPatch.osc1.drift * 16.0f;
        layer.drift2 = hashToBipolar (hash ^ 0x202u) * newPatch.osc2.drift * 16.0f;
        layer.pan = numLayers > 1
                        ? ((static_cast<float> (i) - layerCentre) / std::max (0.5f, layerCentre))
                              * newPatch.unisonSpread
                        : 0.0f;
        layer.gainL1 = layer.gainR1 = 0.707f;
        layer.gainL2 = layer.gainR2 = 0.707f;
    }

    filterL.reset();
    filterR.reset();
    smoothedCutoff = newPatch.filter.cutoffHz;
    smoothedResonance = newPatch.filter.resonance;
    smoothedPan = 0.0f;
    filterCoeffs = makeFilterCoeffs (newPatch.filter, smoothedCutoff,
                                     smoothedResonance, sampleRate);
    gainL = gainR = 0.707f;
    lastDelaySend = lastReverbSend = lastChorusDepth = 0.0f;

    for (int i = 0; i < numLayers; ++i)
        updateLayerPan (newPatch, i, 0.0f);
}

void Voice::retarget (int midiNote, float newVelocity) noexcept
{
    if (! active || patch == nullptr)
        return;

    note = std::clamp (midiNote, 0, 127);
    targetNote = static_cast<float> (note);
    velocity = std::clamp (newVelocity, 0.001f, 1.0f);
    releasing = false;
}

void Voice::release() noexcept
{
    if (! active)
        return;

    releasing = true;
    ampEnv.noteOff();
    filterEnv.noteOff();
}

void Voice::forceStop() noexcept
{
    reset();
}

float Voice::midiToHz (float midiNote) const noexcept
{
    const float frequency = 440.0f * std::exp2 ((midiNote - 69.0f) / 12.0f);
    return std::clamp (frequency, 0.01f, static_cast<float> (sampleRate) * 0.45f);
}

void Voice::updateLayerPan (const SynthPatch& p, int layerIndex, float panModulation) noexcept
{
    auto& layer = layers[static_cast<size_t> (layerIndex)];
    const float pan1 = std::clamp (p.osc1.pan + layer.pan + panModulation, -1.0f, 1.0f);
    const float pan2 = std::clamp (p.osc2.pan + layer.pan + panModulation, -1.0f, 1.0f);

    layer.gainL1 = std::sqrt (0.5f * (1.0f - pan1));
    layer.gainR1 = std::sqrt (0.5f * (1.0f + pan1));
    layer.gainL2 = std::sqrt (0.5f * (1.0f - pan2));
    layer.gainR2 = std::sqrt (0.5f * (1.0f + pan2));
}

void Voice::updateControl (const SynthPatch& p, float globalOne, float globalTwo,
                           double bpm, int chunkSamples) noexcept
{
    globalLfo1Value = globalOne;
    globalLfo2Value = globalTwo;

    const float rate1 = lfoRateHz (p.lfo1, bpm);
    const float rate2 = lfoRateHz (p.lfo2, bpm);
    const float lfo1Value = p.lfo1.perVoice ? lfo1.advance (chunkSamples, rate1) : globalLfo1Value;
    const float lfo2Value = p.lfo2.perVoice ? lfo2.advance (chunkSamples, rate2) : globalLfo2Value;

    ModSources sources;
    sources.lfo1 = lfo1Value;
    sources.lfo2 = lfo2Value;
    sources.filterEnv = filterEnv.getLevel();
    sources.ampEnv = ampEnv.getLevel();
    sources.velocity = velocity;
    sources.keyTrack = std::clamp (static_cast<float> (note - 60) / 36.0f, -1.0f, 1.0f);
    sources.modWheel = modWheel;
    sources.aftertouch = aftertouch;
    sources.pitchBend = pitchBend;
    sources.noteRandom = noteRandom;

    modValues = evaluateModMatrix (p, sources);
    lastDelaySend = modValues.delaySend;
    lastReverbSend = modValues.reverbSend;
    lastChorusDepth = modValues.chorusDepth;

    if (p.glideMs > 0.1f && std::abs (targetNote - currentNote) > 0.001f)
    {
        const float glideSeconds = p.glideMs * 0.001f;
        const float alpha = 1.0f - std::exp (-static_cast<float> (chunkSamples)
                                             / (glideSeconds * static_cast<float> (sampleRate)));
        currentNote += (targetNote - currentNote) * alpha;
    }
    else
    {
        currentNote = targetNote;
    }

    const float envOctaves = p.filter.envAmount * filterEnv.getLevel();
    const float keyOctaves = p.filter.keyTrack * static_cast<float> (note - 60) / 12.0f;
    const float velocityOctaves = p.filter.velocity * velocity * 1.4f;
    const float cutoffOctaves = std::clamp (envOctaves + keyOctaves + velocityOctaves
                                                 + modValues.cutoffOctaves
                                                 + aftertouch * 0.25f,
                                             -8.0f, 8.0f);
    const float targetCutoff = std::clamp (p.filter.cutoffHz * std::exp2 (cutoffOctaves),
                                          18.0f, static_cast<float> (sampleRate) * 0.45f);
    const float targetResonance = std::clamp (p.filter.resonance + modValues.resonance * 0.5f,
                                              0.0f, 1.0f);

    smoothedCutoff += (targetCutoff - smoothedCutoff) * 0.35f;
    smoothedResonance += (targetResonance - smoothedResonance) * 0.25f;
    filterCoeffs = makeFilterCoeffs (p.filter, smoothedCutoff, smoothedResonance, sampleRate);

    smoothedPan += (modValues.pan - smoothedPan) * 0.25f;
    gainL = std::sqrt (0.5f * (1.0f - smoothedPan));
    gainR = std::sqrt (0.5f * (1.0f + smoothedPan));

    for (int i = 0; i < numLayers; ++i)
        updateLayerPan (p, i, smoothedPan);
}

void Voice::process (const SynthPatch& p, float* left, float* right, int numSamples,
                     float globalOne, float globalTwo, double bpm) noexcept
{
    if (! active || left == nullptr || right == nullptr || numSamples <= 0)
        return;

    const SynthPatch& currentPatch = patch != nullptr ? *patch : p;
    const float layerGain = 0.28f / std::sqrt (static_cast<float> (numLayers));
    const float velocityScale = 1.0f - currentPatch.ampEnv.velocity
                                + currentPatch.ampEnv.velocity * velocity;

    int offset = 0;

    while (offset < numSamples)
    {
        const int chunk = std::min (16, numSamples - offset);
        updateControl (currentPatch, globalOne, globalTwo, bpm, chunk);

        for (int i = 0; i < chunk; ++i)
        {
            filterEnv.process();
            const float ampEnvLevel = ampEnv.process();

            const float bendSemitones = pitchBend * currentPatch.pitchBendRange;
            const float wheelVibrato = currentPatch.modWheelDepth * modWheel
                                       * (currentPatch.lfo1.perVoice
                                              ? lfo1.getLastValue() : globalLfo1Value);
            const float baseMidi = currentNote + bendSemitones + modValues.pitchSemitones
                                   + wheelVibrato * 0.7f;

            float mixedL = 0.0f;
            float mixedR = 0.0f;

            for (int layerIndex = 0; layerIndex < numLayers; ++layerIndex)
            {
                auto& layer = layers[static_cast<size_t> (layerIndex)];

                const float freq1 = midiToHz (baseMidi
                                              + currentPatch.osc1.octave * 12.0f
                                              + currentPatch.osc1.semitone
                                              + (currentPatch.osc1.fineCents + layer.detuneCents
                                                 + layer.drift1) / 100.0f);
                const float freq2 = midiToHz (baseMidi
                                              + currentPatch.osc2.octave * 12.0f
                                              + currentPatch.osc2.semitone
                                              + (currentPatch.osc2.fineCents + layer.detuneCents
                                                 + layer.drift2) / 100.0f);

                const double increment1 = static_cast<double> (freq1) / sampleRate;
                const double increment2 = static_cast<double> (freq2) / sampleRate;

                layer.osc1.phase += increment1;
                const bool masterWrapped = layer.osc1.phase >= 1.0;
                if (masterWrapped)
                    layer.osc1.phase -= 1.0;

                if (masterWrapped && currentPatch.osc2.sync)
                    layer.osc2.phase = 0.0;

                layer.osc2.phase += increment2;
                if (layer.osc2.phase >= 1.0)
                    layer.osc2.phase -= 1.0;

                const float pulseWidth1 = std::clamp (currentPatch.osc1.pulseWidth
                                                           + modValues.pulseWidth1,
                                                       0.05f, 0.95f);
                const float pulseWidth2 = std::clamp (currentPatch.osc2.pulseWidth
                                                           + modValues.pulseWidth2,
                                                       0.05f, 0.95f);
                const float wavetablePos = std::clamp (currentPatch.osc1.wavetablePos
                                                            + modValues.wavetablePos,
                                                       0.0f, 1.0f);

                const float shaped1 = currentPatch.osc1.wavetable
                    ? getSharedWavetableBank().sample (wavetablePos,
                                                       static_cast<float> (layer.osc1.phase),
                                                       freq1, sampleRate)
                    : renderAnalogShape (layer.osc1.phase, increment1,
                                         currentPatch.osc1.shape, pulseWidth1,
                                         layer.osc1.triangleState);

                const float shaped2 = renderOscillator (layer.osc2, currentPatch.osc2,
                                                        increment2, pulseWidth2,
                                                        std::clamp (currentPatch.osc2.wavetablePos
                                                                        + modValues.wavetablePos,
                                                                    0.0f, 1.0f),
                                                        getSharedWavetableBank(), sampleRate, freq2);

                const float subFrequency = freq1 * 0.5f;
                const double subIncrement = static_cast<double> (subFrequency) / sampleRate;
                layer.subPhase += subIncrement;
                if (layer.subPhase >= 1.0)
                    layer.subPhase -= 1.0;
                const float sub = currentPatch.subSquare
                    ? pulseWave (layer.subPhase, subIncrement, 0.5f)
                    : sineWave (layer.subPhase);

                const float noise = currentPatch.noiseLevel > 0.0f
                    ? deterministicNoise (layer.noiseState) : 0.0f;
                const float ring = shaped1 * shaped2;

                const float osc1Signal = shaped1 * currentPatch.osc1.level;
                const float osc2Signal = shaped2 * currentPatch.osc2.level;
                const float subSignal = sub * currentPatch.subLevel;
                const float noiseSignal = noise * currentPatch.noiseLevel;
                const float ringSignal = ring * currentPatch.ringLevel;

                mixedL += (osc1Signal * layer.gainL1
                           + osc2Signal * layer.gainL2
                           + (subSignal + noiseSignal + ringSignal) * 0.7071f) * layerGain;
                mixedR += (osc1Signal * layer.gainR1
                           + osc2Signal * layer.gainR2
                           + (subSignal + noiseSignal + ringSignal) * 0.7071f) * layerGain;
            }

            const float filteredL = filterL.process (mixedL, filterCoeffs,
                                                      currentPatch.filter.model,
                                                      currentPatch.filter.type);
            const float filteredR = filterR.process (mixedR, filterCoeffs,
                                                      currentPatch.filter.model,
                                                      currentPatch.filter.type);

            const float amp = std::clamp (ampEnvLevel * velocityScale * modValues.amp,
                                          0.0f, 1.8f);
            left[offset + i] += filteredL * amp * gainL;
            right[offset + i] += filteredR * amp * gainR;
        }

        offset += chunk;
    }

    if (! ampEnv.isActive()
        || (ampEnv.getStage() == Adsr::Stage::Sustain && ampEnv.getLevel() < 1.0e-5f))
    {
        active = false;
        releasing = true;
    }
}
} // namespace synthset
