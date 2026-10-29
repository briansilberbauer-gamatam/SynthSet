---
name: vst3-polysynth
description: >-
  Build SynthSet, a preset-only 16-voice polyphonic VST3/Standalone synthesizer
  in JUCE 8.0.8 (C++20). Covers classic 1970s-80s-inspired and modern
  oscillator/filter/LFO/effects architecture, a fixed 48-preset bank in eight
  groups with Classic/Modern collections, modern liquid-glass UI with
  per-preset reactive artwork, real-time-safe preset switching, offline
  Linux build/test tooling, and GitHub Actions CI releasing versioned Windows,
  Linux, and macOS Universal distributions.
user-invocable: true
---

# SynthSet — Preset-Only Polyphonic VST3 Synthesizer Skill

This skill defines how to design and implement **SynthSet**: a polyphonic
synthesizer with a deep classic/modern synthesis engine whose *entire public
interface is a factory preset library*. It is the project skill for
`SynthSet`. Read the companion references before writing engine or UI code.

**Companion references**

- `references/synth-dsp-architecture.md` — C++ architecture, voice allocation,
  PolyBLEP oscillators, TPT/ZDF filters, analog envelopes, LFOs and modulation,
  BBD-style chorus, phaser, delay, Dattorro plate reverb, preset switching, and
  the mandatory test matrix. **Read before implementing DSP.**
- `references/preset-library.md` — the exact 48-preset bank, group taxonomy,
  stable IDs, sound-design targets, per-preset artwork direction, loudness
  normalization, and preset validation. **Read before touching preset data.**
- `references/look-and-feel-recipes.md` — modern liquid-glass drawing recipes,
  per-preset artwork generation, preset rail, hero stage, typography, animation,
  scaling, and headless screenshot verification. **Read before implementing UI.**
- `references/github-actions-ci-release.md` — complete `.github/workflows/build.yml`
  for Windows x64, Linux x64, macOS Universal, versioned ZIPs, SHA-256
  checksums, and GitHub Releases. **Read before writing CI or packaging.**

---

## 1. The Product Contract

### 1.1 Presets are the product

SynthSet is **not** a subtractive-synth editor. It is a curated instrument: the
user auditions finished sounds, not parameter states.

Mandatory consequences:

1. `AudioProcessor::getNumParameters()` returns **0**. There is no APVTS, no
   hidden parameter tree, and no automation lane. Do not add an "invisible"
   control and call it a preset value.
2. The editor contains **no knobs, sliders, toggles, XY pads, envelope handles,
   or editable numeric fields**. It may contain text, artwork, collection/group
   navigation, a preset list, previous/next buttons, keyboard focus handling,
   and read-only status (voice count, MIDI activity, preset tags).
3. The only persistent state is `{schemaVersion, presetId}`. If the ID no longer
   exists, fall back to the bank's default preset. Never serialize the patch
   itself into host state.
4. Presets are selected by UI click, host program list, MIDI program change, or
   previous/next step. Host automation of preset choice is not required.
5. The bank is read-only and compiled into the binary. Do not add user-save,
   import, export, favourites, ratings, tags editing, or "init patch" flows in
   v1.
6. There is one screen. No settings sheet, no About panel, no preferences, no
   licensing dialog. Put company/version information in the plugin metadata and
   a small non-interactive footer if needed.
7. Collection/group filters are transient browse state. They are never
   persisted, never exposed to the host, and never change the sound.

If a requirement appears to need a control, encode the difference in presets
instead. If the owner explicitly changes this contract in a later request, update
this skill first; do not quietly violate it.

### 1.2 One preset = one immutable `SynthPatch`

A preset must contain every sound-determining value: two oscillators, sub,
noise, oscillator sync/ring/PWM, drift, per-voice filter model and routing,
filter envelope, amp envelope, two LFOs, eight fixed modulation routes, voice
mode/unison/glide, arpeggiator, chorus/phaser/delay/reverb, saturation, and
output trim. The `SynthPatch` is trivially copyable, contains no heap-owning
types, and is never mutated by the audio thread.

