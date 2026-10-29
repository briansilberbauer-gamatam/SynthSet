# SynthSet Factory Preset Library — 48 Presets in 8 Groups

This is the authoritative v1 bank for SynthSet. Implement it exactly: **48
factory presets, eight groups × six presets, 24 Classic + 24 Modern**. Each preset
is an immutable `SynthPatch` plus display metadata and an `ArtworkSpec`. There is
no user editing and no "init" preset.

Read `synth-dsp-architecture.md` for the `SynthPatch` fields and
`look-and-feel-recipes.md` for artwork generation.

---

## 1. Taxonomy and Ordering

### Collection

- **Classic** — 1970s/1980s-inspired polyphonic voicing, warmer palettes, lower
  visual energy, analog drift, chorus/ensemble where idiomatic.
- **Modern** — wavetable, unison, spectral motion, wider stereo, cooler/more
  saturated glass artwork.

### Group display order (stable)

| # | Group | ID segment | Preset count |
| :-: | :--- | :--- | :-: |
| 1 | Bass | `bass` | 6 |
| 2 | Pads | `pad` | 6 |
| 3 | Keys | `keys` | 6 |
| 4 | Leads | `lead` | 6 |
| 5 | Plucks & Arps | `pluck` | 6 |
| 6 | Brass & Strings | `brass` | 6 |
| 7 | Textures & Atmos | `texture` | 6 |
| 8 | Motion & Rhythmic | `motion` | 6 |

Each group contains three Classic and three Modern presets, listed Classic first.
The table order inside each group is the intended display order and must not be
alphabetized.

### Stable IDs

Format: `<collection>.<group>.<slug>`.

- IDs are lower-case ASCII, digits, and hyphens only.
- IDs are permanent. Do not rename, recycle, or derive them from row indices.
- Display names can change without breaking sessions.
- Default preset: **`classic.keys.five-voice-keys`**.
- Persist the ID in host state, not the program index.

### Naming and legal

Display names deliberately avoid manufacturer trademarks: "106", "Five Voice",
"Eight Voice", "Six Voice", "Sunset Poly". The `Lineage` column is internal
design guidance and may mention Juno/Prophet/OB/Jupiter/CS-80/Polysix for
engineers. User-facing descriptions should say "inspired by a classic 1980s
six-voice poly" and must not imply affiliation or use another company's logo or
trade dress.

---

## 2. The Bank

Legend: **Coll** = C (Classic) / M (Modern). **Motif** maps to `Motif` in
`ArtworkSpec`. Every preset gets a unique artwork seed derived from its stable
ID; no two presets may share a hero image.

### 2.1 Bass

| ID | Name | Coll | Lineage | Design target | UI tags | Motif |
| :--- | :--- | :-: | :--- | :--- | :--- | :--- |
| `classic.bass.106-sub` | 106 Sub | C | Juno-106 sub bass | One pulse + sub one octave down, SVF low-pass around 180 Hz, fast filter decay, mono, no chorus. Round fundamental, almost no click. | Sub, Mono, Round | Prism |
| `classic.bass.five-voice-low` | Five Voice Low | C | Prophet-5 bass | Two saws with 4–7 cents detune, 4-pole ladder around 240 Hz, punchy filter envelope, short release, poly for octave stacking. | Analog, Punch, Poly | Grid |
| `classic.bass.sunset-thump` | Sunset Thump | C | OB-X bass | Two pulse oscillators detuned, ladder around 300 Hz, mild drive, fast amp decay and medium release. Warm but defined. | Warm, Drive, Mono | Tide |
| `modern.bass.glass-sub` | Glass Sub | M | Modern clean sub | Sine plus a wavetable sub layer, ladder around 90 Hz, minimal drive, mono legato, very clean. Sits under a mix without mud. | Sub, Clean, Modern | Halo |
| `modern.bass.neon-reese` | Neon Reese | M | Modern Reese | Two detuned saws, unison 3, SVF low-pass around 420 Hz, 4-stage phaser, wide stereo, long sustain. Dark and animated. | Reese, Wide, Dark | Ribbon |
| `modern.bass.prism-bass` | Prism Bass | M | Modern wavetable bass | Wavetable + narrow pulse, short filter envelope, chorus, mono, bright transient. Designed for syncopated lines. | Wavetable, Pluck, Bright | Prism |

