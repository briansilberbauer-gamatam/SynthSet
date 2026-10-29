#pragma once

#include <array>
#include <cstdint>
#include <type_traits>

namespace synthset
{
enum class Group : std::uint8_t
{
    Bass = 0,
    Pads,
    Keys,
    Leads,
    PlucksArps,
    BrassStrings,
    Textures,
    Motion,
    Count
};

enum class Collection : std::uint8_t { Classic = 0, Modern, Count };

enum class VoiceMode : std::uint8_t { Poly = 0, Mono, Legato, Unison };
enum class FilterModel : std::uint8_t { Ladder4P = 0, Svf2P };
enum class FilterType : std::uint8_t { LowPass = 0, HighPass, BandPass };
enum class LfoShape : std::uint8_t { Sine = 0, Triangle, Saw, Square, SampleHold };

enum class ModSource : std::uint8_t
{
    None = 0,
    Lfo1,
    Lfo2,
    FilterEnv,
    AmpEnv,
    Velocity,
    KeyTrack,
    ModWheel,
    Aftertouch,
    PitchBend,
    NoteRandom
};

enum class ModDest : std::uint8_t
{
    None = 0,
    Pitch,
    PulseWidth1,
    PulseWidth2,
    WavetablePos,
    Cutoff,
    Resonance,
    Amp,
    Pan,
    ChorusDepth,
    DelaySend,
    ReverbSend
};

enum class Motif : std::uint8_t
{
    Aurora = 0,
    Prism,
    Bloom,
    Ribbon,
    Caustic,
    Grid,
    Tide,
    Halo
};

inline constexpr int kNumVoices = 16;
inline constexpr int kMaxUnisonLayers = 7;
inline constexpr int kMaxOscillatorLayers = 48;
inline constexpr int kNumPresets = 48;
inline constexpr int kNumGroups = static_cast<int> (Group::Count);
inline constexpr int kPresetsPerGroup = 6;

struct OscParams
{
    float level = 0.7f;
    float pan = 0.0f;            // -1..1
    float octave = 0.0f;         // -3..3, integer-valued
    float semitone = 0.0f;       // -12..12
    float fineCents = 0.0f;      // -50..50
    float shape = 0.0f;          // 0 saw, .33 pulse, .66 triangle, 1 sine
    float pulseWidth = 0.5f;     // 0.05..0.95
    float wavetablePos = 0.0f;   // 0..1
    float drift = 0.08f;         // 0..1
    bool sync = false;
    bool wavetable = false;
};

struct FilterParams
{
    FilterModel model = FilterModel::Ladder4P;
    FilterType type = FilterType::LowPass;
    float cutoffHz = 1200.0f;
    float resonance = 0.25f;     // 0..1 (normalized)
    float envAmount = 0.0f;      // octaves of cutoff movement at full envelope
    float keyTrack = 0.0f;       // 0..1
    float velocity = 0.0f;       // 0..1
    float drive = 0.0f;          // 0..1
    float highPassHz = 20.0f;    // post-filter rumble removal
};

struct AdsrParams
{
    float attackMs = 5.0f;
    float decayMs = 400.0f;
    float sustain = 0.7f;
    float releaseMs = 350.0f;
    float curve = 0.5f;          // 0 = near-linear, 1 = strongly exponential
    float velocity = 0.0f;       // 0..1
};

struct LfoParams
{
    LfoShape shape = LfoShape::Sine;
    float rateHz = 0.35f;
    float fadeInMs = 0.0f;
    float phase = 0.0f;
    bool perVoice = true;
    bool tempoSync = false;
    float syncBeats = 1.0f;      // cycle length in beats when tempo sync is on
    bool retrigger = true;
};

struct ModRoute
{
    ModSource source = ModSource::None;
    ModDest dest = ModDest::None;
    float amount = 0.0f;         // -1..1
};

struct ChorusParams
{
    int mode = 0;                // 0 off, 1 = I, 2 = II, 3 = ensemble
    float depth = 0.35f;
    float mix = 0.35f;
};

struct PhaserParams
{
    int stages = 0;              // 0 off, 4, or 6
    float rateHz = 0.25f;
    float depth = 0.5f;
    float feedback = 0.2f;
    float mix = 0.25f;
};

struct DelayParams
{
    float timeMs = 360.0f;
    float feedback = 0.28f;
    float toneHz = 5200.0f;
    float mix = 0.18f;
    bool pingPong = true;
    bool tempoSync = false;
    float syncBeats = 0.75f;
};

struct ReverbParams
{
    float size = 0.55f;
    float decay = 0.6f;
    float dampingHz = 7000.0f;
    float predelayMs = 12.0f;
    float mix = 0.22f;
    float width = 0.8f;
};

struct ArpParams
{
    bool enabled = false;
    bool latch = false;
    int rateDiv = 4;             // 4 = 1/16, 8 = 1/8, 16 = 1/4
    int octaves = 1;             // 1..3
    int pattern = 0;             // up, down, up/down, random, as-played
    float gate = 0.55f;          // 0.05..0.95
};

struct SynthPatch
{
    OscParams osc1, osc2;
    float subLevel = 0.0f;
    float noiseLevel = 0.0f;
    float ringLevel = 0.0f;
    bool subSquare = true;

    FilterParams filter;
    AdsrParams filterEnv, ampEnv;
    LfoParams lfo1, lfo2;
    std::array<ModRoute, 8> mod {};

    VoiceMode voiceMode = VoiceMode::Poly;
    int unisonVoices = 1;        // 1..7
    float unisonDetune = 0.0f;   // cents
    float unisonSpread = 0.0f;   // 0..1
    float glideMs = 0.0f;

    float pitchBendRange = 2.0f;
    float modWheelDepth = 0.0f;

    ChorusParams chorus;
    PhaserParams phaser;
    DelayParams delay;
    ReverbParams reverb;

    float drive = 0.0f;
    float outputTrimDb = 0.0f;
    float stereoWidth = 1.0f;

    ArpParams arp;
    std::uint32_t seed = 0;
};

static_assert (std::is_trivially_copyable_v<SynthPatch>,
               "SynthPatch must remain trivially copyable and audio-thread safe");

} // namespace synthset