### 1.3 Preset change is a first-class audio event

A preset switch must be musical and click-free:

- `setCurrentProgram()` / UI click / program change posts a request atomically.
- The audio thread notices at the next block, fades the engine output over
  **10–25 ms**, resets voices and effect tails, swaps to the new immutable
  patch, and fades back in over **10–25 ms**.
- No allocation, locking, `juce::String`, JSON parsing, or image work occurs on
  the audio thread.
- The UI receives the new selection asynchronously and crossfades the artwork.

---

## 2. Model & Tool Routing in DeepSeek Harness

Split work by reasoning depth:

| Area | Model | Responsibility |
| :--- | :--- | :--- |
| Oscillators, filters, voice allocation, modulation, effects, click-free switching | **DeepSeek V4 Pro** (`deepseek-pro` / `deepseek-reasoner`) | Numerical stability, aliasing, ZDF equations, preset crossfade, real-time safety. |
| Liquid-glass `LookAndFeel`, artwork generator, interaction/animation | **DeepSeek V4 Pro** | Composition, procedural art, typography, performance-conscious painting. |
| CMake targets, patch structs, preset tables, test harnesses, build scripts | **DeepSeek V4 Flash** (`deepseek-flash`) | Mechanical scaffolding, repetitive preset data, CI YAML, build execution. |
| Debug loops, visual QA, packaging | **DeepSeek V4 Flash** | Run builds/tests, render screenshots, inspect logs, iterate on layout. |

Use `todo_write` for multi-step work. Use `subagent` / `subagent_fork` to keep
large DSP and UI files out of the main context. Use `workflow` only for genuinely
parallel fan-out. Follow the `dsh` read-before-write rule on every existing file.

---

## 3. Dependency Policy

- **JUCE 8.0.8** is the framework and the only required third-party dependency.
  Use `juce_dsp` for FFT/analysis, biquads, oversampling utilities, and
  convolution/reverb primitives where they fit; write the voice, PolyBLEP
  oscillator, ZDF filter, modulation matrix, and preset engine explicitly.
- **No third-party synth engine** is needed for v1. A bundled engine adds
  license, binary-size, build-reproducibility, and real-time-control risk without
  guaranteeing a better classic polyphonic character.
- **Optional acceleration only after profiling**: `xsimd` or JUCE's SIMD helpers
  may be added for oscillator/filter batching if the 16-voice CPU target is
  missed. Do not add them speculatively; they complicate Windows/macOS builds and
  are not required for the design.
- **No runtime downloads, sample packs, or model files.** Factory presets and
  tables are compiled in. The OFL font is bundled. This keeps CI deterministic
  and the plugin usable offline.
- If a dependency is proposed, document its license (must be compatible with a
  redistributable plugin), why custom/JUCE code is insufficient, its binary-size and
  build impact, and how it behaves under `prepareToPlay()` without allocations.

---

## 4. Recommended Repository Layout