### 2.2 Pads

| ID | Name | Coll | Lineage | Design target | UI tags | Motif |
| :--- | :--- | :-: | :--- | :--- | :--- | :--- |
| `classic.pad.106-dream` | 106 Dream | C | Juno-106 pad | PWM pulse + saw, sub, chorus II, slow attack (~1.2 s), long release (~3 s), plate. Lush and slightly unstable. | Lush, Chorus, Wide | Aurora |
| `classic.pad.five-voice-halo` | Five Voice Halo | C | Prophet-5 pad | Two saws detuned, ladder around 2.5 kHz, slow attack, long release, stereo delay. Warm analog halo without chorus. | Analog, Warm, Evolving | Halo |
| `classic.pad.sunset-pad` | Sunset Pad | C | OB-X pad | Pulse/saw blend, SVF low-pass, ensemble, slow filter LFO, medium attack. Soft sunset-colored motion. | Retro, Motion, Warm | Ribbon |
| `modern.pad.aurora-glass` | Aurora Glass | M | Modern wavetable pad | Wavetable + saw, unison 5, slow LFO to wavetable position and cutoff, chorus, plate, ~4 s release. Wide and luminous. | Glass, Wide, Cinematic | Aurora |
| `modern.pad.frozen-bloom` | Frozen Bloom | M | Modern cold pad | Wavetable + filtered noise, high-pass, tempo-synced delay, slow filter bloom, long plate. Icy and breathing. | Icy, Slow, Atmospheric | Bloom |
| `modern.pad.iridescent-choir` | Iridescent Choir | M | Modern spectral choir | Two wavetables + noise, chorus, very long reverb, aftertouch to cutoff. Vocal-ish without a formant filter. | Choir, Spectral, Lush | Caustic |

### 2.3 Keys

| ID | Name | Coll | Lineage | Design target | UI tags | Motif |
| :--- | :--- | :-: | :--- | :--- | :--- | :--- |
| `classic.keys.five-voice-keys` | Five Voice Keys | C | Prophet-5 keys | Two saws, ladder around 3 kHz, fast attack, 600 ms decay to 30% sustain, light delay, no chorus. The default preset. | Classic, Poly, Bright | Prism |
| `classic.keys.eight-voice-organ` | Eight Voice Organ | C | OB-X organ | Pulse + sub, broad SVF low-pass, fast attack/release, chorus I, no filter envelope. Stacks like an organ. | Organ, Retro, Ensemble | Grid |
| `classic.keys.106-keys` | 106 Keys | C | Juno-106 keys | PWM pulse, sub, chorus I, moderate attack, ~800 ms release. Soft, chorus-rich keyboard sound. | Analog, Chorus, Soft | Caustic |
| `modern.keys.glass-keys` | Glass Keys | M | Modern glass keys | Wavetable with a short noise transient, ladder low-pass, fast amp decay, delay + plate. Clear glassy attack. | Glass, Pluck, Modern | Prism |
| `modern.keys.neon-clav` | Neon Clav | M | Modern clav | Narrow pulse, high-pass, snappy filter envelope, 4-stage phaser, poly. Funky and bright without being a sample. | Clav, Funky, Bright | Grid |
| `modern.keys.mono-noire` | Mono Noire | M | Cinematic mono keys | Sine/triangle + noise, dark ladder, long release, plate, aftertouch vibrato. Intimate film-noir texture. | Noir, Cinematic, Soft | Halo |

### 2.4 Leads

