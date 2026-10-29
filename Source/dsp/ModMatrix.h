#pragma once

#include "../preset/SynthPatch.h"

#include <algorithm>

namespace synthset
{
struct ModSources
{
    float lfo1 = 0.0f;
    float lfo2 = 0.0f;
    float filterEnv = 0.0f;
    float ampEnv = 0.0f;
    float velocity = 0.0f;
    float keyTrack = 0.0f;
    float modWheel = 0.0f;
    float aftertouch = 0.0f;
    float pitchBend = 0.0f;
    float noteRandom = 0.0f;
};

struct ModValues
{
    float pitchSemitones = 0.0f;
    float pulseWidth1 = 0.0f;
    float pulseWidth2 = 0.0f;
    float wavetablePos = 0.0f;
    float cutoffOctaves = 0.0f;
    float resonance = 0.0f;
    float amp = 1.0f;
    float pan = 0.0f;
    float chorusDepth = 0.0f;
    float delaySend = 0.0f;
    float reverbSend = 0.0f;
};

inline float sourceValue (ModSource source, const ModSources& s) noexcept
{
    switch (source)
    {
        case ModSource::Lfo1:      return s.lfo1;
        case ModSource::Lfo2:      return s.lfo2;
        case ModSource::FilterEnv: return s.filterEnv;
        case ModSource::AmpEnv:    return s.ampEnv;
        case ModSource::Velocity:  return s.velocity;
        case ModSource::KeyTrack:  return s.keyTrack;
        case ModSource::ModWheel:  return s.modWheel;
        case ModSource::Aftertouch:return s.aftertouch;
        case ModSource::PitchBend: return s.pitchBend;
        case ModSource::NoteRandom:return s.noteRandom;
        case ModSource::None:
        default:                   return 0.0f;
    }
}

inline ModValues evaluateModMatrix (const SynthPatch& patch, const ModSources& sources) noexcept
{
    ModValues v;

    for (const auto& route : patch.mod)
    {
        if (route.source == ModSource::None || route.dest == ModDest::None)
            continue;

        const float value = sourceValue (route.source, sources) * route.amount;

        switch (route.dest)
        {
            case ModDest::Pitch:        v.pitchSemitones += value * 12.0f; break;
            case ModDest::PulseWidth1:  v.pulseWidth1 += value * 0.45f; break;
            case ModDest::PulseWidth2:  v.pulseWidth2 += value * 0.45f; break;
            case ModDest::WavetablePos: v.wavetablePos += value; break;
            case ModDest::Cutoff:       v.cutoffOctaves += value * 4.0f; break;
            case ModDest::Resonance:    v.resonance += value; break;
            case ModDest::Amp:          v.amp += value; break;
            case ModDest::Pan:          v.pan += value; break;
            case ModDest::ChorusDepth:  v.chorusDepth += value; break;
            case ModDest::DelaySend:    v.delaySend += value; break;
            case ModDest::ReverbSend:   v.reverbSend += value; break;
            case ModDest::None:
            default: break;
        }
    }

    v.pitchSemitones = std::clamp (v.pitchSemitones, -48.0f, 48.0f);
    v.cutoffOctaves = std::clamp (v.cutoffOctaves, -8.0f, 8.0f);
    v.resonance = std::clamp (v.resonance, -1.0f, 1.0f);
    v.amp = std::clamp (v.amp, 0.0f, 1.8f);
    v.pan = std::clamp (v.pan, -1.0f, 1.0f);
    v.chorusDepth = std::clamp (v.chorusDepth, -1.0f, 1.0f);
    v.delaySend = std::clamp (v.delaySend, -1.0f, 1.0f);
    v.reverbSend = std::clamp (v.reverbSend, -1.0f, 1.0f);
    return v;
}
} // namespace synthset