```text
SynthSet/
├── .dsh/skills/vst3-polysynth/          # This skill (working copy)
├── .agents/skills/vst3-polysynth/       # Mirror for other agent runtimes
├── .github/workflows/build.yml
├── AGENTS.md
├── CHANGELOG.md
├── CMakeLists.txt
├── README.md
├── Resources/
│   ├── LatoRegular.ttf
│   ├── LatoSemibold.ttf
│   ├── LatoBlack.ttf
│   └── Lato-OFL.txt
├── Source/
│   ├── PluginProcessor.h/.cpp
│   ├── PluginEditor.h/.cpp
│   ├── Version.h
│   ├── preset/
│   │   ├── SynthPatch.h          # Plain data: every sound-determining field
│   │   ├── Preset.h              # ID, name, group, collection, copy, art
│   │   ├── PresetBank.h/.cpp     # Exactly 48 immutable factory presets
│   │   └── PresetManager.h/.cpp  # Program list, atomic selection, state
│   ├── dsp/
│   │   ├── SynthEngine.h/.cpp    # Voices, MIDI, arp, block processing
│   │   ├── Voice.h/.cpp          # Per-voice oscillators/filter/envelopes/LFO
│   │   ├── Oscillator.h/.cpp     # PolyBLEP + wavetable + sync
│   │   ├── Filters.h/.cpp        # TPT SVF, ZDF ladder, HPF
│   │   ├── Envelope.h/.cpp
│   │   ├── Lfo.h/.cpp
│   │   ├── ModMatrix.h
│   │   ├── Chorus.h/.cpp
│   │   ├── Phaser.h/.cpp
│   │   ├── Delay.h/.cpp
│   │   ├── PlateReverb.h/.cpp
│   │   └── OutputStage.h/.cpp
│   └── ui/
│       ├── Theme.h/.cpp          # Glass tokens + per-preset visual specs
│       ├── GlassLookAndFeel.h/.cpp
│       ├── ArtworkStage.h/.cpp   # Procedural per-preset hero image/crossfade
│       ├── PresetRail.h/.cpp     # Collection filter, groups, preset rows
│       ├── HeroPanel.h/.cpp      # Name, description, tags, char line
│       └── StatusStrip.h/.cpp    # Voices, MIDI, version
├── Tests/DspTests.cpp
├── Tools/HostSmokeTest.cpp
├── Tools/RenderEditor.cpp
└── scripts/
    ├── setup-linux.sh
    ├── build.sh
    └── render-ui.sh
```

Do not create `Parameters.h`, an APVTS, or a `Source/ui/Knob.*`.

---

## 5. Sound Engine Summary

The full, implementable specification is in
`references/synth-dsp-architecture.md`. The non-negotiable shape:

### 4.1 Voices

- **16 voices**, allocated in `prepareToPlay()`. Note-on is sample-accurate from
  the `MidiBuffer`; sustain pedal, pitch bend, mod wheel, and channel pressure
  are handled without allocation.
- Voice modes baked per preset: Poly, Mono, Legato, Unison (2–7 layers).
  Unison presets reduce polyphony so total oscillator count stays bounded.
- Stealing priority: oldest released voice, then quietest voice, then oldest
  sounding voice. Apply a 3–5 ms fade on stolen voices.
- Deterministic per-voice drift/tolerance from the preset seed: tiny pitch,
  cutoff, envelope, and stereo-position offsets. Same note must sound slightly
  different per voice, never randomly inconsistent between renders.

### 4.2 Oscillators and mix

- Two primary oscillators per voice with continuous morph across saw, pulse,
  triangle, sine, and (for modern presets) band-limited wavetable.
- Pulse width modulation, hard sync, ring modulation, sub oscillator (square or
  sine), and white/pink noise. Per-oscillator level, octave, semitone, fine
  tune, stereo spread, and drift.
- PolyBLEP anti-aliasing for discontinuous waveforms; mip-mapped tables for
  wavetable mode. No naive saw/pulse at high notes.
- Classic presets lean on two-oscillator detune, pulse-width motion, and
  Juno-style sub/chorus. Modern presets add wavetable position, unison, and
  spectral movement.

### 4.3 Filters

- Selectable per preset: **4-pole ZDF ladder** (warm, saturating, self-oscillating
  at high resonance) and **2-pole TPT state-variable filter** (LP/HP/BP,
  smoother/brighter character), plus a separate 1-pole or 2-pole high-pass.
- Filter envelope with amount and keytracking, velocity sensitivity, drive with
  output compensation, and nonlinear feedback stabilized at 44.1–192 kHz.
- Coefficient updates at control rate with parameter smoothing; cutoff and
  resonance must never zipper or blow up.

### 4.4 Envelopes, LFOs, modulation

- Two analog-style ADSR envelopes per voice (filter, amp) with exponential
  segments and preset-defined curves. No click on retrigger.
- Two LFOs: one global/per-voice selectable, shapes sine/triangle/saw/square/
  sample-and-hold, rate, fade-in, phase, and tempo sync where specified.
- Eight fixed modulation routes per preset: source → destination → bipolar
  amount. Sources include both LFOs, both envelopes, velocity, keytrack, mod
  wheel, aftertouch, pitch bend, note-on random. Destinations include pitch,
  both pulse widths, wavetable position, filter cutoff, resonance, amp, pan,
  chorus depth, delay send, reverb send.