| ID | Name | Coll | Lineage | Design target | UI tags | Motif |
| :--- | :--- | :-: | :--- | :--- | :--- | :--- |
| `classic.lead.five-voice-solo` | Five Voice Solo | C | Prophet-5 lead | Saw + pulse, ladder around 4 kHz, mono legato, 80 ms glide, stereo delay. Expressive with velocity and mod wheel. | Solo, Analog, Expressive | Ribbon |
| `classic.lead.106-lead` | 106 Lead | C | Juno-106 lead | PWM pulse + sub, chorus I, mono, filter envelope, bright but not harsh. The classic singing lead. | Lead, Chorus, 80s | Bloom |
| `classic.lead.eight-voice-sync` | Eight Voice Sync | C | OB-X sync lead | Hard-synced oscillator 2, ladder low-pass, fast filter envelope, mono, drive. Aggressive sync sweep on velocity. | Sync, Aggressive, Mono | Grid |
| `modern.lead.laser-bloom` | Laser Bloom | M | Modern laser lead | Wavetable + saw, unison 5, high resonance, phaser, delay, pitch bend. Bright laser bloom with a controlled core. | Laser, Wide, Modern | Bloom |
| `modern.lead.chrome-siren` | Chrome Siren | M | Modern siren | Wavetable, LFO pitch vibrato, ring modulation, tempo delay, stereo. Expressive and metallic. | Siren, Motion, Chrome | Ribbon |
| `modern.lead.hyper-lead` | Hyper Lead | M | Modern hyper lead | Saw + square, unison 3, drive, chorus, delay, 60 ms glide. Loud, wide, and forward. | Hyper, Bright, Wide | Prism |

### 2.5 Plucks & Arps

| ID | Name | Coll | Lineage | Design target | UI tags | Motif |
| :--- | :--- | :-: | :--- | :--- | :--- | :--- |
| `classic.pluck.106-pluck` | 106 Pluck | C | Juno-106 pluck | Pulse + sub, fast filter envelope, chorus I, short release, stereo delay. Round and chorus-rich. | Pluck, Retro, Chorus | Caustic |
| `classic.pluck.five-voice-arp` | Five Voice Arp | C | Prophet-5 arp | Two saws, ladder, arp up at 1/16, short gate, delay, no reverb. Tight analog sequence. | Arp, Analog, Rhythmic | Grid |
| `classic.pluck.six-voice-bells` | Six Voice Bells | C | Polysix bells | Sine + pulse, ring modulation, high-pass, plate, random arp pattern, long tail. Crystalline but vintage. | Bells, Arp, Crystalline | Halo |
| `modern.pluck.glass-drop` | Glass Drop | M | Modern glass pluck | Wavetable, pitch modulation from filter envelope, short amp decay, plate, stereo spread. A single glass droplet. | Glass, Drop, Modern | Caustic |
| `modern.pluck.crystal-arp` | Crystal Arp | M | Modern crystal arp | Wavetable, arp up/down at 1/16, tempo delay, chorus, bright. Clean and sparkling. | Arp, Crystal, Rhythmic | Prism |
| `modern.pluck.kinetic-pluck` | Kinetic Pluck | M | Modern kinetic pluck | Pulse + noise, filter envelope, phaser, arp at 1/8, gated, drive. Mechanical and propulsive. | Kinetic, Pluck, Motion | Grid |

### 2.6 Brass & Strings

| ID | Name | Coll | Lineage | Design target | UI tags | Motif |
| :--- | :--- | :-: | :--- | :--- | :--- | :--- |
| `classic.brass.five-voice-brass` | Five Voice Brass | C | Prophet-5 brass | Two saws, ladder around 2 kHz, fast filter envelope with velocity, no chorus, light plate. Bold and articulate. | Brass, Analog, Bold | Halo |
| `classic.brass.eight-voice-brass` | Eight Voice Brass | C | OB-X brass | Pulse + saw, SVF, pronounced filter envelope, ensemble, wide. Big 1980s brass section. | Brass, Retro, Wide | Ribbon |
| `classic.brass.106-strings` | 106 Strings | C | Juno-106 strings | Saw + sub, chorus II, slow attack, long release, high-pass, ensemble. Warm string machine. | Strings, Chorus, Warm | Aurora |
| `modern.brass.neon-brass` | Neon Brass | M | Modern brass | Wavetable + saw, drive, filter envelope, chorus, delay. Bright and synthetic. | Brass, Modern, Neon | Bloom |
| `modern.brass.velvet-strings` | Velvet Strings | M | Modern velvet strings | Wavetable + saw, slow attack, chorus, plate, aftertouch to cutoff. Soft and cinematic. | Strings, Velvet, Cinematic | Caustic |
| `modern.brass.chrome-ensemble` | Chrome Ensemble | M | Modern ensemble | Two wavetables, unison 4, chorus, phaser, long release, wide. A chrome-plated string/brass hybrid. | Ensemble, Chrome, Wide | Ribbon |

