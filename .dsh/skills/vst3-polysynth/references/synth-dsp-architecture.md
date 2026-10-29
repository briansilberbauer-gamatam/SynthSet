# SynthSet DSP Architecture & Real-Time Implementation Reference

This reference specifies the C++ synthesis engine behind SynthSet. It assumes
JUCE 8.0.8, C++20, and a **preset-only** public interface (zero automatable
parameters, 48 host programs, one selected-preset ID in state). Read this before
writing any `Source/dsp/` code.

---

## 1. Design Goals and Non-Goals

### Goals

- 16-voice polyphony with a warm, expensive-sounding classic core (Juno/Prophet/
  OB/Jupiter/CS-80-inspired) and a modern wavetable/motion layer.
- Distinct, level-matched presets; no "edit mode" required to get a finished
  sound.
- Click-free preset switching with no zipper noise.
- Deterministic offline rendering for tests and screenshots.
- Zero allocation, locks, file I/O, or logging on the audio thread.
- Stable at 44.1, 48, 88.2, 96, 176.4, and 192 kHz; mono or stereo host bus.
- Buildable offline on Linux and in GitHub Actions on Windows/macOS.

### Non-goals

- User patch editing, parameter automation, MPE, multi-timbral operation, or
  user wavetable import.
- Bit-exact emulation of any specific commercial synthesizer. "Inspired by" is
  the target, not circuit cloning.
- Dynamic latency or per-preset oversampling. Report one constant latency.
- A general-purpose DSP framework. Use JUCE `juce_dsp` where it helps; write the
  voice/oscillator/filter core explicitly.

---

## 2. Module Map

```text
MIDI in ──► SynthEngine ──► Voice[16] ──► voice sum (stereo)
              │               │
              │               ├─ Osc1 + Osc2 + Sub + Noise
              │               ├─ Ring / Sync / PWM / Drift
              │               ├─ Filter block (ZDF ladder or TPT SVF + HPF)
              │               ├─ Filter envelope + Amp envelope
              │               ├─ LFO1 + LFO2 + fixed ModMatrix
              │               └─ per-voice pan / level
              ▼
         Arpeggiator (host-synced, preset-controlled)
              ▼
         FX: Drive → Chorus → Phaser → Delay → Plate Reverb → EQ
              ▼
         Output: DC block → soft clip → preset trim → master safety
```

The arpeggiator generates note events before voice allocation. It must not
allocate during processing; use a fixed-capacity event queue (e.g. 64 events).

---

## 3. Data Model: `SynthPatch`

`SynthPatch` is a plain, trivially copyable aggregate. No `juce::String`,
`std::vector`, `std::function`, owning pointers, or `juce::var`.

```cpp
// Source/preset/SynthPatch.h
#pragma once
#include <array>
#include <cstdint>

namespace synthset
{
enum class Group : std::uint8_t
{
    Bass = 0, Pads, Keys, Leads, PlucksArps, BrassStrings, Textures, Motion,
    Count
};

enum class Collection : std::uint8_t { Classic = 0, Modern, Count };

enum class VoiceMode : std::uint8_t { Poly = 0, Mono, Legato, Unison };
enum class FilterModel : std::uint8_t { Ladder4P = 0, Svf2P };
enum class FilterType : std::uint8_t { LowPass = 0, HighPass, BandPass };
enum class LfoShape : std::uint8_t { Sine = 0, Triangle, Saw, Square, SampleHold };
enum class ModSource : std::uint8_t
{
    None = 0, Lfo1, Lfo2, FilterEnv, AmpEnv, Velocity, KeyTrack,
    ModWheel, Aftertouch, PitchBend, NoteRandom
};
enum class ModDest : std::uint8_t
{
    None = 0, Pitch, PulseWidth1, PulseWidth2, WavetablePos,
    Cutoff, Resonance, Amp, Pan, ChorusDepth, DelaySend, ReverbSend
};
enum class Motif : std::uint8_t
{
    Aurora = 0, Prism, Bloom, Ribbon, Caustic, Grid, Tide, Halo
};

struct OscParams
{
    float level = 0.7f;
    float pan = 0.0f;            // -1..1
    float octave = 0.0f;         // -3..3, integer-valued
    float semitone = 0.0f;       // -12..12
    float fineCents = 0.0f;      // -50..50
    float shape = 0.0f;          // 0 saw, .33 pulse, .66 tri, 1 sine
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
    float envAmount = 0.0f;      // -1..1 in octaves-ish
    float keyTrack = 0.0f;       // 0..1
    float velocity = 0.0f;       // 0..1
    float drive = 0.0f;          // 0..1
};

struct AdsrParams
{
    float attackMs = 5.0f;
    float decayMs = 400.0f;
    float sustain = 0.7f;
    float releaseMs = 350.0f;
    float curve = 0.5f;          // 0 = linear-ish, 1 = strongly exponential
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
    float syncBeats = 1.0f;      // used when tempoSync
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
    float subLevel = 0.0f;       // 0..1; waveform selected by subSquare
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

    float pitchBendRange = 2.0f; // semitones
    float modWheelDepth = 0.0f;  // preset-defined vibrato depth

    ChorusParams chorus;
    PhaserParams phaser;
    DelayParams delay;
    ReverbParams reverb;

    float drive = 0.0f;          // pre-FX
    float outputTrimDb = 0.0f;   // -24..+12

    ArpParams arp;
    std::uint32_t seed = 0;      // deterministic drift/artwork/randomness
};
} // namespace synthset
```