- The matrix is compiled per preset and evaluated without branches that allocate
  or iterate over strings.

### 4.5 Effects and output

- **Juno-style BBD chorus/ensemble** (two modes plus off), 4/6-stage phaser,
  stereo filtered delay, and a Dattorro-style plate reverb. All buffers are
  preallocated; all time changes are smoothed.
- Arpeggiator/pattern per preset, host-tempo synced, with latch/range/rate/
  pattern baked into the patch. No UI for it.
- Output stage: DC blocker, gentle analog saturation, safety soft clip,
  per-preset trim. Enforce **peak ≤ −1.0 dBFS** for a 16-voice chord.
- No dynamic latency. Report `0` samples unless a fixed, always-on stage truly
  introduces latency. Never change reported latency when presets change.

### 4.6 Real-time rules

- `juce::ScopedNoDenormals` at the top of `processBlock()`.
- Zero heap allocation, locks, file access, logging, or `juce::String` in the
  audio callback.
- Read the immutable patch once per block into a local `const SynthPatch&`;
  never read host-owned or UI-owned memory unsynchronized.
- No per-sample `std::function`, virtual dispatch across the patch model, or
  denormal-prone recursive filters without a DC offset/flush strategy.
- `prepareToPlay()` preallocates all delay lines, wavetables, scratch buffers,
  and voice structures; `releaseResources()` frees only when playback stops.

---

## 6. Preset Bank Requirements

The exact 48 presets and their sound targets are defined in
`references/preset-library.md`. Follow that table.

### 5.1 Taxonomy

Eight groups, in this stable display order:

1. **Bass**
2. **Pads**
3. **Keys**
4. **Leads**
5. **Plucks & Arps**
6. **Brass & Strings**
7. **Textures & Atmos**
8. **Motion & Rhythmic**

Two collections: **Classic** (analog heritage) and **Modern** (contemporary
glass/spectral/motion). Exactly six presets per group: three Classic and three
Modern. Totals: **48 presets, 24 Classic, 24 Modern**.

### 5.2 IDs and display

- Stable ID format: `classic.<group>.<slug>` or `modern.<group>.<slug>`, e.g.
  `classic.pad.106-dream`, `modern.bass.glass-sub`.
- IDs are never reused, never reordered semantically, and never derived from a
  row index. Display names may change; IDs may not.
- Each preset carries: `id`, `name`, `group`, `collection`, `lineage` (internal
  engineering inspiration only, never rendered), `description` (one sentence
  shown in UI), `tags` (2–4 short chips), `ArtworkSpec`, and the `SynthPatch`.
- Display names and all user-facing copy must avoid other manufacturers'
  trademarks. Use "106", "Five Voice", "Eight Voice", "Sunset Poly", etc.
  Internal lineage notes may say "Juno-106-inspired" for engineers; the UI says
  "inspired by the classic 1980s six-voice sound" or similar.

### 5.3 Default and ordering

- Default preset: `classic.keys.five-voice-keys`.
- Within a group, sort Classic before Modern, then by the table order (which is
  curated, not alphabetical).
- Host program indices follow the flattened bank order. Persist the stable ID,
  not the index, because future bank revisions may insert presets.

### 5.4 Loudness and safety

All presets must be level-matched:

- Render each preset with a fixed test MIDI phrase at 48 kHz: single C4 note,
  8-note chord, and a 2-second release.
- Enforce sample peak ≤ **−1.0 dBFS**, no NaN/Inf, and no DC above 0.001.
- Integrated loudness across presets within a **±1.5 LU** window; use per-patch
  `outputTrim` to correct rather than compressing the patch.
- No preset may rely on the host limiter. The plugin's own safety stage catches
  accidents, but it must not be the primary level control.

---

## 7. Modern Liquid-Glass UI

The complete drawing recipes are in
`references/look-and-feel-recipes.md`. The visual contract:

### 6.1 Composition