### 2.7 Textures & Atmos

| ID | Name | Coll | Lineage | Design target | UI tags | Motif |
| :--- | :--- | :-: | :--- | :--- | :--- | :--- |
| `classic.texture.106-drift` | 106 Drift | C | Juno-106 drift | PWM pad, chorus, slow LFO to cutoff and pan, long reverb, ~6 s release. Slowly drifting analog cloud. | Drift, Analog, Atmospheric | Tide |
| `classic.texture.five-voice-wind` | Five Voice Wind | C | Prophet-5 wind | Filtered noise + saw, band-pass, slow LFO, phaser, long plate. A synthetic wind field. | Wind, Noise, Evolving | Aurora |
| `classic.texture.eight-voice-choir` | Eight Voice Choir | C | OB-X choir | Pulse + saw, chorus, high-pass, slow attack, huge reverb. Vast and vocal-adjacent. | Choir, Analog, Vast | Halo |
| `modern.texture.deep-space` | Deep Space | M | Modern drone | Wavetable drone, sub, long reverb, delay, slow filter motion, stereo. For long, weightless beds. | Drone, Deep, Cinematic | Tide |
| `modern.texture.glass-rain` | Glass Rain | M | Modern stochastic glass | Wavetable + noise, random arp used sparsely, delay, plate, high-pass. Scattered glass droplets in space. | Rain, Stochastic, Glass | Caustic |
| `modern.texture.solstice-drone` | Solstice Drone | M | Modern warm drone | Two wavetables, unison, slow LFO, long reverb, wide but mono-compatible. A warm seasonal drone. | Drone, Solstice, Warm | Bloom |

### 2.8 Motion & Rhythmic

| ID | Name | Coll | Lineage | Design target | UI tags | Motif |
| :--- | :--- | :-: | :--- | :--- | :--- | :--- |
| `classic.motion.106-gate` | 106 Gate | C | Juno-106 gate | PWM pad, arp gated at 1/16, chorus, delay, rhythmic. The classic gated pad without a trance gate UI. | Gated, Retro, Rhythmic | Grid |
| `classic.motion.five-voice-pulse` | Five Voice Pulse | C | Prophet-5 pulse | Saw, LFO-to-amp pulse, delay, moderate filter movement. Mechanical but warm. | Pulse, Analog, Motion | Tide |
| `classic.motion.six-voice-motion` | Six Voice Motion | C | Polysix motion | Pulse pad, arp up/down, filter LFO, ensemble, plate. Restless and melodic. | Motion, Arp, Retro | Bloom |
| `modern.motion.kinetic-grid` | Kinetic Grid | M | Modern kinetic grid | Wavetable, arp at 1/16, filter envelope, phaser, delay, staccato gate. Tight and architectural. | Grid, Kinetic, Modern | Grid |
| `modern.motion.neon-sequence` | Neon Sequence | M | Modern sequence | Wavetable + saw, sequential arp, tempo delay, chorus, bright. A neon-lit pattern generator. | Sequence, Neon, Rhythmic | Prism |
| `modern.motion.tidal-motion` | Tidal Motion | M | Modern tidal motion | Wavetable, slow LFO to cutoff/pan, delay, plate, arp at 1/8. Long evolving rhythm. | Tidal, Evolving, Wide | Tide |

---

## 3. Bank Totals (Test Assertions)

| Check | Expected |
| :--- | :--- |
| Total presets | 48 |
| Groups | 8 |
| Presets per group | 6 |
| Classic presets | 24 |
| Modern presets | 24 |
| Classic per group | 3 |
| Modern per group | 3 |
| Default preset | `classic.keys.five-voice-keys` |
| Unique IDs | 48 |
| Unique display names | 48 |

The preset tests must assert every row above, including the exact count per
group, not just the total.

---

## 4. Artwork Direction

Each group has a base glass palette; the individual preset's motif and
`seed = fnv1a(id)` make its image unique. Classic presets use lower `energy`
(0.35–0.55), higher `haze` (0.55–0.8), and warmer accents. Modern presets use
higher `energy` (0.6–0.9), lower `haze` (0.25–0.55), and cooler or more
iridescent accents.