Guidance:

- Keep values in physical units where possible (`Hz`, `ms`, `dB`) and normalize
  only at the point of use. Document every range.
- `float` is sufficient for audio; use `double` for phases, sample rates, and
  long delay time accumulation.
- Do not put `juce::Colour` in `SynthPatch`; artwork is a UI concern, defined by
  a separate `ArtworkSpec` in the immutable preset metadata.
- If the field list grows, keep it aggregate-initializable and add a
  compile-time `static_assert(std::is_trivially_copyable_v<SynthPatch>)`.

---

## 4. Preset Metadata and Bank

```cpp
// Source/preset/Preset.h
struct ArtworkSpec
{
    std::uint32_t top = 0xff05070c;
    std::uint32_t mid = 0xff0b1420;
    std::uint32_t bottom = 0xff02040a;
    std::uint32_t accent = 0xff7fd8ff;
    std::uint32_t accent2 = 0xffb38cff;
    Motif motif = Motif::Aurora;
    float seed = 0.0f;
    float energy = 0.5f;
    float haze = 0.5f;
};

struct Preset
{
    const char* id;              // stable, e.g. "classic.pad.106-dream"
    const char* name;            // display, e.g. "106 Dream"
    const char* lineage;         // internal engineering note; never rendered
    const char* description;     // one sentence
    const char* tags[4];         // NULL-terminated
    Group group;
    Collection collection;
    ArtworkSpec artwork;
    SynthPatch patch;
};
```

- The bank is a `static constexpr std::array<Preset, 48>` (or `static const`)
  in `PresetBank.cpp`. No JSON at runtime.
- A constexpr bank allows the compiler to place patches in read-only memory and
  makes accidental mutation impossible.
- Keep helper builders for shared voicing families, but the resulting object must
  be fully specified. Example:

```cpp
constexpr SynthPatch makeJunoPad()
{
    SynthPatch p {};
    p.osc1.shape = 0.34f; p.osc1.pulseWidth = 0.42f; p.osc1.level = 0.55f;
    p.osc2.shape = 0.0f;  p.osc2.fineCents = 8.0f;    p.osc2.level = 0.45f;
    p.subLevel = 0.28f;
    p.filter.model = FilterModel::Svf2P;
    p.filter.cutoffHz = 1800.0f;
    p.filter.resonance = 0.18f;
    p.chorus.mode = 2; p.chorus.depth = 0.65f; p.chorus.mix = 0.4f;
    p.ampEnv.attackMs = 900.0f; p.ampEnv.releaseMs = 2600.0f;
    p.reverb.mix = 0.22f;
    return p;
}
```

### Host program integration

- `AudioProcessor::getNumPrograms()` returns `48`.
- `getProgramName(index)` returns `preset.name` (or a `juce::String` built on the
  message thread; this is not an audio-thread call).
- `setCurrentProgram(index)` posts the new index atomically.
- `getCurrentProgram()` returns the last applied index, not the pending one.
- `getStateInformation()` writes a tiny XML/ValueTree blob:
  `schema=1`, `presetId="classic.keys.five-voice-keys"`. Read it on restore,
  resolve the ID, and fall back to the default if unknown.
- `setStateInformation()` must never apply a raw patch. It resolves a factory
  preset only.
