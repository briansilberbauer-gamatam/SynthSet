#include "PresetBank.h"

#include <algorithm>
#include <cstring>

namespace synthset
{
namespace
{
SynthPatch basePatch (const char* id, float trimDb = -7.0f)
{
    SynthPatch p;
    p.seed = fnv1a (id);
    p.outputTrimDb = trimDb;
    p.osc1.level = 0.50f;
    p.osc2.level = 0.42f;
    p.osc1.drift = 0.12f;
    p.osc2.drift = 0.15f;
    p.lfo1.rateHz = 0.0f;
    p.lfo2.rateHz = 0.0f;
    p.chorus.mode = 0;
    p.chorus.mix = 0.0f;
    p.phaser.stages = 0;
    p.phaser.mix = 0.0f;
    p.delay.mix = 0.0f;
    p.delay.feedback = 0.0f;
    p.reverb.mix = 0.0f;
    return p;
}

void envelope (AdsrParams& env, float attackMs, float decayMs, float sustain,
               float releaseMs, float curve = 0.55f, float velocity = 0.0f)
{
    env.attackMs = attackMs;
    env.decayMs = decayMs;
    env.sustain = sustain;
    env.releaseMs = releaseMs;
    env.curve = curve;
    env.velocity = velocity;
}

void lfo (LfoParams& l, LfoShape shape, float rateHz, float fadeInMs = 0.0f,
          float phase = 0.0f, bool perVoice = true, bool tempoSync = false,
          float syncBeats = 1.0f)
{
    l.shape = shape;
    l.rateHz = rateHz;
    l.fadeInMs = fadeInMs;
    l.phase = phase;
    l.perVoice = perVoice;
    l.tempoSync = tempoSync;
    l.syncBeats = syncBeats;
}

void route (SynthPatch& p, int slot, ModSource source, ModDest dest, float amount)
{
    if (slot >= 0 && slot < static_cast<int> (p.mod.size()))
        p.mod[static_cast<size_t> (slot)] = { source, dest, amount };
}

void setChorus (SynthPatch& p, int mode, float depth, float mix)
{
    p.chorus.mode = mode;
    p.chorus.depth = depth;
    p.chorus.mix = mix;
}

void setDelay (SynthPatch& p, float timeMs, float feedback, float toneHz, float mix,
               bool pingPong = true, bool tempoSync = false, float syncBeats = 0.75f)
{
    p.delay.timeMs = timeMs;
    p.delay.feedback = feedback;
    p.delay.toneHz = toneHz;
    p.delay.mix = mix;
    p.delay.pingPong = pingPong;
    p.delay.tempoSync = tempoSync;
    p.delay.syncBeats = syncBeats;
}

void setReverb (SynthPatch& p, float size, float decay, float dampingHz,
                float predelayMs, float mix, float width = 0.85f)
{
    p.reverb.size = size;
    p.reverb.decay = decay;
    p.reverb.dampingHz = dampingHz;
    p.reverb.predelayMs = predelayMs;
    p.reverb.mix = mix;
    p.reverb.width = width;
}

void setPhaser (SynthPatch& p, int stages, float rateHz, float depth,
                float feedback, float mix)
{
    p.phaser.stages = stages;
    p.phaser.rateHz = rateHz;
    p.phaser.depth = depth;
    p.phaser.feedback = feedback;
    p.phaser.mix = mix;
}

void setArp (SynthPatch& p, bool enabled, int rateDiv, int octaves, int pattern,
             float gate, bool latch = false)
{
    p.arp.enabled = enabled;
    p.arp.rateDiv = rateDiv;
    p.arp.octaves = octaves;
    p.arp.pattern = pattern;
    p.arp.gate = gate;
    p.arp.latch = latch;
}

ArtworkSpec artwork (Group group, const char* id, Motif motif, float energy,
                     float haze, std::uint32_t accent, std::uint32_t accent2)
{
    ArtworkSpec a;
    a.seed = seedFromId (id);
    a.motif = motif;
    a.energy = energy;
    a.haze = haze;
    a.accent = accent;
    a.accent2 = accent2;

    switch (group)
    {
        case Group::Bass:
            a.top = 0xff03060d; a.mid = 0xff081426; a.bottom = 0xff010307;
            break;
        case Group::Pads:
            a.top = 0xff060512; a.mid = 0xff1a1036; a.bottom = 0xff020208;
            break;
        case Group::Keys:
            a.top = 0xff0a0807; a.mid = 0xff241a10; a.bottom = 0xff050302;
            break;
        case Group::Leads:
            a.top = 0xff0c0308; a.mid = 0xff2a0717; a.bottom = 0xff050104;
            break;
        case Group::PlucksArps:
            a.top = 0xff020a0a; a.mid = 0xff052322; a.bottom = 0xff010505;
            break;
        case Group::BrassStrings:
            a.top = 0xff0b0503; a.mid = 0xff2b1407; a.bottom = 0xff050201;
            break;
        case Group::Textures:
            a.top = 0xff020409; a.mid = 0xff071a22; a.bottom = 0xff010205;
            break;
        case Group::Motion:
            a.top = 0xff05030a; a.mid = 0xff11142e; a.bottom = 0xff020106;
            break;
        case Group::Count:
        default:
            break;
    }

    return a;
}

Preset makePreset (const char* id, const char* name, const char* lineage,
                   const char* description, std::array<const char*, 4> tags,
                   Group group, Collection collection, ArtworkSpec art,
                   SynthPatch patch)
{
    Preset p;
    p.id = id;
    p.name = name;
    p.lineage = lineage;
    p.description = description;
    p.tags = tags;
    p.group = group;
    p.collection = collection;
    p.artwork = art;
    p.patch = patch;
    return p;
}

//==============================================================================
SynthPatch make106Sub()
{
    auto p = basePatch ("classic.bass.106-sub", -4.0f);
    p.osc1.shape = 0.33f; p.osc1.pulseWidth = 0.42f; p.osc1.level = 0.68f; p.osc1.octave = -1.0f;
    p.osc1.drift = 0.08f;
    p.osc2.level = 0.0f;
    p.subLevel = 0.44f; p.subSquare = true;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 180.0f; p.filter.resonance = 0.16f;
    p.filter.envAmount = 0.78f; p.filter.keyTrack = 0.25f; p.filter.drive = 0.08f;
    p.filter.highPassHz = 22.0f;
    envelope (p.filterEnv, 1.0f, 180.0f, 0.10f, 180.0f, 0.7f);
    envelope (p.ampEnv, 4.0f, 500.0f, 0.55f, 180.0f, 0.62f);
    p.voiceMode = VoiceMode::Mono;
    p.glideMs = 0.0f;
    setReverb (p, 0.30f, 0.30f, 6500.0f, 8.0f, 0.045f, 0.7f);
    return p;
}

SynthPatch makeFiveVoiceLow()
{
    auto p = basePatch ("classic.bass.five-voice-low", -6.0f);
    p.osc1.shape = 0.0f; p.osc1.level = 0.46f; p.osc1.fineCents = -6.0f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.46f; p.osc2.fineCents = 7.0f;
    p.subLevel = 0.16f; p.subSquare = false;
    p.filter.model = FilterModel::Ladder4P;
    p.filter.cutoffHz = 240.0f; p.filter.resonance = 0.28f;
    p.filter.envAmount = 1.55f; p.filter.keyTrack = 0.35f; p.filter.drive = 0.12f;
    envelope (p.filterEnv, 2.0f, 280.0f, 0.14f, 250.0f, 0.7f);
    envelope (p.ampEnv, 2.0f, 500.0f, 0.52f, 220.0f, 0.55f);
    p.osc1.drift = 0.18f; p.osc2.drift = 0.2f;
    return p;
}

SynthPatch makeSunsetThump()
{
    auto p = basePatch ("classic.bass.sunset-thump", -6.0f);
    p.osc1.shape = 0.33f; p.osc1.pulseWidth = 0.36f; p.osc1.level = 0.48f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.42f; p.osc2.fineCents = 9.0f;
    p.subLevel = 0.18f;
    p.filter.model = FilterModel::Ladder4P;
    p.filter.cutoffHz = 300.0f; p.filter.resonance = 0.22f;
    p.filter.envAmount = 1.22f; p.filter.keyTrack = 0.3f; p.filter.drive = 0.22f;
    envelope (p.filterEnv, 2.0f, 220.0f, 0.20f, 180.0f, 0.7f);
    envelope (p.ampEnv, 3.0f, 350.0f, 0.55f, 250.0f, 0.55f);
    p.voiceMode = VoiceMode::Mono;
    p.drive = 0.18f;
    return p;
}

SynthPatch makeGlassSub()
{
    auto p = basePatch ("modern.bass.glass-sub", -4.0f);
    p.osc1.shape = 1.0f; p.osc1.level = 0.62f; p.osc1.octave = -1.0f; p.osc1.drift = 0.02f;
    p.osc2.wavetable = true; p.osc2.wavetablePos = 0.18f; p.osc2.level = 0.22f;
    p.osc2.octave = 0.0f;
    p.subLevel = 0.35f; p.subSquare = false;
    p.filter.model = FilterModel::Ladder4P;
    p.filter.cutoffHz = 95.0f; p.filter.resonance = 0.10f;
    p.filter.envAmount = 0.28f; p.filter.keyTrack = 0.25f; p.filter.drive = 0.04f;
    envelope (p.filterEnv, 2.0f, 350.0f, 0.5f, 250.0f, 0.5f);
    envelope (p.ampEnv, 4.0f, 180.0f, 0.80f, 220.0f, 0.42f);
    p.voiceMode = VoiceMode::Legato;
    p.glideMs = 0.0f;
    p.drive = 0.04f;
    setReverb (p, 0.2f, 0.2f, 4000.0f, 0.0f, 0.0f);
    return p;
}

SynthPatch makeNeonReese()
{
    auto p = basePatch ("modern.bass.neon-reese", -9.0f);
    p.osc1.shape = 0.0f; p.osc1.level = 0.44f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.42f; p.osc2.fineCents = -13.0f;
    p.osc1.drift = 0.22f; p.osc2.drift = 0.22f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 420.0f; p.filter.resonance = 0.25f;
    p.filter.envAmount = 0.6f; p.filter.keyTrack = 0.2f;
    envelope (p.filterEnv, 60.0f, 900.0f, 0.45f, 900.0f, 0.55f);
    envelope (p.ampEnv, 8.0f, 1400.0f, 0.88f, 700.0f, 0.45f);
    p.voiceMode = VoiceMode::Unison;
    p.unisonVoices = 3; p.unisonDetune = 11.0f; p.unisonSpread = 0.82f;
    setPhaser (p, 4, 0.16f, 0.55f, 0.28f, 0.18f);
    setChorus (p, 1, 0.30f, 0.14f);
    setDelay (p, 380.0f, 0.24f, 3200.0f, 0.09f);
    setReverb (p, 0.45f, 0.42f, 4200.0f, 12.0f, 0.07f);
    return p;
}

SynthPatch makePrismBass()
{
    auto p = basePatch ("modern.bass.prism-bass", -5.0f);
    p.osc1.wavetable = true; p.osc1.wavetablePos = 0.55f; p.osc1.level = 0.50f;
    p.osc2.shape = 0.33f; p.osc2.pulseWidth = 0.26f; p.osc2.level = 0.30f;
    p.osc2.fineCents = 7.0f;
    p.filter.model = FilterModel::Ladder4P;
    p.filter.cutoffHz = 650.0f; p.filter.resonance = 0.20f;
    p.filter.envAmount = 2.0f; p.filter.keyTrack = 0.25f;
    envelope (p.filterEnv, 1.0f, 180.0f, 0.02f, 140.0f, 0.8f);
    envelope (p.ampEnv, 1.0f, 300.0f, 0.15f, 180.0f, 0.7f);
    p.voiceMode = VoiceMode::Mono;
    p.drive = 0.10f;
    setChorus (p, 1, 0.24f, 0.18f);
    setDelay (p, 280.0f, 0.20f, 5600.0f, 0.10f);
    return p;
}

//==============================================================================
SynthPatch make106Dream()
{
    auto p = basePatch ("classic.pad.106-dream", -9.0f);
    p.osc1.shape = 0.33f; p.osc1.pulseWidth = 0.42f; p.osc1.level = 0.55f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.45f; p.osc2.fineCents = 7.0f;
    p.subLevel = 0.28f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 1800.0f; p.filter.resonance = 0.18f;
    p.filter.envAmount = 1.2f; p.filter.keyTrack = 0.3f;
    envelope (p.filterEnv, 180.0f, 900.0f, 0.55f, 2200.0f, 0.5f);
    envelope (p.ampEnv, 1200.0f, 1400.0f, 0.85f, 3200.0f, 0.4f);
    lfo (p.lfo1, LfoShape::Sine, 0.28f, 0.0f, 0.0f);
    route (p, 0, ModSource::Lfo1, ModDest::PulseWidth1, 0.22f);
    route (p, 1, ModSource::Lfo1, ModDest::Cutoff, 0.12f);
    p.osc1.drift = 0.35f; p.osc2.drift = 0.35f;
    setChorus (p, 2, 0.65f, 0.40f);
    setReverb (p, 0.62f, 0.55f, 5800.0f, 18.0f, 0.22f);
    return p;
}

SynthPatch makeFiveVoiceHalo()
{
    auto p = basePatch ("classic.pad.five-voice-halo", -10.0f);
    p.osc1.shape = 0.0f; p.osc1.level = 0.43f; p.osc1.fineCents = -6.0f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.43f; p.osc2.fineCents = 7.0f;
    p.filter.model = FilterModel::Ladder4P;
    p.filter.cutoffHz = 2500.0f; p.filter.resonance = 0.16f;
    p.filter.envAmount = 0.7f; p.filter.keyTrack = 0.2f;
    envelope (p.filterEnv, 400.0f, 1600.0f, 0.50f, 2500.0f, 0.5f);
    envelope (p.ampEnv, 900.0f, 1800.0f, 0.82f, 2800.0f, 0.42f);
    lfo (p.lfo1, LfoShape::Triangle, 0.17f);
    route (p, 0, ModSource::Lfo1, ModDest::Pitch, 0.045f);
    route (p, 1, ModSource::Lfo1, ModDest::Pan, 0.15f);
    p.osc1.drift = 0.22f; p.osc2.drift = 0.25f;
    setDelay (p, 410.0f, 0.26f, 4800.0f, 0.16f);
    setReverb (p, 0.68f, 0.58f, 5400.0f, 20.0f, 0.25f);
    return p;
}

SynthPatch makeSunsetPad()
{
    auto p = basePatch ("classic.pad.sunset-pad", -10.0f);
    p.osc1.shape = 0.33f; p.osc1.pulseWidth = 0.46f; p.osc1.level = 0.48f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.40f; p.osc2.fineCents = 8.0f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 1500.0f; p.filter.resonance = 0.20f;
    p.filter.envAmount = 0.9f; p.filter.keyTrack = 0.22f;
    envelope (p.filterEnv, 500.0f, 1400.0f, 0.50f, 2000.0f, 0.5f);
    envelope (p.ampEnv, 700.0f, 1500.0f, 0.80f, 2400.0f, 0.42f);
    lfo (p.lfo1, LfoShape::Sine, 0.21f);
    route (p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.200f);
    route (p, 1, ModSource::Lfo1, ModDest::Pan, 0.22f);
    p.osc1.drift = 0.30f;
    setChorus (p, 3, 0.52f, 0.30f);
    setReverb (p, 0.55f, 0.50f, 6000.0f, 14.0f, 0.17f);
    return p;
}

SynthPatch makeAuroraGlass()
{
    auto p = basePatch ("modern.pad.aurora-glass", -12.0f);
    p.osc1.wavetable = true; p.osc1.wavetablePos = 0.35f; p.osc1.level = 0.45f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.40f; p.osc2.fineCents = 9.0f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 2400.0f; p.filter.resonance = 0.22f;
    p.filter.envAmount = 1.4f; p.filter.keyTrack = 0.20f;
    envelope (p.filterEnv, 800.0f, 1800.0f, 0.45f, 3000.0f, 0.5f);
    envelope (p.ampEnv, 1400.0f, 2000.0f, 0.90f, 4200.0f, 0.38f);
    lfo (p.lfo1, LfoShape::Sine, 0.12f);
    route (p, 0, ModSource::Lfo1, ModDest::WavetablePos, 0.25f);
    route (p, 1, ModSource::Lfo1, ModDest::Cutoff, 0.15f);
    route (p, 2, ModSource::Aftertouch, ModDest::Cutoff, 0.35f);
    p.voiceMode = VoiceMode::Unison;
    p.unisonVoices = 5; p.unisonDetune = 12.0f; p.unisonSpread = 0.75f;
    p.osc1.drift = 0.28f; p.osc2.drift = 0.26f;
    setChorus (p, 3, 0.55f, 0.30f);
    setDelay (p, 520.0f, 0.30f, 5000.0f, 0.18f);
    setReverb (p, 0.78f, 0.72f, 6200.0f, 24.0f, 0.30f);
    return p;
}

SynthPatch makeFrozenBloom()
{
    auto p = basePatch ("modern.pad.frozen-bloom", -13.0f);
    p.osc1.wavetable = true; p.osc1.wavetablePos = 0.68f; p.osc1.level = 0.40f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.22f; p.osc2.fineCents = 12.0f;
    p.noiseLevel = 0.15f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 1700.0f; p.filter.resonance = 0.20f;
    p.filter.envAmount = 1.65f; p.filter.highPassHz = 160.0f;
    envelope (p.filterEnv, 1400.0f, 2200.0f, 0.35f, 4000.0f, 0.55f);
    envelope (p.ampEnv, 1600.0f, 2200.0f, 0.78f, 5200.0f, 0.38f);
    lfo (p.lfo1, LfoShape::Triangle, 0.08f);
    route (p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.22f);
    route (p, 1, ModSource::Lfo1, ModDest::WavetablePos, 0.18f);
    p.osc1.drift = 0.32f;
    setChorus (p, 2, 0.35f, 0.18f);
    setDelay (p, 500.0f, 0.38f, 4200.0f, 0.20f, true, true, 1.0f);
    setReverb (p, 0.82f, 0.75f, 7000.0f, 30.0f, 0.34f);
    return p;
}

SynthPatch makeIridescentChoir()
{
    auto p = basePatch ("modern.pad.iridescent-choir", -12.0f);
    p.osc1.wavetable = true; p.osc1.wavetablePos = 0.42f; p.osc1.level = 0.38f;
    p.osc2.wavetable = true; p.osc2.wavetablePos = 0.72f; p.osc2.level = 0.34f;
    p.osc2.fineCents = -9.0f;
    p.noiseLevel = 0.06f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 2100.0f; p.filter.resonance = 0.24f;
    p.filter.envAmount = 0.8f; p.filter.highPassHz = 120.0f;
    envelope (p.filterEnv, 1200.0f, 2000.0f, 0.55f, 3500.0f, 0.5f);
    envelope (p.ampEnv, 900.0f, 2400.0f, 0.88f, 5000.0f, 0.4f);
    lfo (p.lfo1, LfoShape::Sine, 0.09f);
    route (p, 0, ModSource::Lfo1, ModDest::WavetablePos, 0.20f);
    route (p, 1, ModSource::Aftertouch, ModDest::Cutoff, 0.42f);
    route (p, 2, ModSource::Lfo1, ModDest::Pan, 0.18f);
    p.osc1.drift = 0.33f;
    setChorus (p, 3, 0.45f, 0.32f);
    setReverb (p, 0.90f, 0.82f, 6000.0f, 35.0f, 0.38f, 0.95f);
    return p;
}

//==============================================================================
SynthPatch makeFiveVoiceKeys()
{
    auto p = basePatch ("classic.keys.five-voice-keys", -7.0f);
    p.osc1.shape = 0.0f; p.osc1.level = 0.50f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.50f; p.osc2.fineCents = -6.0f;
    p.filter.model = FilterModel::Ladder4P;
    p.filter.cutoffHz = 3000.0f; p.filter.resonance = 0.12f;
    p.filter.envAmount = 0.9f; p.filter.keyTrack = 0.50f;
    envelope (p.filterEnv, 2.0f, 500.0f, 0.25f, 450.0f, 0.65f);
    envelope (p.ampEnv, 3.0f, 600.0f, 0.30f, 500.0f, 0.55f, 0.15f);
    p.osc1.drift = 0.15f; p.osc2.drift = 0.18f;
    setDelay (p, 320.0f, 0.18f, 5600.0f, 0.10f);
    setReverb (p, 0.5f, 0.45f, 7000.0f, 12.0f, 0.12f);
    return p;
}

SynthPatch makeEightVoiceOrgan()
{
    auto p = basePatch ("classic.keys.eight-voice-organ", -8.0f);
    p.osc1.shape = 0.33f; p.osc1.pulseWidth = 0.5f; p.osc1.level = 0.46f;
    p.osc2.shape = 0.66f; p.osc2.level = 0.24f;
    p.subLevel = 0.30f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 4200.0f; p.filter.resonance = 0.10f;
    p.filter.envAmount = 0.0f;
    envelope (p.filterEnv, 1.0f, 100.0f, 0.0f, 100.0f);
    envelope (p.ampEnv, 3.0f, 100.0f, 0.95f, 180.0f, 0.45f);
    setChorus (p, 1, 0.30f, 0.25f);
    setReverb (p, 0.35f, 0.3f, 6000.0f, 8.0f, 0.08f);
    return p;
}

SynthPatch make106Keys()
{
    auto p = basePatch ("classic.keys.106-keys", -8.0f);
    p.osc1.shape = 0.33f; p.osc1.pulseWidth = 0.44f; p.osc1.level = 0.52f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.18f; p.osc2.fineCents = 6.0f;
    p.subLevel = 0.24f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 2600.0f; p.filter.resonance = 0.16f;
    p.filter.envAmount = 0.65f; p.filter.keyTrack = 0.3f;
    envelope (p.filterEnv, 80.0f, 700.0f, 0.40f, 800.0f, 0.5f);
    envelope (p.ampEnv, 20.0f, 800.0f, 0.60f, 800.0f, 0.45f, 0.12f);
    lfo (p.lfo1, LfoShape::Sine, 0.22f);
    route (p, 0, ModSource::Lfo1, ModDest::PulseWidth1, 0.18f);
    setChorus (p, 1, 0.40f, 0.30f);
    setReverb (p, 0.45f, 0.4f, 6500.0f, 10.0f, 0.10f);
    return p;
}

SynthPatch makeGlassKeys()
{
    auto p = basePatch ("modern.keys.glass-keys", -7.0f);
    p.osc1.wavetable = true; p.osc1.wavetablePos = 0.60f; p.osc1.level = 0.48f;
    p.osc2.shape = 0.66f; p.osc2.level = 0.22f; p.osc2.fineCents = 5.0f;
    p.noiseLevel = 0.07f;
    p.filter.model = FilterModel::Ladder4P;
    p.filter.cutoffHz = 2600.0f; p.filter.resonance = 0.20f;
    p.filter.envAmount = 1.3f; p.filter.keyTrack = 0.35f;
    envelope (p.filterEnv, 1.0f, 400.0f, 0.10f, 300.0f, 0.7f);
    envelope (p.ampEnv, 2.0f, 900.0f, 0.20f, 600.0f, 0.55f, 0.12f);
    setDelay (p, 280.0f, 0.20f, 6200.0f, 0.15f);
    setReverb (p, 0.55f, 0.52f, 7000.0f, 12.0f, 0.22f);
    return p;
}

SynthPatch makeNeonClav()
{
    auto p = basePatch ("modern.keys.neon-clav", -6.0f);
    p.osc1.shape = 0.33f; p.osc1.pulseWidth = 0.18f; p.osc1.level = 0.52f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.26f; p.osc2.fineCents = 4.0f;
    p.filter.model = FilterModel::Ladder4P;
    p.filter.cutoffHz = 2200.0f; p.filter.resonance = 0.35f;
    p.filter.envAmount = 2.4f; p.filter.keyTrack = 0.4f; p.filter.highPassHz = 220.0f;
    envelope (p.filterEnv, 1.0f, 160.0f, 0.05f, 120.0f, 0.8f);
    envelope (p.ampEnv, 1.0f, 180.0f, 0.05f, 160.0f, 0.75f, 0.2f);
    setPhaser (p, 4, 0.30f, 0.55f, 0.22f, 0.15f);
    setDelay (p, 240.0f, 0.18f, 6400.0f, 0.08f);
    return p;
}

SynthPatch makeMonoNoire()
{
    auto p = basePatch ("modern.keys.mono-noire", -7.0f);
    p.osc1.shape = 0.66f; p.osc1.level = 0.46f;
    p.osc2.shape = 1.0f; p.osc2.level = 0.30f; p.osc2.fineCents = -5.0f;
    p.noiseLevel = 0.035f;
    p.filter.model = FilterModel::Ladder4P;
    p.filter.cutoffHz = 900.0f; p.filter.resonance = 0.12f;
    p.filter.envAmount = 0.4f; p.filter.keyTrack = 0.35f;
    envelope (p.filterEnv, 20.0f, 1200.0f, 0.35f, 1800.0f, 0.5f);
    envelope (p.ampEnv, 25.0f, 1800.0f, 0.40f, 2200.0f, 0.45f, 0.18f);
    p.voiceMode = VoiceMode::Mono;
    p.glideMs = 40.0f;
    route (p, 0, ModSource::Aftertouch, ModDest::Pitch, 0.03f);
    route (p, 1, ModSource::ModWheel, ModDest::Cutoff, 0.30f);
    setReverb (p, 0.68f, 0.6f, 5800.0f, 18.0f, 0.30f);
    return p;
}

//==============================================================================
SynthPatch makeFiveVoiceSolo()
{
    auto p = basePatch ("classic.lead.five-voice-solo", -6.0f);
    p.osc1.shape = 0.0f; p.osc1.level = 0.48f;
    p.osc2.shape = 0.33f; p.osc2.pulseWidth = 0.44f; p.osc2.level = 0.35f;
    p.osc2.fineCents = 5.0f;
    p.filter.model = FilterModel::Ladder4P;
    p.filter.cutoffHz = 4000.0f; p.filter.resonance = 0.18f;
    p.filter.envAmount = 1.0f; p.filter.keyTrack = 0.4f;
    envelope (p.filterEnv, 8.0f, 600.0f, 0.35f, 500.0f, 0.6f);
    envelope (p.ampEnv, 10.0f, 600.0f, 0.75f, 450.0f, 0.45f, 0.25f);
    p.voiceMode = VoiceMode::Legato;
    p.glideMs = 80.0f;
    p.modWheelDepth = 0.55f;
    lfo (p.lfo1, LfoShape::Sine, 5.2f, 250.0f);
    route (p, 0, ModSource::Lfo1, ModDest::Pitch, 0.035f);
    route (p, 1, ModSource::ModWheel, ModDest::Pitch, 0.32f);
    route (p, 2, ModSource::ModWheel, ModDest::Cutoff, 0.12f);
    p.osc1.drift = 0.14f;
    setDelay (p, 340.0f, 0.25f, 5200.0f, 0.15f);
    setReverb (p, 0.5f, 0.45f, 6500.0f, 12.0f, 0.10f);
    return p;
}

SynthPatch make106Lead()
{
    auto p = basePatch ("classic.lead.106-lead", -6.0f);
    p.osc1.shape = 0.33f; p.osc1.pulseWidth = 0.40f; p.osc1.level = 0.55f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.20f; p.osc2.fineCents = 7.0f;
    p.subLevel = 0.22f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 3500.0f; p.filter.resonance = 0.20f;
    p.filter.envAmount = 1.2f; p.filter.keyTrack = 0.3f;
    envelope (p.filterEnv, 5.0f, 500.0f, 0.40f, 450.0f, 0.6f);
    envelope (p.ampEnv, 8.0f, 600.0f, 0.70f, 500.0f, 0.45f, 0.2f);
    p.voiceMode = VoiceMode::Mono;
    p.glideMs = 45.0f;
    lfo (p.lfo1, LfoShape::Sine, 5.5f, 300.0f);
    route (p, 0, ModSource::Lfo1, ModDest::Pitch, 0.025f);
    setChorus (p, 1, 0.35f, 0.22f);
    setDelay (p, 300.0f, 0.22f, 5400.0f, 0.10f);
    return p;
}

SynthPatch makeEightVoiceSync()
{
    auto p = basePatch ("classic.lead.eight-voice-sync", -7.0f);
    p.osc1.shape = 0.0f; p.osc1.level = 0.45f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.40f; p.osc2.sync = true;
    p.osc2.semitone = 7.0f;
    p.filter.model = FilterModel::Ladder4P;
    p.filter.cutoffHz = 2800.0f; p.filter.resonance = 0.25f;
    p.filter.envAmount = 2.5f; p.filter.keyTrack = 0.3f;
    envelope (p.filterEnv, 2.0f, 500.0f, 0.30f, 400.0f, 0.75f);
    envelope (p.ampEnv, 4.0f, 500.0f, 0.75f, 380.0f, 0.5f, 0.25f);
    p.voiceMode = VoiceMode::Mono;
    p.glideMs = 25.0f;
    p.drive = 0.25f;
    route (p, 0, ModSource::Velocity, ModDest::Cutoff, 0.22f);
    setDelay (p, 300.0f, 0.2f, 4800.0f, 0.08f);
    return p;
}

SynthPatch makeLaserBloom()
{
    auto p = basePatch ("modern.lead.laser-bloom", -10.0f);
    p.osc1.wavetable = true; p.osc1.wavetablePos = 0.78f; p.osc1.level = 0.44f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.38f; p.osc2.fineCents = 8.0f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 3200.0f; p.filter.resonance = 0.42f;
    p.filter.envAmount = 1.5f; p.filter.keyTrack = 0.3f;
    envelope (p.filterEnv, 4.0f, 450.0f, 0.3f, 400.0f, 0.65f);
    envelope (p.ampEnv, 6.0f, 600.0f, 0.7f, 450.0f, 0.45f, 0.2f);
    p.voiceMode = VoiceMode::Unison;
    p.unisonVoices = 5; p.unisonDetune = 9.0f; p.unisonSpread = 0.8f;
    setPhaser (p, 4, 0.24f, 0.55f, 0.24f, 0.16f);
    setDelay (p, 360.0f, 0.30f, 6000.0f, 0.18f);
    setReverb (p, 0.55f, 0.5f, 6500.0f, 14.0f, 0.16f);
    lfo (p.lfo1, LfoShape::Sine, 0.35f);
    route (p, 0, ModSource::Lfo1, ModDest::WavetablePos, 0.18f);
    return p;
}

SynthPatch makeChromeSiren()
{
    auto p = basePatch ("modern.lead.chrome-siren", -9.0f);
    p.osc1.wavetable = true; p.osc1.wavetablePos = 0.66f; p.osc1.level = 0.42f;
    p.osc2.shape = 0.33f; p.osc2.pulseWidth = 0.3f; p.osc2.level = 0.32f;
    p.ringLevel = 0.20f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 3000.0f; p.filter.resonance = 0.3f;
    p.filter.envAmount = 1.2f;
    envelope (p.filterEnv, 30.0f, 800.0f, 0.45f, 600.0f, 0.55f);
    envelope (p.ampEnv, 15.0f, 900.0f, 0.75f, 700.0f, 0.45f, 0.2f);
    p.voiceMode = VoiceMode::Mono;
    p.glideMs = 90.0f;
    lfo (p.lfo1, LfoShape::Sine, 0.28f);
    route (p, 0, ModSource::Lfo1, ModDest::Pitch, 0.09f);
    route (p, 1, ModSource::Lfo1, ModDest::WavetablePos, 0.24f);
    route (p, 2, ModSource::ModWheel, ModDest::Pitch, 0.22f);
    setDelay (p, 400.0f, 0.35f, 5200.0f, 0.20f, true, true, 0.75f);
    setReverb (p, 0.65f, 0.58f, 6000.0f, 18.0f, 0.18f);
    return p;
}

SynthPatch makeHyperLead()
{
    auto p = basePatch ("modern.lead.hyper-lead", -9.0f);
    p.osc1.shape = 0.0f; p.osc1.level = 0.44f;
    p.osc2.shape = 0.33f; p.osc2.pulseWidth = 0.5f; p.osc2.level = 0.38f;
    p.osc2.fineCents = 7.0f;
    p.filter.model = FilterModel::Ladder4P;
    p.filter.cutoffHz = 3800.0f; p.filter.resonance = 0.22f;
    p.filter.envAmount = 1.1f; p.filter.keyTrack = 0.3f;
    envelope (p.filterEnv, 6.0f, 500.0f, 0.4f, 450.0f, 0.6f);
    envelope (p.ampEnv, 8.0f, 600.0f, 0.75f, 450.0f, 0.45f, 0.2f);
    p.voiceMode = VoiceMode::Unison;
    p.unisonVoices = 3; p.unisonDetune = 10.0f; p.unisonSpread = 0.7f;
    p.glideMs = 60.0f;
    p.drive = 0.28f;
    setChorus (p, 1, 0.35f, 0.20f);
    setDelay (p, 320.0f, 0.25f, 5600.0f, 0.15f);
    return p;
}

//==============================================================================
SynthPatch make106Pluck()
{
    auto p = basePatch ("classic.pluck.106-pluck", -7.0f);
    p.osc1.shape = 0.33f; p.osc1.pulseWidth = 0.4f; p.osc1.level = 0.55f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.16f; p.osc2.fineCents = 6.0f;
    p.subLevel = 0.20f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 1800.0f; p.filter.resonance = 0.22f;
    p.filter.envAmount = 2.0f; p.filter.keyTrack = 0.35f;
    envelope (p.filterEnv, 1.0f, 220.0f, 0.05f, 180.0f, 0.8f);
    envelope (p.ampEnv, 1.0f, 450.0f, 0.10f, 280.0f, 0.7f);
    setChorus (p, 1, 0.35f, 0.25f);
    setDelay (p, 300.0f, 0.22f, 5400.0f, 0.16f);
    return p;
}

SynthPatch makeFiveVoiceArp()
{
    auto p = basePatch ("classic.pluck.five-voice-arp", -8.0f);
    p.osc1.shape = 0.0f; p.osc1.level = 0.44f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.40f; p.osc2.fineCents = 6.0f;
    p.filter.model = FilterModel::Ladder4P;
    p.filter.cutoffHz = 2600.0f; p.filter.resonance = 0.2f;
    p.filter.envAmount = 1.5f; p.filter.keyTrack = 0.3f;
    envelope (p.filterEnv, 1.0f, 220.0f, 0.2f, 180.0f, 0.7f);
    envelope (p.ampEnv, 1.0f, 180.0f, 0.0f, 120.0f, 0.7f);
    setArp (p, true, 4, 1, 0, 0.35f);
    setDelay (p, 320.0f, 0.22f, 5000.0f, 0.18f);
    setReverb (p, 0.35f, 0.35f, 6000.0f, 8.0f, 0.05f);
    return p;
}

SynthPatch makeSixVoiceBells()
{
    auto p = basePatch ("classic.pluck.six-voice-bells", -8.0f);
    p.osc1.shape = 1.0f; p.osc1.level = 0.42f;
    p.osc2.shape = 0.66f; p.osc2.level = 0.26f; p.osc2.fineCents = 9.0f;
    p.ringLevel = 0.26f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 3400.0f; p.filter.resonance = 0.18f;
    p.filter.envAmount = 0.8f; p.filter.highPassHz = 380.0f;
    envelope (p.filterEnv, 2.0f, 800.0f, 0.15f, 1200.0f, 0.6f);
    envelope (p.ampEnv, 2.0f, 1200.0f, 0.10f, 1800.0f, 0.65f);
    setArp (p, true, 4, 1, 3, 0.25f);
    setReverb (p, 0.78f, 0.7f, 6500.0f, 20.0f, 0.35f);
    return p;
}

SynthPatch makeGlassDrop()
{
    auto p = basePatch ("modern.pluck.glass-drop", -6.0f);
    p.osc1.wavetable = true; p.osc1.wavetablePos = 0.48f; p.osc1.level = 0.48f;
    p.osc2.shape = 0.66f; p.osc2.level = 0.18f; p.osc2.fineCents = 12.0f;
    p.filter.model = FilterModel::Ladder4P;
    p.filter.cutoffHz = 2800.0f; p.filter.resonance = 0.3f;
    p.filter.envAmount = 1.8f;
    envelope (p.filterEnv, 1.0f, 260.0f, 0.0f, 220.0f, 0.8f);
    envelope (p.ampEnv, 1.0f, 500.0f, 0.0f, 600.0f, 0.7f);
    route (p, 0, ModSource::FilterEnv, ModDest::Pitch, 0.12f);
    route (p, 1, ModSource::Velocity, ModDest::Cutoff, 0.15f);
    setDelay (p, 360.0f, 0.24f, 6400.0f, 0.16f);
    setReverb (p, 0.65f, 0.62f, 7000.0f, 14.0f, 0.28f);
    return p;
}

SynthPatch makeCrystalArp()
{
    auto p = basePatch ("modern.pluck.crystal-arp", -8.0f);
    p.osc1.wavetable = true; p.osc1.wavetablePos = 0.62f; p.osc1.level = 0.46f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.24f; p.osc2.fineCents = 9.0f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 3200.0f; p.filter.resonance = 0.26f;
    p.filter.envAmount = 1.2f;
    envelope (p.filterEnv, 1.0f, 300.0f, 0.2f, 260.0f, 0.7f);
    envelope (p.ampEnv, 1.0f, 250.0f, 0.05f, 300.0f, 0.65f);
    setArp (p, true, 4, 2, 2, 0.30f);
    setChorus (p, 2, 0.32f, 0.20f);
    setDelay (p, 360.0f, 0.28f, 6500.0f, 0.20f, true, true, 0.5f);
    setReverb (p, 0.6f, 0.55f, 7200.0f, 12.0f, 0.18f);
    return p;
}

SynthPatch makeKineticPluck()
{
    auto p = basePatch ("modern.pluck.kinetic-pluck", -8.0f);
    p.osc1.shape = 0.33f; p.osc1.pulseWidth = 0.32f; p.osc1.level = 0.46f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.22f; p.osc2.fineCents = 8.0f;
    p.noiseLevel = 0.10f;
    p.filter.model = FilterModel::Ladder4P;
    p.filter.cutoffHz = 1900.0f; p.filter.resonance = 0.3f;
    p.filter.envAmount = 2.0f; p.filter.drive = 0.12f;
    envelope (p.filterEnv, 1.0f, 180.0f, 0.05f, 150.0f, 0.8f);
    envelope (p.ampEnv, 1.0f, 250.0f, 0.0f, 180.0f, 0.7f);
    setArp (p, true, 8, 1, 0, 0.35f);
    setPhaser (p, 4, 0.42f, 0.5f, 0.2f, 0.14f);
    p.drive = 0.18f;
    setDelay (p, 260.0f, 0.28f, 5000.0f, 0.18f);
    return p;
}

//==============================================================================
SynthPatch makeFiveVoiceBrass()
{
    auto p = basePatch ("classic.brass.five-voice-brass", -7.0f);
    p.osc1.shape = 0.0f; p.osc1.level = 0.5f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.5f; p.osc2.fineCents = 5.0f;
    p.filter.model = FilterModel::Ladder4P;
    p.filter.cutoffHz = 2000.0f; p.filter.resonance = 0.15f;
    p.filter.envAmount = 2.0f; p.filter.velocity = 0.5f; p.filter.keyTrack = 0.35f;
    envelope (p.filterEnv, 60.0f, 500.0f, 0.40f, 500.0f, 0.6f);
    envelope (p.ampEnv, 20.0f, 400.0f, 0.80f, 600.0f, 0.45f, 0.25f);
    p.osc1.drift = 0.2f; p.osc2.drift = 0.2f;
    setReverb (p, 0.5f, 0.45f, 6500.0f, 12.0f, 0.10f);
    return p;
}

SynthPatch makeEightVoiceBrass()
{
    auto p = basePatch ("classic.brass.eight-voice-brass", -8.0f);
    p.osc1.shape = 0.33f; p.osc1.pulseWidth = 0.38f; p.osc1.level = 0.46f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.42f; p.osc2.fineCents = 7.0f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 2400.0f; p.filter.resonance = 0.18f;
    p.filter.envAmount = 1.8f; p.filter.velocity = 0.35f; p.filter.keyTrack = 0.3f;
    envelope (p.filterEnv, 40.0f, 450.0f, 0.45f, 450.0f, 0.6f);
    envelope (p.ampEnv, 15.0f, 400.0f, 0.80f, 550.0f, 0.45f, 0.2f);
    setChorus (p, 3, 0.48f, 0.28f);
    setReverb (p, 0.55f, 0.48f, 6200.0f, 14.0f, 0.14f);
    return p;
}

SynthPatch make106Strings()
{
    auto p = basePatch ("classic.brass.106-strings", -10.0f);
    p.osc1.shape = 0.0f; p.osc1.level = 0.44f;
    p.osc2.shape = 0.33f; p.osc2.pulseWidth = 0.45f; p.osc2.level = 0.28f;
    p.osc2.fineCents = 6.0f;
    p.subLevel = 0.16f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 2200.0f; p.filter.resonance = 0.16f;
    p.filter.envAmount = 0.8f; p.filter.highPassHz = 100.0f;
    envelope (p.filterEnv, 600.0f, 1500.0f, 0.55f, 2200.0f, 0.5f);
    envelope (p.ampEnv, 800.0f, 1800.0f, 0.82f, 2800.0f, 0.42f);
    lfo (p.lfo1, LfoShape::Sine, 0.18f);
    route (p, 0, ModSource::Lfo1, ModDest::Pan, 0.18f);
    setChorus (p, 2, 0.55f, 0.35f);
    setReverb (p, 0.6f, 0.54f, 5800.0f, 16.0f, 0.16f);
    return p;
}

SynthPatch makeNeonBrass()
{
    auto p = basePatch ("modern.brass.neon-brass", -8.0f);
    p.osc1.wavetable = true; p.osc1.wavetablePos = 0.4f; p.osc1.level = 0.44f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.38f; p.osc2.fineCents = 8.0f;
    p.filter.model = FilterModel::Ladder4P;
    p.filter.cutoffHz = 2400.0f; p.filter.resonance = 0.2f;
    p.filter.envAmount = 1.9f; p.filter.velocity = 0.35f; p.filter.drive = 0.12f;
    envelope (p.filterEnv, 12.0f, 500.0f, 0.4f, 500.0f, 0.65f);
    envelope (p.ampEnv, 10.0f, 500.0f, 0.75f, 500.0f, 0.45f, 0.2f);
    p.drive = 0.22f;
    setChorus (p, 1, 0.36f, 0.20f);
    setDelay (p, 360.0f, 0.25f, 5800.0f, 0.14f);
    setReverb (p, 0.5f, 0.45f, 6800.0f, 12.0f, 0.12f);
    return p;
}

SynthPatch makeVelvetStrings()
{
    auto p = basePatch ("modern.brass.velvet-strings", -11.0f);
    p.osc1.wavetable = true; p.osc1.wavetablePos = 0.32f; p.osc1.level = 0.40f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.34f; p.osc2.fineCents = 7.0f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 2000.0f; p.filter.resonance = 0.18f;
    p.filter.envAmount = 0.9f; p.filter.highPassHz = 90.0f;
    envelope (p.filterEnv, 700.0f, 1800.0f, 0.55f, 2600.0f, 0.5f);
    envelope (p.ampEnv, 1200.0f, 2000.0f, 0.85f, 4000.0f, 0.4f);
    route (p, 0, ModSource::Aftertouch, ModDest::Cutoff, 0.35f);
    route (p, 1, ModSource::Lfo1, ModDest::Pan, 0.15f);
    lfo (p.lfo1, LfoShape::Sine, 0.11f);
    setChorus (p, 3, 0.42f, 0.30f);
    setReverb (p, 0.72f, 0.65f, 6200.0f, 22.0f, 0.26f);
    return p;
}

SynthPatch makeChromeEnsemble()
{
    auto p = basePatch ("modern.brass.chrome-ensemble", -12.0f);
    p.osc1.wavetable = true; p.osc1.wavetablePos = 0.58f; p.osc1.level = 0.40f;
    p.osc2.wavetable = true; p.osc2.wavetablePos = 0.76f; p.osc2.level = 0.34f;
    p.osc2.fineCents = -8.0f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 2800.0f; p.filter.resonance = 0.22f;
    p.filter.envAmount = 1.0f; p.filter.highPassHz = 110.0f;
    envelope (p.filterEnv, 500.0f, 1500.0f, 0.5f, 2200.0f, 0.5f);
    envelope (p.ampEnv, 900.0f, 1800.0f, 0.85f, 3600.0f, 0.4f);
    p.voiceMode = VoiceMode::Unison;
    p.unisonVoices = 4; p.unisonDetune = 10.0f; p.unisonSpread = 0.85f;
    setChorus (p, 3, 0.55f, 0.34f);
    setPhaser (p, 4, 0.14f, 0.4f, 0.18f, 0.12f);
    setReverb (p, 0.7f, 0.62f, 6200.0f, 20.0f, 0.22f);
    return p;
}

//==============================================================================
SynthPatch make106Drift()
{
    auto p = basePatch ("classic.texture.106-drift", -12.0f);
    p.osc1.shape = 0.33f; p.osc1.pulseWidth = 0.46f; p.osc1.level = 0.44f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.3f; p.osc2.fineCents = 8.0f;
    p.subLevel = 0.15f; p.noiseLevel = 0.04f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 1600.0f; p.filter.resonance = 0.22f;
    p.filter.envAmount = 0.8f; p.filter.highPassHz = 80.0f;
    envelope (p.filterEnv, 1200.0f, 2200.0f, 0.55f, 4000.0f, 0.5f);
    envelope (p.ampEnv, 1800.0f, 2600.0f, 0.88f, 6000.0f, 0.38f);
    lfo (p.lfo1, LfoShape::Sine, 0.06f);
    route (p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.26f);
    route (p, 1, ModSource::Lfo1, ModDest::Pan, 0.32f);
    route (p, 2, ModSource::Lfo1, ModDest::PulseWidth1, 0.18f);
    p.osc1.drift = 0.42f; p.osc2.drift = 0.42f;
    setChorus (p, 2, 0.6f, 0.38f);
    setReverb (p, 0.9f, 0.82f, 6000.0f, 32.0f, 0.38f, 0.95f);
    return p;
}

SynthPatch makeFiveVoiceWind()
{
    auto p = basePatch ("classic.texture.five-voice-wind", -13.0f);
    p.osc1.shape = 0.0f; p.osc1.level = 0.18f;
    p.osc2.level = 0.0f;
    p.noiseLevel = 0.42f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.type = FilterType::BandPass;
    p.filter.cutoffHz = 1200.0f; p.filter.resonance = 0.38f;
    p.filter.envAmount = 1.2f; p.filter.highPassHz = 120.0f;
    envelope (p.filterEnv, 1600.0f, 2400.0f, 0.45f, 3600.0f, 0.5f);
    envelope (p.ampEnv, 1500.0f, 2200.0f, 0.85f, 5000.0f, 0.4f);
    lfo (p.lfo1, LfoShape::Sine, 0.08f);
    lfo (p.lfo2, LfoShape::Triangle, 0.13f);
    route (p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.4f);
    route (p, 1, ModSource::Lfo2, ModDest::Pan, 0.3f);
    route (p, 2, ModSource::Lfo1, ModDest::Amp, 0.12f);
    setPhaser (p, 6, 0.08f, 0.6f, 0.22f, 0.20f);
    setReverb (p, 0.95f, 0.85f, 5500.0f, 40.0f, 0.42f, 0.98f);
    return p;
}

SynthPatch makeEightVoiceChoir()
{
    auto p = basePatch ("classic.texture.eight-voice-choir", -11.0f);
    p.osc1.shape = 0.33f; p.osc1.pulseWidth = 0.47f; p.osc1.level = 0.4f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.32f; p.osc2.fineCents = 7.0f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 1900.0f; p.filter.resonance = 0.2f;
    p.filter.envAmount = 0.8f; p.filter.highPassHz = 130.0f;
    envelope (p.filterEnv, 900.0f, 1800.0f, 0.5f, 3200.0f, 0.5f);
    envelope (p.ampEnv, 1100.0f, 2200.0f, 0.85f, 4200.0f, 0.4f);
    setChorus (p, 3, 0.5f, 0.35f);
    setReverb (p, 0.92f, 0.82f, 5600.0f, 30.0f, 0.36f, 0.98f);
    return p;
}

SynthPatch makeDeepSpace()
{
    auto p = basePatch ("modern.texture.deep-space", -14.0f);
    p.osc1.wavetable = true; p.osc1.wavetablePos = 0.24f; p.osc1.level = 0.36f;
    p.osc2.shape = 1.0f; p.osc2.level = 0.2f; p.osc2.octave = -1.0f;
    p.subLevel = 0.25f;
    p.filter.model = FilterModel::Ladder4P;
    p.filter.cutoffHz = 900.0f; p.filter.resonance = 0.2f;
    p.filter.envAmount = 1.2f; p.filter.highPassHz = 45.0f;
    envelope (p.filterEnv, 2200.0f, 3000.0f, 0.5f, 5000.0f, 0.5f);
    envelope (p.ampEnv, 2500.0f, 3000.0f, 0.9f, 6000.0f, 0.35f);
    lfo (p.lfo1, LfoShape::Sine, 0.05f);
    route (p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.3f);
    route (p, 1, ModSource::Lfo1, ModDest::WavetablePos, 0.2f);
    route (p, 2, ModSource::Lfo1, ModDest::Pan, 0.2f);
    setDelay (p, 620.0f, 0.4f, 3800.0f, 0.22f);
    setReverb (p, 0.98f, 0.9f, 5200.0f, 45.0f, 0.44f, 1.0f);
    return p;
}

SynthPatch makeGlassRain()
{
    auto p = basePatch ("modern.texture.glass-rain", -10.0f);
    p.osc1.wavetable = true; p.osc1.wavetablePos = 0.72f; p.osc1.level = 0.38f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.18f; p.osc2.fineCents = 11.0f;
    p.noiseLevel = 0.12f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 3400.0f; p.filter.resonance = 0.3f;
    p.filter.envAmount = 1.2f; p.filter.highPassHz = 260.0f;
    envelope (p.filterEnv, 2.0f, 700.0f, 0.1f, 900.0f, 0.7f);
    envelope (p.ampEnv, 5.0f, 1200.0f, 0.0f, 1600.0f, 0.65f);
    setArp (p, true, 8, 2, 3, 0.22f);
    setDelay (p, 420.0f, 0.4f, 6000.0f, 0.22f);
    setReverb (p, 0.85f, 0.78f, 7000.0f, 24.0f, 0.34f);
    return p;
}

SynthPatch makeSolsticeDrone()
{
    auto p = basePatch ("modern.texture.solstice-drone", -13.0f);
    p.osc1.wavetable = true; p.osc1.wavetablePos = 0.38f; p.osc1.level = 0.38f;
    p.osc2.wavetable = true; p.osc2.wavetablePos = 0.64f; p.osc2.level = 0.3f;
    p.osc2.fineCents = -7.0f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 1400.0f; p.filter.resonance = 0.22f;
    p.filter.envAmount = 1.0f; p.filter.highPassHz = 60.0f;
    envelope (p.filterEnv, 2400.0f, 3000.0f, 0.55f, 5000.0f, 0.45f);
    envelope (p.ampEnv, 2600.0f, 3000.0f, 0.9f, 6000.0f, 0.35f);
    p.voiceMode = VoiceMode::Unison;
    p.unisonVoices = 3; p.unisonDetune = 8.0f; p.unisonSpread = 0.65f;
    lfo (p.lfo1, LfoShape::Triangle, 0.07f);
    route (p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.24f);
    route (p, 1, ModSource::Lfo1, ModDest::Pan, 0.26f);
    setChorus (p, 2, 0.4f, 0.25f);
    setReverb (p, 0.92f, 0.84f, 5800.0f, 35.0f, 0.4f, 0.98f);
    return p;
}

//==============================================================================
SynthPatch make106Gate()
{
    auto p = basePatch ("classic.motion.106-gate", -9.0f);
    p.osc1.shape = 0.33f; p.osc1.pulseWidth = 0.43f; p.osc1.level = 0.46f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.3f; p.osc2.fineCents = 8.0f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 2000.0f; p.filter.resonance = 0.2f;
    p.filter.envAmount = 1.0f;
    envelope (p.filterEnv, 40.0f, 800.0f, 0.4f, 600.0f, 0.6f);
    envelope (p.ampEnv, 1.0f, 300.0f, 0.2f, 300.0f, 0.6f);
    lfo (p.lfo1, LfoShape::Triangle, 0.2f);
    route (p, 0, ModSource::Lfo1, ModDest::Pan, 0.2f);
    setArp (p, true, 4, 1, 0, 0.35f);
    setChorus (p, 2, 0.5f, 0.32f);
    setDelay (p, 300.0f, 0.28f, 5000.0f, 0.18f);
    return p;
}

SynthPatch makeFiveVoicePulse()
{
    auto p = basePatch ("classic.motion.five-voice-pulse", -8.0f);
    p.osc1.shape = 0.0f; p.osc1.level = 0.46f;
    p.osc2.shape = 0.33f; p.osc2.pulseWidth = 0.3f; p.osc2.level = 0.3f;
    p.osc2.fineCents = 6.0f;
    p.filter.model = FilterModel::Ladder4P;
    p.filter.cutoffHz = 2100.0f; p.filter.resonance = 0.24f;
    p.filter.envAmount = 1.3f;
    envelope (p.filterEnv, 20.0f, 700.0f, 0.4f, 600.0f, 0.6f);
    envelope (p.ampEnv, 10.0f, 600.0f, 0.7f, 500.0f, 0.45f);
    lfo (p.lfo1, LfoShape::Square, 3.2f, 0.0f, 0.0f, true, true, 0.5f);
    route (p, 0, ModSource::Lfo1, ModDest::Amp, 0.52f);
    route (p, 1, ModSource::Lfo1, ModDest::Cutoff, 0.12f);
    setDelay (p, 340.0f, 0.26f, 5200.0f, 0.16f);
    setReverb (p, 0.45f, 0.4f, 6000.0f, 10.0f, 0.08f);
    return p;
}

SynthPatch makeSixVoiceMotion()
{
    auto p = basePatch ("classic.motion.six-voice-motion", -10.0f);
    p.osc1.shape = 0.33f; p.osc1.pulseWidth = 0.44f; p.osc1.level = 0.42f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.3f; p.osc2.fineCents = 7.0f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 1800.0f; p.filter.resonance = 0.2f;
    p.filter.envAmount = 1.0f;
    envelope (p.filterEnv, 60.0f, 700.0f, 0.4f, 700.0f, 0.55f);
    envelope (p.ampEnv, 20.0f, 600.0f, 0.55f, 600.0f, 0.45f);
    lfo (p.lfo1, LfoShape::Triangle, 0.25f);
    route (p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.22f);
    setArp (p, true, 8, 2, 2, 0.45f);
    setChorus (p, 3, 0.42f, 0.3f);
    setReverb (p, 0.6f, 0.55f, 6200.0f, 16.0f, 0.16f);
    return p;
}

SynthPatch makeKineticGrid()
{
    auto p = basePatch ("modern.motion.kinetic-grid", -8.0f);
    p.osc1.wavetable = true; p.osc1.wavetablePos = 0.62f; p.osc1.level = 0.5f;
    p.osc2.shape = 0.33f; p.osc2.pulseWidth = 0.28f; p.osc2.level = 0.28f;
    p.filter.model = FilterModel::Ladder4P;
    p.filter.cutoffHz = 1400.0f; p.filter.resonance = 0.3f;
    p.filter.envAmount = 2.2f;
    envelope (p.filterEnv, 1.0f, 140.0f, 0.15f, 120.0f, 0.8f);
    envelope (p.ampEnv, 1.0f, 180.0f, 0.0f, 120.0f, 0.75f);
    setArp (p, true, 4, 2, 0, 0.35f);
    lfo (p.lfo1, LfoShape::Square, 2.0f, 0.0f, 0.0f, true, true, 0.5f);
    route (p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.12f);
    route (p, 1, ModSource::Velocity, ModDest::Cutoff, 0.18f);
    setPhaser (p, 4, 0.38f, 0.5f, 0.24f, 0.18f);
    setDelay (p, 300.0f, 0.28f, 5600.0f, 0.20f, true, true, 0.5f);
    setReverb (p, 0.55f, 0.5f, 6500.0f, 12.0f, 0.12f);
    return p;
}

SynthPatch makeNeonSequence()
{
    auto p = basePatch ("modern.motion.neon-sequence", -9.0f);
    p.osc1.wavetable = true; p.osc1.wavetablePos = 0.68f; p.osc1.level = 0.44f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.3f; p.osc2.fineCents = 7.0f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 2600.0f; p.filter.resonance = 0.24f;
    p.filter.envAmount = 1.4f;
    envelope (p.filterEnv, 1.0f, 240.0f, 0.15f, 220.0f, 0.75f);
    envelope (p.ampEnv, 1.0f, 260.0f, 0.05f, 280.0f, 0.7f);
    setArp (p, true, 4, 2, 0, 0.4f);
    setChorus (p, 2, 0.38f, 0.22f);
    setDelay (p, 360.0f, 0.3f, 6200.0f, 0.20f, true, true, 0.5f);
    setReverb (p, 0.55f, 0.5f, 7000.0f, 12.0f, 0.12f);
    return p;
}

SynthPatch makeTidalMotion()
{
    auto p = basePatch ("modern.motion.tidal-motion", -11.0f);
    p.osc1.wavetable = true; p.osc1.wavetablePos = 0.5f; p.osc1.level = 0.4f;
    p.osc2.shape = 0.0f; p.osc2.level = 0.28f; p.osc2.fineCents = 9.0f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 1800.0f; p.filter.resonance = 0.24f;
    p.filter.envAmount = 1.2f; p.filter.highPassHz = 80.0f;
    envelope (p.filterEnv, 300.0f, 1200.0f, 0.5f, 2200.0f, 0.55f);
    envelope (p.ampEnv, 200.0f, 1000.0f, 0.7f, 2500.0f, 0.45f);
    setArp (p, true, 8, 1, 1, 0.5f);
    lfo (p.lfo1, LfoShape::Sine, 0.07f);
    route (p, 0, ModSource::Lfo1, ModDest::Cutoff, 0.28f);
    route (p, 1, ModSource::Lfo1, ModDest::Pan, 0.3f);
    setDelay (p, 440.0f, 0.32f, 5000.0f, 0.2f);
    setReverb (p, 0.82f, 0.75f, 5800.0f, 26.0f, 0.3f);
    return p;
}

std::array<Preset, kNumPresets> buildBank()
{
    // Short-term loudness calibration, measured with the factory test phrase at
    // 48 kHz. These offsets are applied on top of each patch's gain staging so
    // the bank sits in a tight ±1.5 dB loudness window without a bus compressor.
    static constexpr std::array<float, kNumPresets> loudnessTrims {
        3.23f, 5.34f, 12.98f, 12.33f, 2.86f, 18.26f, 3.79f, 13.74f,
        5.10f, 13.06f, 17.98f, 16.06f, 8.94f, 2.06f, 3.73f, 16.71f,
        19.18f, 20.66f, 17.31f, 11.60f, 20.57f, 13.29f, 17.08f, 12.12f,
        5.46f, 22.27f, 20.58f, 17.56f, 26.03f, 23.69f, 7.63f, 5.20f,
        8.48f, 11.75f, 10.59f, 15.31f, 10.11f, 17.53f, 11.07f, 18.23f,
        26.60f, 12.93f, 19.58f, 10.10f, 15.31f, 25.63f, 28.52f, 23.13f
    };

    std::array<Preset, kNumPresets> bank {
        makePreset ("classic.bass.106-sub", "106 Sub", "Juno-106 sub bass",
            "A round mono sub with a pulse body and just enough filter snap.",
            { "Sub", "Mono", "Round", nullptr }, Group::Bass, Collection::Classic,
            artwork (Group::Bass, "classic.bass.106-sub", Motif::Prism, 0.38f, 0.68f, 0xff4fc3ff, 0xff6858ff),
            make106Sub()),
        makePreset ("classic.bass.five-voice-low", "Five Voice Low", "Prophet-5 bass",
            "Two detuned saws and a punchy ladder filter for warm analog bass.",
            { "Analog", "Punch", "Poly", nullptr }, Group::Bass, Collection::Classic,
            artwork (Group::Bass, "classic.bass.five-voice-low", Motif::Grid, 0.42f, 0.62f, 0xff65e0ff, 0xff786bff),
            makeFiveVoiceLow()),
        makePreset ("classic.bass.sunset-thump", "Sunset Thump", "OB-X bass",
            "A driven pulse-and-saw bass with a soft sunset edge and firm attack.",
            { "Warm", "Drive", "Mono", nullptr }, Group::Bass, Collection::Classic,
            artwork (Group::Bass, "classic.bass.sunset-thump", Motif::Tide, 0.45f, 0.60f, 0xff7fd5ff, 0xff9b6bff),
            makeSunsetThump()),
        makePreset ("modern.bass.glass-sub", "Glass Sub", "Modern clean sub",
            "A clean sine sub with a quiet wavetable layer to sit under a mix.",
            { "Sub", "Clean", "Modern", nullptr }, Group::Bass, Collection::Modern,
            artwork (Group::Bass, "modern.bass.glass-sub", Motif::Halo, 0.78f, 0.32f, 0xff71e6ff, 0xff6675ff),
            makeGlassSub()),
        makePreset ("modern.bass.neon-reese", "Neon Reese", "Modern Reese",
            "A dark, animated Reese with a wide unison spread and slow phase.",
            { "Reese", "Wide", "Dark", nullptr }, Group::Bass, Collection::Modern,
            artwork (Group::Bass, "modern.bass.neon-reese", Motif::Ribbon, 0.84f, 0.28f, 0xff3fbdff, 0xff8e5cff),
            makeNeonReese()),
        makePreset ("modern.bass.prism-bass", "Prism Bass", "Modern wavetable bass",
            "A bright wavetable bass with a narrow pulse edge for syncopated lines.",
            { "Wavetable", "Pluck", "Bright", nullptr }, Group::Bass, Collection::Modern,
            artwork (Group::Bass, "modern.bass.prism-bass", Motif::Prism, 0.76f, 0.35f, 0xff8ee8ff, 0xffa06bff),
            makePrismBass()),

        makePreset ("classic.pad.106-dream", "106 Dream", "Juno-106 pad",
            "A slow, chorus-rich pad inspired by the classic 1980s six-voice sound.",
            { "Lush", "Chorus", "Wide", nullptr }, Group::Pads, Collection::Classic,
            artwork (Group::Pads, "classic.pad.106-dream", Motif::Aurora, 0.48f, 0.72f, 0xffc7a6ff, 0xff63f5d6),
            make106Dream()),
        makePreset ("classic.pad.five-voice-halo", "Five Voice Halo", "Prophet-5 pad",
            "Two detuned saws drifting through a warm stereo halo.",
            { "Analog", "Warm", "Evolving", nullptr }, Group::Pads, Collection::Classic,
            artwork (Group::Pads, "classic.pad.five-voice-halo", Motif::Halo, 0.45f, 0.70f, 0xffd8b6ff, 0xff7ee7d2),
            makeFiveVoiceHalo()),
        makePreset ("classic.pad.sunset-pad", "Sunset Pad", "OB-X pad",
            "A soft pulse-and-saw pad moving slowly like late sunset light.",
            { "Retro", "Motion", "Warm", nullptr }, Group::Pads, Collection::Classic,
            artwork (Group::Pads, "classic.pad.sunset-pad", Motif::Ribbon, 0.52f, 0.66f, 0xffffa9d9, 0xffffd17d),
            makeSunsetPad()),
        makePreset ("modern.pad.aurora-glass", "Aurora Glass", "Modern wavetable pad",
            "A luminous wavetable pad that opens slowly into a wide glass aurora.",
            { "Glass", "Wide", "Cinematic", nullptr }, Group::Pads, Collection::Modern,
            artwork (Group::Pads, "modern.pad.aurora-glass", Motif::Aurora, 0.88f, 0.32f, 0xff8fd8ff, 0xffb596ff),
            makeAuroraGlass()),
        makePreset ("modern.pad.frozen-bloom", "Frozen Bloom", "Modern cold pad",
            "An icy, breathing pad of wavetable glass and filtered air.",
            { "Icy", "Slow", "Atmospheric", nullptr }, Group::Pads, Collection::Modern,
            artwork (Group::Pads, "modern.pad.frozen-bloom", Motif::Bloom, 0.80f, 0.40f, 0xffa7e4ff, 0xff8effd6),
            makeFrozenBloom()),
        makePreset ("modern.pad.iridescent-choir", "Iridescent Choir", "Modern spectral choir",
            "A spectral, vocal-adjacent pad floating in a very long glass reverb.",
            { "Choir", "Spectral", "Lush", nullptr }, Group::Pads, Collection::Modern,
            artwork (Group::Pads, "modern.pad.iridescent-choir", Motif::Caustic, 0.82f, 0.46f, 0xffd2a7ff, 0xff7dffd8),
            makeIridescentChoir()),

        makePreset ("classic.keys.five-voice-keys", "Five Voice Keys", "Prophet-5 keys",
            "The default: two saws through a bright ladder for a classic poly key.",
            { "Classic", "Poly", "Bright", nullptr }, Group::Keys, Collection::Classic,
            artwork (Group::Keys, "classic.keys.five-voice-keys", Motif::Prism, 0.45f, 0.66f, 0xfff6d9a8, 0xffa8d8ff),
            makeFiveVoiceKeys()),
        makePreset ("classic.keys.eight-voice-organ", "Eight Voice Organ", "OB-X organ",
            "A broad vintage organ stack with pulse, sub, and a light ensemble.",
            { "Organ", "Retro", "Ensemble", nullptr }, Group::Keys, Collection::Classic,
            artwork (Group::Keys, "classic.keys.eight-voice-organ", Motif::Grid, 0.40f, 0.68f, 0xffffd6a4, 0xffb1c8ff),
            makeEightVoiceOrgan()),
        makePreset ("classic.keys.106-keys", "106 Keys", "Juno-106 keys",
            "A soft, chorus-rich keyboard with a gentle pulse-width shimmer.",
            { "Analog", "Chorus", "Soft", nullptr }, Group::Keys, Collection::Classic,
            artwork (Group::Keys, "classic.keys.106-keys", Motif::Caustic, 0.46f, 0.70f, 0xffeccaa0, 0xff9ccfff),
            make106Keys()),
        makePreset ("modern.keys.glass-keys", "Glass Keys", "Modern glass keys",
            "A clear glassy attack with a short noise transient and a soft plate.",
            { "Glass", "Pluck", "Modern", nullptr }, Group::Keys, Collection::Modern,
            artwork (Group::Keys, "modern.keys.glass-keys", Motif::Prism, 0.84f, 0.34f, 0xffffe1b5, 0xff86e4ff),
            makeGlassKeys()),
        makePreset ("modern.keys.neon-clav", "Neon Clav", "Modern clav",
            "A bright, snappy clav with narrow pulse and a phaser bite.",
            { "Clav", "Funky", "Bright", nullptr }, Group::Keys, Collection::Modern,
            artwork (Group::Keys, "modern.keys.neon-clav", Motif::Grid, 0.80f, 0.32f, 0xffffcf82, 0xff7ed6ff),
            makeNeonClav()),
        makePreset ("modern.keys.mono-noire", "Mono Noire", "Cinematic mono keys",
            "An intimate noir texture of sine, triangle, and soft noise.",
            { "Noir", "Cinematic", "Soft", nullptr }, Group::Keys, Collection::Modern,
            artwork (Group::Keys, "modern.keys.mono-noire", Motif::Halo, 0.72f, 0.50f, 0xffd9b78f, 0xff8fa8c8),
            makeMonoNoire()),

        makePreset ("classic.lead.five-voice-solo", "Five Voice Solo", "Prophet-5 lead",
            "An expressive mono solo with glide, bite, and a touch of stereo space.",
            { "Solo", "Analog", "Expressive", nullptr }, Group::Leads, Collection::Classic,
            artwork (Group::Leads, "classic.lead.five-voice-solo", Motif::Ribbon, 0.50f, 0.64f, 0xffff759f, 0xffffbd72),
            makeFiveVoiceSolo()),
        makePreset ("classic.lead.106-lead", "106 Lead", "Juno-106 lead",
            "A singing pulse lead with chorus and just enough filter movement.",
            { "Lead", "Chorus", "80s", nullptr }, Group::Leads, Collection::Classic,
            artwork (Group::Leads, "classic.lead.106-lead", Motif::Bloom, 0.48f, 0.66f, 0xffff5c92, 0xffffa45e),
            make106Lead()),
        makePreset ("classic.lead.eight-voice-sync", "Eight Voice Sync", "OB-X sync lead",
            "A hard-synced mono lead that tears forward under velocity.",
            { "Sync", "Aggressive", "Mono", nullptr }, Group::Leads, Collection::Classic,
            artwork (Group::Leads, "classic.lead.eight-voice-sync", Motif::Grid, 0.54f, 0.58f, 0xffff4d8d, 0xffffb15c),
            makeEightVoiceSync()),
        makePreset ("modern.lead.laser-bloom", "Laser Bloom", "Modern laser lead",
            "A wide unison laser with a bright bloom and a controlled core.",
            { "Laser", "Wide", "Modern", nullptr }, Group::Leads, Collection::Modern,
            artwork (Group::Leads, "modern.lead.laser-bloom", Motif::Bloom, 0.88f, 0.30f, 0xffff6aa8, 0xffffc07a),
            makeLaserBloom()),
        makePreset ("modern.lead.chrome-siren", "Chrome Siren", "Modern siren",
            "A metallic siren with pitch drift, ring modulation, and tempo echo.",
            { "Siren", "Motion", "Chrome", nullptr }, Group::Leads, Collection::Modern,
            artwork (Group::Leads, "modern.lead.chrome-siren", Motif::Ribbon, 0.82f, 0.36f, 0xffff84b4, 0xffff9d6d),
            makeChromeSiren()),
        makePreset ("modern.lead.hyper-lead", "Hyper Lead", "Modern hyper lead",
            "A loud, forward unison lead with drive, chorus, and a short glide.",
            { "Hyper", "Bright", "Wide", nullptr }, Group::Leads, Collection::Modern,
            artwork (Group::Leads, "modern.lead.hyper-lead", Motif::Prism, 0.86f, 0.28f, 0xffff3c7a, 0xffffb84d),
            makeHyperLead()),

        makePreset ("classic.pluck.106-pluck", "106 Pluck", "Juno-106 pluck",
            "A round chorus pluck with a quick filter snap and a soft echo.",
            { "Pluck", "Retro", "Chorus", nullptr }, Group::PlucksArps, Collection::Classic,
            artwork (Group::PlucksArps, "classic.pluck.106-pluck", Motif::Caustic, 0.44f, 0.68f, 0xff62ffd8, 0xffb6ff7a),
            make106Pluck()),
        makePreset ("classic.pluck.five-voice-arp", "Five Voice Arp", "Prophet-5 arp",
            "A tight analog arpeggio with detuned saws and a short gate.",
            { "Arp", "Analog", "Rhythmic", nullptr }, Group::PlucksArps, Collection::Classic,
            artwork (Group::PlucksArps, "classic.pluck.five-voice-arp", Motif::Grid, 0.48f, 0.62f, 0xff5ef2c8, 0xffc4ff72),
            makeFiveVoiceArp()),
        makePreset ("classic.pluck.six-voice-bells", "Six Voice Bells", "Polysix bells",
            "Crystalline vintage bells with ring modulation and a random pattern.",
            { "Bells", "Arp", "Crystalline", nullptr }, Group::PlucksArps, Collection::Classic,
            artwork (Group::PlucksArps, "classic.pluck.six-voice-bells", Motif::Halo, 0.52f, 0.66f, 0xff7dffe0, 0xffc8ff8e),
            makeSixVoiceBells()),
        makePreset ("modern.pluck.glass-drop", "Glass Drop", "Modern glass pluck",
            "A single glass droplet with a pitch-tipped filter envelope.",
            { "Glass", "Drop", "Modern", nullptr }, Group::PlucksArps, Collection::Modern,
            artwork (Group::PlucksArps, "modern.pluck.glass-drop", Motif::Caustic, 0.86f, 0.28f, 0xff73ffe1, 0xffb0ff6e),
            makeGlassDrop()),
        makePreset ("modern.pluck.crystal-arp", "Crystal Arp", "Modern crystal arp",
            "Clean, sparkling arpeggios through a chorus and tempo delay.",
            { "Arp", "Crystal", "Rhythmic", nullptr }, Group::PlucksArps, Collection::Modern,
            artwork (Group::PlucksArps, "modern.pluck.crystal-arp", Motif::Prism, 0.84f, 0.30f, 0xff4fffd2, 0xffd6ff73),
            makeCrystalArp()),
        makePreset ("modern.pluck.kinetic-pluck", "Kinetic Pluck", "Modern kinetic pluck",
            "A mechanical pulse-and-noise pluck moving through phaser and gate.",
            { "Kinetic", "Pluck", "Motion", nullptr }, Group::PlucksArps, Collection::Modern,
            artwork (Group::PlucksArps, "modern.pluck.kinetic-pluck", Motif::Grid, 0.80f, 0.34f, 0xff8cffbf, 0xffe4ff74),
            makeKineticPluck()),

        makePreset ("classic.brass.five-voice-brass", "Five Voice Brass", "Prophet-5 brass",
            "Two detuned saws and a round ladder for warm, articulate poly brass.",
            { "Brass", "Analog", "Bold", nullptr }, Group::BrassStrings, Collection::Classic,
            artwork (Group::BrassStrings, "classic.brass.five-voice-brass", Motif::Halo, 0.46f, 0.66f, 0xffffc46b, 0xffff7a45),
            makeFiveVoiceBrass()),
        makePreset ("classic.brass.eight-voice-brass", "Eight Voice Brass", "OB-X brass",
            "A wide 1980s brass section with pronounced filter movement.",
            { "Brass", "Retro", "Wide", nullptr }, Group::BrassStrings, Collection::Classic,
            artwork (Group::BrassStrings, "classic.brass.eight-voice-brass", Motif::Ribbon, 0.50f, 0.62f, 0xffffd07a, 0xffff8a4f),
            makeEightVoiceBrass()),
        makePreset ("classic.brass.106-strings", "106 Strings", "Juno-106 strings",
            "A warm string machine with slow ensemble and a soft high-pass.",
            { "Strings", "Chorus", "Warm", nullptr }, Group::BrassStrings, Collection::Classic,
            artwork (Group::BrassStrings, "classic.brass.106-strings", Motif::Aurora, 0.44f, 0.70f, 0xffffd9a3, 0xffffa05f),
            make106Strings()),
        makePreset ("modern.brass.neon-brass", "Neon Brass", "Modern brass",
            "Bright synthetic brass with drive, motion, and a neon edge.",
            { "Brass", "Modern", "Neon", nullptr }, Group::BrassStrings, Collection::Modern,
            artwork (Group::BrassStrings, "modern.brass.neon-brass", Motif::Bloom, 0.86f, 0.30f, 0xffffb85c, 0xffff6a35),
            makeNeonBrass()),
        makePreset ("modern.brass.velvet-strings", "Velvet Strings", "Modern velvet strings",
            "Soft cinematic strings with slow attack and aftertouch bloom.",
            { "Strings", "Velvet", "Cinematic", nullptr }, Group::BrassStrings, Collection::Modern,
            artwork (Group::BrassStrings, "modern.brass.velvet-strings", Motif::Caustic, 0.76f, 0.44f, 0xffffcf8f, 0xffff9a58),
            makeVelvetStrings()),
        makePreset ("modern.brass.chrome-ensemble", "Chrome Ensemble", "Modern ensemble",
            "A chrome-plated string and brass hybrid with wide unison.",
            { "Ensemble", "Chrome", "Wide", nullptr }, Group::BrassStrings, Collection::Modern,
            artwork (Group::BrassStrings, "modern.brass.chrome-ensemble", Motif::Ribbon, 0.82f, 0.38f, 0xffffe0a3, 0xffff8f56),
            makeChromeEnsemble()),

        makePreset ("classic.texture.106-drift", "106 Drift", "Juno-106 drift",
            "A slowly drifting analog cloud with chorus and a very long tail.",
            { "Drift", "Analog", "Atmospheric", nullptr }, Group::Textures, Collection::Classic,
            artwork (Group::Textures, "classic.texture.106-drift", Motif::Tide, 0.46f, 0.74f, 0xff8ae8ff, 0xffa6ffcb),
            make106Drift()),
        makePreset ("classic.texture.five-voice-wind", "Five Voice Wind", "Prophet-5 wind",
            "A synthetic wind field of filtered noise and slow motion.",
            { "Wind", "Noise", "Evolving", nullptr }, Group::Textures, Collection::Classic,
            artwork (Group::Textures, "classic.texture.five-voice-wind", Motif::Aurora, 0.44f, 0.76f, 0xff7fdfff, 0xffadffd4),
            makeFiveVoiceWind()),
        makePreset ("classic.texture.eight-voice-choir", "Eight Voice Choir", "OB-X choir",
            "A vast, vocal-adjacent analog choir in a huge glass room.",
            { "Choir", "Analog", "Vast", nullptr }, Group::Textures, Collection::Classic,
            artwork (Group::Textures, "classic.texture.eight-voice-choir", Motif::Halo, 0.50f, 0.72f, 0xff9af2ff, 0xffb8ffd0),
            makeEightVoiceChoir()),
        makePreset ("modern.texture.deep-space", "Deep Space", "Modern drone",
            "A weightless drone for long, deep cinematic beds.",
            { "Drone", "Deep", "Cinematic", nullptr }, Group::Textures, Collection::Modern,
            artwork (Group::Textures, "modern.texture.deep-space", Motif::Tide, 0.86f, 0.30f, 0xff76e2ff, 0xff87ffd7),
            makeDeepSpace()),
        makePreset ("modern.texture.glass-rain", "Glass Rain", "Modern stochastic glass",
            "Scattered glass droplets suspended in a long, high plate reverb.",
            { "Rain", "Stochastic", "Glass", nullptr }, Group::Textures, Collection::Modern,
            artwork (Group::Textures, "modern.texture.glass-rain", Motif::Caustic, 0.88f, 0.26f, 0xff9bf1ff, 0xffb8ffdb),
            makeGlassRain()),
        makePreset ("modern.texture.solstice-drone", "Solstice Drone", "Modern warm drone",
            "A warm seasonal drone, wide but carefully mono-compatible.",
            { "Drone", "Solstice", "Warm", nullptr }, Group::Textures, Collection::Modern,
            artwork (Group::Textures, "modern.texture.solstice-drone", Motif::Bloom, 0.78f, 0.40f, 0xff7affd6, 0xffb9ffea),
            makeSolsticeDrone()),

        makePreset ("classic.motion.106-gate", "106 Gate", "Juno-106 gate",
            "A classic gated chorus pad with a tight sixteenth-note pulse.",
            { "Gated", "Retro", "Rhythmic", nullptr }, Group::Motion, Collection::Classic,
            artwork (Group::Motion, "classic.motion.106-gate", Motif::Grid, 0.50f, 0.64f, 0xff8cff6a, 0xff5ec8ff),
            make106Gate()),
        makePreset ("classic.motion.five-voice-pulse", "Five Voice Pulse", "Prophet-5 pulse",
            "A mechanical amplitude pulse with warm analog filter movement.",
            { "Pulse", "Analog", "Motion", nullptr }, Group::Motion, Collection::Classic,
            artwork (Group::Motion, "classic.motion.five-voice-pulse", Motif::Tide, 0.48f, 0.66f, 0xff9dff72, 0xff6bc9ff),
            makeFiveVoicePulse()),
        makePreset ("classic.motion.six-voice-motion", "Six Voice Motion", "Polysix motion",
            "A restless arpeggiated pad with ensemble and a slow filter sweep.",
            { "Motion", "Arp", "Retro", nullptr }, Group::Motion, Collection::Classic,
            artwork (Group::Motion, "classic.motion.six-voice-motion", Motif::Bloom, 0.52f, 0.62f, 0xffa6ff7c, 0xff76cfff),
            makeSixVoiceMotion()),
        makePreset ("modern.motion.kinetic-grid", "Kinetic Grid", "Modern kinetic grid",
            "A tight, architectural grid of wavetable plucks and sixteenths.",
            { "Grid", "Kinetic", "Modern", nullptr }, Group::Motion, Collection::Modern,
            artwork (Group::Motion, "modern.motion.kinetic-grid", Motif::Grid, 0.90f, 0.26f, 0xff9aff5b, 0xff4fc6ff),
            makeKineticGrid()),
        makePreset ("modern.motion.neon-sequence", "Neon Sequence", "Modern sequence",
            "A neon-lit sequence of wavetable and saw through tempo echo.",
            { "Sequence", "Neon", "Rhythmic", nullptr }, Group::Motion, Collection::Modern,
            artwork (Group::Motion, "modern.motion.neon-sequence", Motif::Prism, 0.86f, 0.30f, 0xffb6ff5e, 0xff63d4ff),
            makeNeonSequence()),
        makePreset ("modern.motion.tidal-motion", "Tidal Motion", "Modern tidal motion",
            "A long, evolving tidal rhythm with slow cut-off and pan drift.",
            { "Tidal", "Evolving", "Wide", nullptr }, Group::Motion, Collection::Modern,
            artwork (Group::Motion, "modern.motion.tidal-motion", Motif::Tide, 0.80f, 0.42f, 0xff7dff74, 0xff6ad7ff),
            makeTidalMotion())
    };

    for (size_t i = 0; i < bank.size(); ++i)
        bank[i].patch.outputTrimDb = std::clamp (bank[i].patch.outputTrimDb + loudnessTrims[i],
                                                 -36.0f, 30.0f);

    return bank;
}

const std::array<Preset, kNumPresets>& bankInstance()
{
    static const auto bank = buildBank();
    return bank;
}
} // namespace

const std::array<Preset, kNumPresets>& getFactoryBank() noexcept
{
    return bankInstance();
}

int getNumFactoryPresets() noexcept
{
    return kNumPresets;
}

int getDefaultPresetIndex() noexcept
{
    return indexOfPresetId (defaultPresetId);
}

const Preset* findPresetById (const char* id) noexcept
{
    const int index = indexOfPresetId (id);
    return index >= 0 ? &getFactoryBank()[static_cast<size_t> (index)] : nullptr;
}

int indexOfPresetId (const char* id) noexcept
{
    if (id == nullptr)
        return -1;

    const auto& bank = getFactoryBank();

    for (int i = 0; i < static_cast<int> (bank.size()); ++i)
        if (std::strcmp (bank[static_cast<size_t> (i)].id, id) == 0)
            return i;

    return -1;
}
} // namespace synthset