| Group | Base gradient | Primary accent | Secondary accent | Motif pool |
| :--- | :--- | :--- | :--- | :--- |
| Bass | `#03060D → #081426 → #010307` | `#5BD7FF` cyan | `#7C6BFF` violet | Prism, Grid, Tide, Halo, Ribbon |
| Pads | `#060512 → #1A1036 → #020208` | `#C7A6FF` lavender | `#63F5D6` mint | Aurora, Halo, Ribbon, Bloom, Caustic |
| Keys | `#0A0807 → #241A10 → #050302` | `#F6D9A8` champagne | `#A8D8FF` ice | Prism, Grid, Caustic, Halo |
| Leads | `#0C0308 → #2A0717 → #050104` | `#FF4D8D` hot pink | `#FFB15C` amber | Bloom, Ribbon, Grid, Prism |
| Plucks & Arps | `#020A0A → #052322 → #010505` | `#62FFD8` aqua | `#B6FF7A` lime | Caustic, Grid, Prism, Halo |
| Brass & Strings | `#0B0503 → #2B1407 → #050201` | `#FFC46B` gold | `#FF7A45` copper | Halo, Ribbon, Aurora, Bloom, Caustic |
| Textures & Atmos | `#020409 → #071A22 → #010205` | `#8AE8FF` glacier | `#A6FFCB` seafoam | Tide, Aurora, Halo, Caustic, Bloom |
| Motion & Rhythmic | `#05030A → #11142E → #020106` | `#8CFF6A` acid | `#5EC8FF` electric blue | Grid, Tide, Bloom, Prism |

Rules:
- Never use the same combination of motif, seed, and accents for two presets.
- Keep the hero image behind a localized dark scrim; legibility beats drama.
- If a bundled raster image replaces procedural art, it must be 1600×900 or
  larger, licensed for redistribution, and accompanied by attribution. Do not
  ship one background reused across presets.
- The rail accent, selected-row tint, focus ring, and status dot follow the
  selected preset's primary accent.

### Deterministic seed

```cpp
constexpr std::uint32_t fnv1a(const char* s) noexcept
{
    std::uint32_t h = 2166136261u;
    while (*s != '\0')
    {
        h ^= static_cast<std::uint32_t>(static_cast<unsigned char>(*s++));
        h *= 16777619u;
    }
    return h;
}

constexpr float seedFromId(const char* id) noexcept
{
    // 24 bits keeps 48 preset seeds collision-free in practice while staying
    // exactly representable in a float's mantissa.
    return static_cast<float>(fnv1a(id) & 0xffffffu) / 16777215.0f;
}
```

Use the same hash for per-voice deterministic drift, but mix in the note/voice
index so voices differ while renders stay reproducible.

---

## 5. Sound-Design Starting Points

These are calibrated starting points, not final values. Tune by ear, then
re-run the loudness and peak tests. Keep every preset's peak at or below
−1.0 dBFS on a 16-voice chord.

### 5.1 `classic.pad.106-dream` (Juno-style pad)

```text
Osc1: pulse, PW 0.42, level 0.55
Osc2: saw, fine +7 cents, level 0.45
Sub:  0.28, square
Filter: SVF low-pass, 1.8 kHz, resonance 0.18, env +1.2 oct, keytrack 0.3
Filter env: A 180 ms, D 900 ms, S 0.55, R 2200 ms
Amp env: A 1200 ms, D 1400 ms, S 0.85, R 3200 ms
LFO1: sine 0.28 Hz → pulse width 1 and cutoff 0.12, per-voice
Chorus: mode II, depth 0.65, mix 0.40
Reverb: plate, size 0.62, decay 0.55, mix 0.22
Drift: 0.35
```

### 5.2 `classic.keys.five-voice-keys` (default)

```text
Osc1: saw, level 0.50
Osc2: saw, fine −6 cents, level 0.50
Filter: ladder, 3.0 kHz, resonance 0.12, env +0.9 oct, keytrack 0.5
Filter env: A 2 ms, D 500 ms, S 0.25, R 450 ms
Amp env: A 3 ms, D 600 ms, S 0.30, R 500 ms
Chorus: off
Delay: 320 ms, feedback 0.18, mix 0.10
Reverb: plate, mix 0.12
Drift: 0.15
```