- Handle `MidiMessage::programChange` by mapping the 0–127 program number to the
  bank (wrap or ignore out-of-range values; document the choice). Prefer ignoring
  out-of-range rather than surprising the player.

---

## 5. Threading and Preset Switching

### Ownership

- The `PresetBank` array is compile-time immutable.
- `PresetManager` owns:
  - `std::atomic<int> pendingProgram { defaultIndex };`
  - `std::atomic<int> appliedProgram { defaultIndex };`
  - a message-thread `juce::AsyncUpdater` for UI notification.
- `SynthEngine` owns voices, FX state, and a local `const SynthPatch* active`.
- No UI pointer is ever read by the audio thread.

### Request path

```cpp
void PresetManager::requestProgram(int index)
{
    if (index >= 0 && index < (int) bank.size())
        pendingProgram.store(index, std::memory_order_release);
}

void SynthEngine::process(AudioBuffer<float>& out, MidiBuffer& midi)
{
    const int requested = pendingProgram.load(std::memory_order_acquire);
    if (requested != localProgram)
        beginPresetFade(requested);   // starts a short fade-out
    // ... fade-out; when gain reaches 0:
    //   reset voices and FX, active = &bank[requested], localProgram = requested
    //   pendingProgramApplied.store(...), start fade-in
}
```

### Fade rules

- Fade length: **15 ms** out and **15 ms** in (adjust 10–25 ms). Linear or
  equal-power ramp, per sample. No zipper.
- On fade-out completion: `allNotesOff()`, clear voice states, reset filter and
  FX delay lines, reset LFO phases (or preserve global phase if defined by the
  patch). Clear reverb/delay tails so the old preset cannot bleed into the new
  one.
- Continue to process MIDI during the fade; note-ons that arrive during the swap
  are applied after the new patch is active.
- If the user selects another preset mid-fade, restart the fade-out from the
  current gain. Do not stack fades or allocate.
- Report the new program via an atomic. UI polls at 30 Hz or uses `AsyncUpdater`
  and crossfades artwork.

### Why not crossfade two full synth engines?

It doubles CPU and complicates voice/MIDI state. A 15 ms silence is musically
acceptable for a preset-only instrument. If a later version wants a true
crossfade, use a preallocated B engine and document the CPU/latency impact.

---

## 6. Voice Allocation

### Voice struct

```cpp
struct Voice
{
    bool active = false;
    bool releasing = false;
    std::uint8_t note = 0;
    float velocity = 0.0f;
    std::uint64_t startOrder = 0;
    float pan = 0.0f;

    // Per-voice layers (unison) are preallocated; 1 layer for normal poly.
    std::array<UnisonLayer, 7> layers {};
    int numLayers = 1;

    // Envelopes, filters, LFO phases, drift values...
};
```

### Allocation policy

1. If a voice already plays the same note and is not releasing, retrigger it
   (respecting mono/legato modes) rather than stacking an identical note.
2. Prefer an inactive voice.
3. Else prefer the oldest releasing voice.
4. Else prefer the quietest sounding voice.
5. Else steal the oldest sounding voice.
6. Fade the stolen voice over 3–5 ms before reusing its state. If the voice is
   already inaudible, reset immediately.

`startOrder` increments monotonically per note-on; it must not overflow in a
session practically (64-bit).

### Modes

- **Poly**: up to 16 notes (or fewer for unison).
- **Mono**: one note; new note steals/retriggers; note-off falls back to the
  most recent held note if legato.
- **Legato**: glide/retrigger rules defined by preset; amp envelope not
  retriggered on overlapping notes.
- **Unison**: 2–7 layers per note, detuned symmetrically, panned with a spread.
  Cap total layers to 48 (e.g. 8 notes × 6 layers) to bound CPU; voice stealing
  applies to notes, not individual layers.

### Drift/tolerance

For each voice/layer, derive deterministic offsets from `patch.seed`, voice
index, note, and layer index:

```cpp
static inline std::uint32_t splitMix32 (std::uint32_t x) noexcept
{
    x += 0x9e3779b9u;
    x = (x ^ (x >> 16)) * 0x85ebca6bu;
    x = (x ^ (x >> 13)) * 0xc2b2ae35u;
    return x ^ (x >> 16);
}

static inline float hashToUnit (std::uint32_t x) noexcept
{
    return static_cast<float> (splitMix32 (x) >> 8) * (1.0f / 16777216.0f);
}

const float pitchOffsetCents =
    (hashToUnit (patch.seed ^ (voiceIdx * 0x9e3779b9u)) - 0.5f)
    * 2.0f * maxDriftCents;
```