- Fixed **1280 × 800** design canvas, 16:10, resizable 960×600–1920×1200.
  Lay out in design coordinates and apply one `AffineTransform` in
  `AudioProcessorEditor::resized()`.
- Full-bleed animated art field; dark scrim for legibility; a **left glass rail**
  (300 px) and a **main hero stage**.
- Left rail: wordmark, collection segmented control (All / Classic / Modern),
  eight group rows with counts, scrollable preset list for the selected group.
  When a host program change or MIDI program change selects a preset outside the
  current filter, switch to its group/collection, reveal the row, and keep the
  audio selection authoritative.
- Main stage: top status strip (group, preset index, voice count, MIDI dot),
  large hero artwork, preset name, collection badge, description, 2–4 tags,
  read-only recipe chips (e.g. `2 OSC`, `LADDER LP`,
  `BBD CHORUS`), and previous/next arrows.
- No controls that alter sound. A small non-interactive footer may show
  `SYNTHSET v1.0 · BLS AUDIO`.

### 6.2 Glass system

- Deep near-black blue/graphite base (`#05070C`–`#0B1119`), not flat gray.
- Glass panels: 20–28 px corner radius, vertical translucent fill, 1 px inner
  top-left highlight, faint outer rim, subtle backdrop blur/refraction, and
  gentle noise grain. Avoid heavy drop shadows and "plastic" bevels.
- Per-preset accent color drives the hero bloom, active row, focus ring, and
  status dot. Use a restrained accent (not full neon saturation).
- Typography: bundled Lato (Regular/Semibold/Black), large light preset name
  (42–54 px), tracked uppercase 9–11 px labels, tabular numerals. Keep body copy
  at ≥ 13 px and contrast ≥ 4.5:1 against its local scrim.
- Micro-motion: parallax/art drift ≤ 0.1 Hz; 150–250 ms hover; 300–450 ms
  artwork crossfade. No bouncing, pulsing text, or continuous full-editor
  repaint.

### 6.3 Per-preset artwork is mandatory

Each preset defines an `ArtworkSpec`:

```cpp
struct ArtworkSpec
{
    juce::uint32 top, mid, bottom;   // background gradient stops
    juce::uint32 accent, accent2;    // bloom / secondary refraction
    Motif motif;                     // Aurora, Prism, Bloom, Ribbon, Caustic, Grid, ...
    float seed;                      // deterministic shape placement
    float energy;                    // 0..1 bloom / line intensity
    float haze;                      // 0..1 diffusion
};
```

`ArtworkStage` renders a cached 2x image from the spec: layered radial blooms,
glass ribbons, caustic arcs, fine grain, and a subject motif. The palette,
geometry, and seed must be visibly unique per preset; never reuse the same hero
image for two presets. Crossfade old and new images on selection. Code-generated
art is the default; optional bundled WebP art may replace it only if every
preset has an image and licensing is documented.

---

## 8. Build, Tests, and Local Verification

### 7.1 CMake targets

Follow the proven sibling pattern:

- `juce_add_plugin(SynthSet ...)` with `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`,
  `NEEDS_MIDI_OUTPUT FALSE`, `FORMATS VST3 Standalone`,
  `VST3_CATEGORIES "Instrument|Synth"`, `COPY_PLUGIN_AFTER_BUILD FALSE`.
- `SynthSetTests` console app (DSP + preset bank + state + loudness).
- `SynthSetHostSmokeTest` console app linking `juce_audio_utils` /
  `juce_gui_extra` with `JUCE_PLUGINHOST_VST3=1`.
- `SynthSetScreenshot` GUI app reusing editor sources, with `--preset` and
  `--out` arguments.
- Asset target for Lato TTFs and license; no per-preset raster assets unless
  intentionally bundled.
- Hybrid JUCE resolution: `add_subdirectory(external/JUCE)` if present, else
  `FetchContent` JUCE 8.0.8. Do **not** define `JUCE_DISPLAY_SPLASH_SCREEN=0`.

### 7.2 Mandatory tests

At minimum, cover:

1. `getNumParameters() == 0`, exactly 48 programs, unique IDs/names, eight
   groups, 24/24 collection split, all presets non-empty.
2. All presets render finite audio at 44.1/48/96/192 kHz, mono and stereo, with
   peak ≤ −1 dBFS on a 16-voice chord.
3. Filter stability at maximum resonance and cutoff extremes; no NaN/Inf.
4. Envelope timing, LFO shapes, modulation destinations, voice stealing, sustain,
   mono/legato/unison behavior.
5. Preset-switch click test: maximum inter-sample delta below a defined
   threshold, no stale effect tails.
6. State round-trip: selected ID restored; unknown ID falls back to default.
7. Host smoke test: VST3 scans, instantiates, exposes 48 programs and 0
   parameters, processes MIDI, switches programs, restores state.

### 7.3 Visual verification

Render at least three presets from different groups/collections (e.g.
`classic.pad.106-dream`, `modern.pad.aurora-glass`,
`modern.bass.glass-sub`) at 2x and call `read_image` on each. Confirm:

- the hero palette and motif visibly change;
- the glass is translucent but text remains legible;
- no accidental knobs/sliders/toggles;
- no clipping, truncation, or overlapping hit zones;
- the selected row, group count, and collection filter all agree.

Call `present` on the strongest preview and include it in the final response.

---

## 9. CI and Versioned Releases

Read `references/github-actions-ci-release.md` before authoring workflow YAML.
Summary:

- Triggers: push to `main`, pull requests, `v*` tags, manual dispatch with an
  optional `release_tag`.
- **Windows x64** (`windows-2022`, static MSVC runtime), **Linux x64**
  (`ubuntu-latest`, Ninja, ALSA/X11/FreeType/Fontconfig dev), **macOS Universal**
  (`macos-15-intel`, `arm64;x86_64`, deployment target 11.0, ad-hoc signed).
- Build Release, run DSP tests, run host smoke test, package:
  - `SynthSet-Windows-x64-v<version>.zip`
  - `SynthSet-Linux-x64-v<version>.zip`
  - `SynthSet-macOS-Universal-v<version>.zip`
  - matching `.sha256` files.
- Upload artifacts on every run. Publish a GitHub Release when a `v*` tag is
  pushed or `release_tag` is supplied manually. Release job alone gets
  `contents: write`.
- Upload the Linux headless UI preview as an artifact named
  `SynthSet-UI-Preview`.

Version source of truth: `project(SynthSet VERSION x.y.z LANGUAGES C CXX)` in
`CMakeLists.txt`. Keep `CHANGELOG.md` aligned with tags; do not tag a version that
does not match the CMake project version.

---

## 10. Definition of Done

A task is complete only when all of these are true:

- The plugin builds as VST3 + Standalone on Linux locally.
- The engine exposes **0 parameters**, 48 programs, and preset-only state.
- All 48 presets are present in the correct 8×6 / 24–24 layout, unique, and
  level-matched.
- All DSP/preset/host tests pass.
- At least three preset screenshots have been visually inspected and show
  distinct artwork and consistent glass styling.
- No audio-thread allocation/locking and no edit controls exist.
- CI YAML is syntactically valid, produces three versioned ZIPs plus checksums,
  and publishes on tags.
- README documents the preset-only concept, group taxonomy, build, tests,
  install paths, and release process.

---

## 11. Anti-Patterns — Reject These

- Adding a cutoff knob "just for sound design" or a master volume "for safety".
- Exposing hidden parameters through APVTS or host automation.
- Saving the whole patch in DAW state; only the preset ID belongs there.
- Using one static background image for the whole plugin.
- Reusing the same hero composition or full palette combination for two presets.
- Tuning presets by ear only and ignoring peak/loudness tests.
- A naive saw/pulse oscillator, an unstable ladder at high resonance, or
  per-sample virtual dispatch.
- Loading JSON or images on the audio thread; parsing presets in `processBlock()`.
- Changing reported latency when switching presets.
- Shipping a `.zip` without a version and SHA-256.
- Using another manufacturer's name/logo in the product or preset UI.