### 5.3 `modern.bass.glass-sub` (clean sub)

```text
Osc1: sine, level 0.60, octave −1
Osc2: wavetable, position 0.18, level 0.25, octave 0
Sub:  0.35, sine
Filter: ladder, 95 Hz, resonance 0.10, keytrack 0.25
Amp env: A 4 ms, D 180 ms, S 0.80, R 220 ms
Drive: 0.05
Chorus: off
Reverb: off
Voice mode: mono, legato, glide 0 ms
Output trim: calibrate to −1 dBFS peak
```

### 5.4 `modern.pad.aurora-glass` (wavetable pad)

```text
Osc1: wavetable, position 0.35, level 0.45
Osc2: saw, fine +9 cents, level 0.40
Unison: 5 voices, 12 cents, spread 0.75
Filter: SVF low-pass, 2.4 kHz, resonance 0.22, env +1.4 oct
Filter env: A 800 ms, D 1800 ms, S 0.45, R 3000 ms
Amp env: A 1400 ms, D 2000 ms, S 0.90, R 4200 ms
LFO1: sine 0.12 Hz → wavetable position 0.25, cutoff 0.15, per-voice
Chorus: mode III, depth 0.55, mix 0.30
Delay: 520 ms, feedback 0.30, mix 0.18
Reverb: plate, size 0.78, decay 0.72, mix 0.30
```

### 5.5 `classic.brass.five-voice-brass`

```text
Osc1: saw, level 0.50
Osc2: saw, fine +5 cents, level 0.50
Filter: ladder, 2.0 kHz, resonance 0.15, env +2.0 oct, velocity 0.5, keytrack 0.35
Filter env: A 60 ms, D 500 ms, S 0.40, R 500 ms
Amp env: A 20 ms, D 400 ms, S 0.80, R 600 ms
Chorus: off
Reverb: plate, mix 0.10
Drift: 0.20
Velocity → filter env and amp
```

### 5.6 `modern.motion.kinetic-grid`

```text
Osc1: wavetable, position 0.62, level 0.55
Osc2: pulse, PW 0.28, level 0.30
Filter: ladder, 1.4 kHz, resonance 0.30, env +2.2 oct
Filter env: A 1 ms, D 140 ms, S 0.15, R 120 ms
Amp env: A 1 ms, D 180 ms, S 0.00, R 120 ms
Arp: on, 1/16, up, octaves 2, gate 0.35, not latched
LFO1: square 1/8 sync → cutoff 0.12
Phaser: 4 stages, mix 0.18
Delay: 1/8 sync, feedback 0.28, mix 0.20
Reverb: plate, mix 0.12
```

For all presets:
- Use velocity where it improves expression, but do not make soft notes
  inaudible.
- Use aftertouch/mod wheel only where the preset description implies expression
  (leads, pads, strings). Do not add hidden global expression.
- Arpeggios are allowed only in Plucks & Arps and Motion & Rhythmic groups, plus
  `modern.texture.glass-rain`. Do not surprise the player with an arp in a pad
  or keys preset.
- Long drones and infinite-release textures must be documented exceptions to the
  "decays below −90 dBFS in 10 s" test; cap them so they do not accumulate DC or
  infinite energy.

---

## 6. Preset Metadata Example

```cpp
static constexpr Preset k106Dream {
    "classic.pad.106-dream",
    "106 Dream",
    "1980s six-voice inspired lush chorus pad",   // internal only
    "A slow, chorus-rich pad inspired by the classic 1980s six-voice sound.",
    { "Lush", "Chorus", "Wide", nullptr },
    Group::Pads,
    Collection::Classic,
    ArtworkSpec { 0xff060512, 0xff1a1036, 0xff020208,
                  0xffc7a6ff, 0xff63f5d6, Motif::Aurora,
                  seedFromId("classic.pad.106-dream"), 0.48f, 0.72f },
    make106DreamPatch()
};
```

- `tags` is a fixed 4-slot array with `nullptr` termination. The UI shows the
  non-null entries.
- `description` is one sentence, ≤ 110 characters, present tense, no marketing
  claims or artist names.
- `lineage` is short and **internal only**; never render it. The UI shows the
  name, description, and tags. Internal lineage may mention reference instruments.