Apply only when `patch.osc*.drift > 0`. Keep offsets fixed for the life of the
voice; do not randomize per sample. The same preset + same MIDI must produce the
same audio offline (except for explicitly global random LFOs, which should also
be deterministic).

---

## 7. Oscillators

### 7.1 Phase and pitch

- Maintain `double phase` in `[0,1)` and `double phaseInc = freq / sampleRate`.
- Compute pitch once per control block (16 or 32 samples) and interpolate/ramp
  the increment to avoid zipper on pitch modulation.
- Clamp frequency to `[0.01, sampleRate * 0.48]` before generating.
- Hard sync: a master phase wrap resets the slave phase; apply a small BLEP at
  the reset to reduce clicks.

### 7.2 PolyBLEP

```cpp
static inline float polyBlep(double t, double dt) noexcept
{
    if (t < dt)
    {
        t /= dt;
        return float(t + t - t * t - 1.0);
    }
    if (t > 1.0 - dt)
    {
        t = (t - 1.0) / dt;
        return float(t * t + t + t + 1.0);
    }
    return 0.0f;
}

static inline float saw(double t, double dt) noexcept
{
    return float(2.0 * t - 1.0) - polyBlep(t, dt);
}

static inline float pulse(double t, double dt, float pw) noexcept
{
    const double p = juce::jlimit(0.05, 0.95, (double) pw);
    float y = (t < p) ? 1.0f : -1.0f;
    y += polyBlep(t, dt);
    double t2 = t - p;
    if (t2 < 0.0) t2 += 1.0;
    y -= polyBlep(t2, dt);
    return y;
}
```

- Triangle: integrate a band-limited square with a leaky integrator, or use a
  band-limited table. Do not use a naive `2*abs(2t-1)-1` at high notes without
  filtering.
- Sine: `std::sin(2π phase)`, or a cheap polynomial approximation if profiling
  demands it.
- Morph: crossfade adjacent waveform outputs (`saw → pulse → triangle → sine`).
  This can produce phase cancellation; it is part of the designed character, not
  a bug.
- Sub: square or sine one/two octaves down, phase-locked to osc1. Never generate
  a naive sub square without band-limiting; use PolyBLEP.
- Noise: white noise from a fast xorshift; pink via a 3-pole filter. Seed per
  voice for determinism.

### 7.3 Wavetable mode

- Generate or embed a small bank of **band-limited, mip-mapped** tables at
  `prepareToPlay()`.
- 2048 samples/table, one table per octave band, crossfade between adjacent
  mips near band boundaries.
- Wavetable position morphs between frames; apply interpolation (linear or
  cubic) and a DC blocker.
- Do not allocate or compute FFTs in `processBlock()`.

### 7.4 Sync and ring mod

- Hard sync: reset slave phase on master wrap; BLEP the reset discontinuity.
- Soft sync (`sync` strength) can blend reset widths for a less aggressive tone.
- Ring mod: `osc1 * osc2`, scaled and DC-blocked; do not let it double the output
  level silently.

### 7.5 Anti-alias acceptance

At 48 kHz, a C6 saw should have no audible alias partials above the noise floor.
Add an FFT-based test using `juce::dsp::FFT`:
- render 1 s at C6 (1046.5 Hz), no filter, no FX;
- compare energy in bins that are not harmonics of the fundamental;
- require at least 12 dB improvement over a naive saw reference and no discrete
  alias spur above −60 dBFS.

---

## 8. Filters

### 8.1 TPT state-variable filter (2-pole)

Use the Cytomic/Zavalishin topology. Compute coefficients when cutoff/resonance
change at control rate:

```cpp
struct Svf
{
    double ic1 = 0.0, ic2 = 0.0;

    struct Coeffs { double a1, a2, a3, k; };

    static Coeffs make(double cutoffHz, double q, double fs)
    {
        const double g = std::tan(juce::MathConstants<double>::pi * cutoffHz / fs);
        const double k = 1.0 / std::max(0.05, q);
        const double a1 = 1.0 / (1.0 + g * (g + k));
        return { a1, g * a1, g * g * a1, k };
    }

    float process(float x, const Coeffs& c, FilterType type) noexcept
    {
        const double v3 = x - ic2;
        const double v1 = c.a1 * ic1 + c.a2 * v3;
        const double v2 = ic2 + c.a2 * ic1 + c.a3 * v3;
        ic1 = 2.0 * v1 - ic1;
        ic2 = 2.0 * v2 - ic2;

        switch (type)
        {
            case FilterType::LowPass:  return float(v2);
            case FilterType::BandPass: return float(v1);
            case FilterType::HighPass: return float(x - c.k * v1 - v2);
        }
        return float(v2);
    }
};
```

Notes:
- `q` from normalized resonance: `q = 0.5 + resonance * 12.0` or a musically
  tuned curve; document it. At high Q, saturate the input or state mildly to
  prevent screech, not to hide instability.
- `cutoffHz` must be clamped to `[20, 0.45 * fs]`.
- Flush denormals (`ic1/ic2` below 1e-20 → 0) or rely on `ScopedNoDenormals`.

### 8.2 ZDF ladder (4-pole)

Four cascaded TPT one-poles with global negative feedback. Solve the zero-delay
feedback loop per sample (or once per sample with 1–2 Newton iterations if the
feedback path is nonlinear).

```cpp
// One TPT one-pole:
// G = tan(pi*fc/fs); a = G/(1+G); b = 1-a;
// y = a*u + b*s; s = 2*y - s;
//
// For four identical stages and feedback k:
// a4 = a^4
// S  = a^3*b*s1 + a^2*b*s2 + a*b*s3 + b*s4
// y4 = (a4*x + S) / (1 + k*a4)
// u  = x - k*y4
// then run the four stages sequentially with u.
```

Implementation rules:
- `k` from resonance: `0..4` where values near 4 approach self-oscillation.
  Clamp below the unstable limit for your discretization; validate at 44.1 and
  192 kHz.
- Apply `tanh` to the feedback state (or use a saturated transistor model) to
  keep high resonance musical. Compensate the drive gain so level does not jump.
- Update states in the same order every sample. Never share state between
  channels.
- A post-filter DC blocker is required when drive is high.
- If self-oscillation is enabled for a preset, it must start from silence without
  NaN and track pitch within a few cents.

### 8.3 High-pass

A 1-pole or 2-pole TPT high-pass after the main filter, preset-controlled. Use it
to remove sub rumble on pads/leads, not as a user control.

### 8.4 Filter modulation

Cutoff frequency in octaves:
```text
cutoff = baseCutoff * 2^(envAmount*env + keyTrack*(note-60)/12
                          + lfo + velocity*velAmount + aftertouch)
```
Clamp the exponent before `exp2` to avoid huge jumps. Smooth cutoff over 5–20 ms
at control rate. Resonance modulation should be smoothed as well; do not step
coefficients at block boundaries without interpolation.

---

## 9. Envelopes

Use analog-style exponential segments with a preset-defined curve.

```cpp
struct Adsr
{
    enum class Stage { Idle, Attack, Decay, Sustain, Release };
    Stage stage = Stage::Idle;
    float level = 0.0f;

    void noteOn() { stage = Stage::Attack; }
    void noteOff() { if (stage != Stage::Idle) stage = Stage::Release; }

    float process(const AdsrParams& p, float sampleRate) noexcept
    {
        const float fs = sampleRate;
        switch (stage)
        {
            case Stage::Attack:
            {
                const float tau = std::max(0.0005f, p.attackMs * 0.001f);
                level += (1.15f - level) * (1.0f - std::exp(-1.0f / (tau * fs)));
                if (level >= 1.0f) { level = 1.0f; stage = Stage::Decay; }
                break;
            }
            case Stage::Decay:
            {
                const float tau = std::max(0.001f, p.decayMs * 0.001f);
                level += (p.sustain - level) * (1.0f - std::exp(-1.0f / (tau * fs)));
                if (std::abs(level - p.sustain) < 1e-4f) { level = p.sustain; stage = Stage::Sustain; }
                break;
            }
            case Stage::Sustain:
                level = p.sustain;
                break;
            case Stage::Release:
            {
                const float tau = std::max(0.001f, p.releaseMs * 0.001f);
                level += (0.0f - level) * (1.0f - std::exp(-1.0f / (tau * fs)));
                if (level < 1e-5f) { level = 0.0f; stage = Stage::Idle; }
                break;
            }
            case Stage::Idle: level = 0.0f; break;
        }
        return juce::jlimit(0.0f, 1.0f, level);
    }
};
```