- `ArtworkSpec` is compile-time data; no image loading on the audio thread.

---

## 7. Loudness Matching and Validation

### Offline calibration

For every preset, render:

1. C4 note-on, 4 s, release 2 s.
2. C3–G4 eight-note chord, 4 s, release 2 s.
3. A short 1/16 arp phrase if the preset uses the arp.

At 48 kHz, stereo, with no external processing. Measure:

- **Sample peak** over the whole render → must be ≤ −1.0 dBFS.
- **Integrated loudness** (ITU-R BS.1770 / `juce::dsp::LoudnessMeter` or an
  equivalent offline implementation) → the 48 presets must sit within a
  ±1.5 LU window.
- **DC offset** after 1 s of silence → |DC| ≤ 0.001.
- **NaN/Inf** → none.
- **Release** → below −90 dBFS within 10 s, except documented drones.

Adjust `outputTrimDb` first. Do not add compression or limiting just to pass the
test; fix the patch gain staging. The output safety clipper is a final guard,
not the mix engineer.

### Test thresholds

- Peak: `<= -1.0 dBFS` (linear ≤ 0.891251).
- Loudness spread: max preset LU − min preset LU ≤ 3.0 LU, with each preset
  within ±1.5 LU of the bank median.
- Preset-switch click: compare the max sample-to-sample delta during the switch
  against the preset's steady-state max delta; fail on a clear discontinuity
  (e.g. > 0.25 absolute at normal levels, or > 2× the steady-state maximum).
- No preset may alias above −60 dBFS at C6 for its raw oscillator layer.

### Bank validation unit test

```cpp
auto& bank = getFactoryBank();
check (bank.size() == 48);
check (std::count_if (bank.begin(), bank.end(),
       [] (auto& p) { return p.collection == Collection::Classic; }) == 24);
check (std::count_if (bank.begin(), bank.end(),
       [] (auto& p) { return p.collection == Collection::Modern; }) == 24);

for (auto g : allGroups)
{
    check (std::count_if (bank.begin(), bank.end(),
           [g] (auto& p) { return p.group == g; }) == 6);
    check (std::count_if (bank.begin(), bank.end(),
           [g] (auto& p) { return p.group == g
                              && p.collection == Collection::Classic; }) == 3);
}

std::set<std::string> ids, names;
for (auto& p : bank)
{
    check (ids.insert (p.id).second);
    check (names.insert (p.name).second);
    check (p.description != nullptr && *p.description != '\0');
}
check (findPresetById ("classic.keys.five-voice-keys") != nullptr);

std::set<float> seeds;
for (auto& p : bank)
    check (seeds.insert (p.artwork.seed).second);
```

---

## 8. UI Copy and Editorial Voice

Preset text should feel like a high-end instrument, not a plugin manual.

- **Name**: 1–3 words, title case, no version numbers.
- **Lineage**: 3–7 words, engineering note only, e.g. "1980s six-voice lush
  chorus pad". Never render it or use another company's name/logo in UI copy.
- **Description**: one sentence, ≤ 110 characters. Describe the sound and use
  case, not parameter values.
- **Tags**: 2–4 chips, title case, from the table.
- **No emoji, no exclamation marks, no "best", "ultimate", or artist names.**

Examples:

```text
"A slow, chorus-rich pad inspired by the classic 1980s six-voice sound."
"Two detuned saws and a round ladder filter for warm, articulate poly brass."
"A clean sine sub with just enough wavetable edge to cut through a modern mix."
"Scattered glass droplets suspended in a long, high plate reverb."
```

Do not display parameter values, filter cutoffs, or "analog modeling" claims in
the UI. The preset is the promise; the engine is the implementation.

---

## 9. Future Bank Revisions

- Adding presets is allowed in a minor release; never renumber or reuse IDs.
- Removing a preset requires a migration mapping to a surviving ID.
- Changing a preset's sound is allowed only with a changelog note and a new
  audit of peak/loudness; sessions should not be silently altered.
- If a "v2" bank grows, keep the eight-group structure and maintain the
  Classic/Modern balance; do not turn the preset rail into an endless flat list.
- User presets are out of scope for v1. Revisit the product contract before
  adding save/import UI.