- The attack target overshoots 1.0 so the segment reaches 1.0 in finite time;
  clamp. For a more analog feel, use a one-pole with a velocity-dependent target
  and a curve warp (`pow(level, curve)` at the destination only).
- Retrigger from the current level, not from zero, unless the preset explicitly
  asks for a hard retrigger. A hard retrigger must ramp, not jump.
- `Idle` is a valid voice-free condition for the amp envelope. Recycle the voice
  after release plus a small tail (e.g. 5 ms) to avoid clicks.
- Velocity scales amp and/or filter envelope by the preset amounts; never apply
  velocity twice.

---

## 10. LFOs and Modulation Matrix

### LFO

- One global LFO and one per-voice LFO (per preset). Per-voice LFOs reset on
  note-on if `phase` is specified; global LFOs free-run.
- Shapes: sine, triangle, saw, square, sample-and-hold. S&H updates once per
  cycle or at a preset-defined rate; interpolate if needed.
- Fade-in: multiply by a ramp from 0 to 1 over `fadeInMs`. Use a per-voice
  fade-in state.
- Tempo sync: read host BPM/PPQ from `AudioPlayHead` at block start; compute
  phase increment from `syncBeats`. Fall back to 120 BPM if no playhead.
- Output in `[-1, 1]` (unipolar sources like envelopes are `[0, 1]`, then
  converted according to the destination's expected polarity).

### Matrix

- Evaluate at **control rate** (every 16 or 32 samples) and smooth destinations.
  This is sufficient for LFOs and envelopes and much cheaper than per-sample
  routing.
- Each destination sums all routes, then clamps:
  - `pitch`: ±48 semitones max, smoothed;
  - `cutoff`: ±8 octaves max;
  - `resonance`: normalized 0..1;
  - `amp`: 0..1.5 (avoid negative phase inversion);
  - `pan`: -1..1;
  - sends: 0..1.
- Source values: velocity/keytrack are unipolar; LFOs bipolar; aftertouch and
  mod wheel unipolar `[0,1]`; pitch bend `[-1,1]` scaled by preset bend range.
- Apply `modWheelDepth` as a preset-defined vibrato depth on pitch and/or
  filter, not as a hidden global parameter.

### Per-voice random

`NoteRandom` must be deterministic from the voice's note-on seed. It is useful
for analog drift, not for a different sound on every render.

---

## 11. Effects

All effects run in stereo, with preallocated buffers sized for the maximum
sample rate (192 kHz) and maximum delay/time. Use `juce::AudioBuffer<float>` or
raw `std::array` allocated in `prepareToPlay()`.

### 11.1 Drive

- Pre-FX soft saturation (`tanh`, tube-like asymmetry, or a wavefolder for modern
  presets).
- 2x oversampling is optional; if used, it must be always-on with constant
  latency, or use ADAA to avoid aliasing without latency.
- Compensate output gain so `drive=0` is unity and increasing drive does not
  blow up the preset level.

### 11.2 Juno-style chorus/ensemble

- Three short modulated delay lines (roughly 5–30 ms), with LFO rates around
  0.513 Hz and 0.863 Hz for mode I/II, plus a faster ensemble mode.
- Invert one side for the wide stereo effect; sum to mono-compatible output when
  needed.
- Smooth delay-time changes; use fractional delay interpolation (linear or
  all-pass).
- Modes and depth are preset values; no UI.

### 11.3 Phaser

- 4 or 6 first-order all-pass stages per channel, modulated cutoff, feedback,
  and wet/dry mix.
- Keep feedback below unity and saturate inside the loop. Stereo LFO phase
  offset for width.

### 11.4 Delay

- Stereo delay lines up to 2 seconds, fractional read, filtered feedback
  (one-pole low-pass), ping-pong routing per preset.
- Tempo sync uses host BPM; if no playhead, 120 BPM.
- Smooth time changes (or mute/duck briefly) to avoid pitch artifacts; for a
  preset-only synth, a 20 ms crossfade is acceptable.
- Clear the lines on preset switch.

### 11.5 Plate reverb

- Dattorro-style plate: input diffusers, a modulated tank with all-pass and
  delay stages, damping filters, and stereo output taps.
- Pre-delay up to 120 ms, decay/size/damping/width per preset.
- No allocation in `processBlock()`; modulated delay reads use fractional
  interpolation.
- Clear the reverb state on preset switch so the previous preset's tail does not
  bleed into the new one.
- Validate mono compatibility: no phase-inverted left/right build-up.

### 11.6 Output stage

- DC blocker (one-pole high-pass at ~5–10 Hz).
- Gentle saturation and a safety soft clip:
  ```cpp
  y = std::tanh(y * drive) / std::tanh(drive); // drive ~1.0–1.5
  ```
  or a polynomial soft clip with a hard safety clamp at ±1.0.
- Per-preset `outputTrimDb`, then a final fixed safety trim. Never let the
  limiter be the primary way presets achieve their level.
- Metering for the UI is read-only: atomic peak/RMS values for L/R and voice
  count. No meter value is a parameter.

---

## 12. Arpeggiator / Pattern Engine

- Runs before voice allocation and consumes held notes.
- Host tempo sync read once per block. If unavailable, 120 BPM.
- Modes: up, down, up/down, random, as-played. Octave range 1–3.
- Rate from preset (`1/4`, `1/8`, `1/16`, `1/32`, triplet variants if needed).
- Gate length 5–95% of the step.
- Latch behavior is baked into the preset.
- No allocation: fixed-size held-note array (max 16), fixed event queue.
- On preset switch, clear the arp state and held-note latch to avoid carrying
  notes into the new patch.

---

## 13. Parameter Smoothing and Control Rate

Even without user parameters, preset changes and modulation require smoothing.

- Control block: 16 samples at 48 kHz is a good default (1/3 ms). Update LFOs,
  envelopes, filter coefficients, and mod matrix there.
- Audio-rate: oscillator phase, filter state, amplitude.
- Smooth: cutoff (5–15 ms), resonance (10–20 ms), pan (5–10 ms), amp gain
  (1–5 ms), FX mix (10–20 ms).
- Use `juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>` or your own
  one-pole. Do not call `setTargetValue()` per sample without a plan.
- Avoid recomputing `tan`, `exp`, or `pow` per sample for slowly changing
  values. Cache coefficients at control rate.
- At 192 kHz, control blocks may be 64 samples to keep CPU sane; document the
  choice and test modulation smoothness.

---

## 14. MIDI Handling

- Parse MIDI events with sample offsets; do not quantize all events to the block
  start.
- Supported: note on/off with velocity, pitch bend, mod wheel (CC1), sustain
  (CC64), channel pressure, all-notes-off (CC123), all-sound-off (CC120), and
  program change.
- Ignore unsupported CCs without logging.
- Sustain: note-offs are deferred until pedal release. Releasing the pedal must
  release all deferred notes; do not leave stuck voices.
- Pitch bend: range from preset, smoothed over ~5 ms.
- Mod wheel/aftertouch: preset-defined destinations, no hidden global parameter.
- Program change: map to bank index. Out-of-range values are ignored and the
  previous preset remains.
- On `reset()` / `prepareToPlay()`, clear voices, arps, sustain, and FX state.
- `allNotesOff` must always work, including during a preset fade.

---

## 15. Host State and Programs

### State blob (small and stable)

```xml
<SYNTHSET schema="1" presetId="classic.keys.five-voice-keys"/>
```

- Use `juce::XmlElement` or `juce::ValueTree` on the message thread.
- No patch data. No UI dimensions. No "last group filter" unless the owner asks.
- If schema is newer than the plugin understands, ignore safely and use the
  default preset.
- If `presetId` is unknown, fall back to the default and do not fail host load.

### Program list details

- Program names are display names; IDs live in metadata.
- `getCurrentProgram()` reflects the applied preset, not a pending request.
- When restoring state, resolve by ID **before** the first `processBlock()` so
  the first note uses the right patch.
- Host may call `setCurrentProgram()` before `prepareToPlay()`; store the pending
  index safely and apply after preparation.

---

## 16. Test Matrix

Implement `Tests/DspTests.cpp` as a self-contained console runner (no test
framework required; return non-zero on failure). Cover:

### Preset bank

1. `bank.size() == 48`.
2. Eight groups, six per group, three Classic + three Modern per group.
3. 24 Classic, 24 Modern.
4. Unique IDs, unique display names, non-empty description/tags.
5. Default ID resolves; unknown ID falls back.
6. `getNumParameters() == 0`; `getNumPrograms() == 48`.

### Audio

7. For every preset, at 44.1/48/96/192 kHz:
   - render a single C4 note and an 8-note chord;
   - all samples finite;
   - peak ≤ −1.0 dBFS;
   - no DC > 0.001 after 1 s of silence;
   - release tail decays below −90 dBFS within 10 s (unless a deliberately
     infinite drone; document exceptions).
8. Filter stability: sweep cutoff/resonance at all sample rates; no NaN, no
   runaway, no denormal CPU spike.
9. Oscillator alias check (FFT, C6 saw, as described above).
10. Envelope timing: attack/decay/release within ±10% at 48 kHz.
11. LFO shape and tempo sync at 90/120/140 BPM.
12. Voice stealing: 32-note chord, then release; no stuck voices, no clicks
    above threshold.
13. Sustain pedal and all-notes-off; no stuck notes.
14. Mono/legato/glide transitions are click-free.
15. Preset-switch click test: switch during a held chord; maximum inter-sample
    delta below a defined threshold; no old reverb tail after 250 ms.
16. State round-trip preserves ID; state with a bogus ID loads default.
17. Mono host bus and stereo host bus both produce finite output.

### Host smoke test

`Tools/HostSmokeTest.cpp` should:
- scan/instantiate `dist/SynthSet.vst3` with `JUCE_PLUGINHOST_VST3=1`;
- assert 48 programs, 0 parameters, 2 in / 2 out buses, MIDI input;
- process a chord and program changes;
- save/restore state and confirm the preset ID;
- verify no crash on editor open/close if the host supports it.

### Numerical helpers

- `isFinite()` over the whole buffer.
- Peak/RMS/DC.
- Click metric: max `abs(y[n] - y[n-1])` around a switch, compared with the
  steady-state max delta of the same preset. Use a generous threshold (e.g.
  0.25 at normal levels) to avoid false failures from bright waveforms.

---

## 17. Performance Budget and Profiling

Target: 16 voices, 48 kHz, 128-sample block, 2 oscillators + sub + noise +
filter + envelopes + LFOs + chorus + delay + plate reverb, under ~35% of one
modern desktop core.

Practical rules:
- 2 oscillators per voice, not 8; unison multiplies within the voice budget.
- Modulation at 16–32-sample control rate.
- Wavetable mips and cached filter coefficients.
- Reverb is the most expensive stage; use a fixed topology and avoid per-sample
  `pow`/`tan`.
- Profile Release builds, not Debug. Use `perf`/Instruments/ETW or simple
  `juce::Time::getHighResolutionTicks()` around block processing in a developer
  build.
- If a preset exceeds budget, reduce unison/voices or simplify reverb; do not
  silently drop voices at random.
- Keep the editor's animation timer independent from audio. When the editor is
  closed, no UI timer runs.

---

## 18. Implementation Order

1. `SynthPatch`, `Preset`, preset bank skeleton with 48 placeholder entries.
2. `SynthEngine` with a single voice, PolyBLEP saw, SVF low-pass, ADSR, stereo
   output.
3. Voice allocation, MIDI, sustain, program selection, state.
4. Filters, oscillators, LFO/mod matrix, drift.
5. Chorus, phaser, delay, reverb, output stage.
6. Preset bank content and loudness matching.
7. UI artwork/rail/hero and screenshot harness.
8. Host smoke test, CI, packaging, documentation.

Do not build the UI first. The preset artwork depends on stable preset metadata,
and the screenshot harness needs a working engine to look alive.

---

## 19. DSP Review Checklist

- [ ] No allocation/locks/logging in `processBlock()`.
- [ ] `getNumParameters() == 0`; 48 programs; state is ID-only.
- [ ] PolyBLEP or band-limited wavetables on every discontinuous waveform.
- [ ] Filter stable at max resonance, 20 Hz and 20 kHz, all sample rates.
- [ ] Envelopes retrigger without clicks; release reaches zero.
- [ ] Voice stealing fades; no stuck notes with sustain/all-notes-off.
- [ ] Preset switch is fade-out/reset/fade-in, clears tails, continues MIDI.
- [ ] Constant reported latency; no per-preset latency changes.
- [ ] All 48 presets finite and ≤ −1 dBFS; loudness within ±1.5 LU.
- [ ] Mono and stereo host buses both work.
- [ ] CPU within budget on the heaviest preset.
- [ ] UI has zero sound-editing controls.
