# SynthSet — DeepSeek Harness (`dsh`) Workspace Instructions

## 1. Mandatory Skill Activation on Turn 1

Before planning, scaffolding, writing C++ DSP/UI code, or running builds in this
repository, **immediately load the project skill** using the `skill` tool:

```json
{ "name": "vst3-polysynth" }
```

Then read the companion references with the `read` tool as needed:

- `.dsh/skills/vst3-polysynth/references/synth-dsp-architecture.md` — voice
  architecture, oscillators, ZDF filters, envelopes, LFOs/modulation, chorus,
  delay, plate reverb, preset switching, real-time safety, and the C++ test
  matrix for a preset-only polyphonic synth.
- `.dsh/skills/vst3-polysynth/references/preset-library.md` — the 48-preset
  factory bank: eight groups, Classic/Modern collections, stable IDs, sound
  design targets, artwork direction, loudness matching, and validation rules.
- `.dsh/skills/vst3-polysynth/references/look-and-feel-recipes.md` — modern
  liquid-glass UI recipes, per-preset reactive artwork, preset rail, hero stage,
  typography, animation, and headless visual verification.
- `.dsh/skills/vst3-polysynth/references/github-actions-ci-release.md` —
  complete GitHub Actions workflow for Windows x64, Linux x64, macOS Universal,
  versioned ZIPs, SHA-256 checksums, and GitHub Releases.

---

## 2. Product Contract (Read This Before Adding Any Control)

**SynthSet is a preset-only polyphonic synthesizer.** The synthesis engine is
deep and fully programmable *internally*, but the public interface exposes
**presets and nothing else**.

- **No settings, no edit page, no knobs, no sliders, no toggles, no macro
  controls, and no automatable parameters.** `getNumParameters()` must remain
  `0`. Do not add an `AudioProcessorValueTreeState` just to hold hidden values.
- The only persistent state is the **selected factory preset ID** plus a schema
  version. Do not save editable parameter blobs.
- The only user actions are: choose a collection filter, choose a group, choose
  a preset, and step to the previous/next preset. Host program changes and MIDI
  program changes select presets.
- The factory bank is read-only and compiled into the plugin. User preset
  saving/import is explicitly out of scope unless the owner requests a later
  version.
- A preset is an immutable, complete `SynthPatch`: oscillators, sub, noise,
  filter model, envelopes, LFOs, fixed modulation routes, voice mode, unison,
  arpeggiator, effects, and output trim. Changing presets is the only way to
  change the sound.
- Keep the preset change click-free: a short audio fade, voice reset, and FX
  tail clear on the audio thread, with no allocation or locking.

This contract is non-negotiable. Do not "helpfully" add a cutoff knob, a master
volume slider, or a settings drawer.

---

## 3. Project & Sound Mandates

- **Plugin**: `SynthSet`, company `BLS Audio`, VST3 + Standalone, instrument,
  stereo out, MIDI in, 16-voice polyphony. Bundle ID `com.blsaudio.synthset`;
  manufacturer code `Blsa`; plugin code `Sset`.
- **Framework**: JUCE **8.0.8**, C++20, CMake, `CMAKE_SKIP_RPATH TRUE`.
  Build offline against `/home/brians/bls/sound/plugin_poc/external/JUCE` and
  `/home/brians/bls/sound/plugin_poc/deps/sysroot` when available.
- **Sound generation**: JUCE `juce_dsp` plus purpose-built oscillator/filter
  code. No third-party synth engine is required. Use PolyBLEP/band-limited
  oscillators, TPT/ZDF filters, analog-style envelopes, and a fixed modulation
  matrix. Every nonlinear stage must be stable, denormal-safe, and free of
  per-block allocation.
- **Heritage**: classic 1970s–1980s polyphonic voicing (Juno-, Prophet-, OB-,
  Jupiter-, CS-80-, Polysix-inspired) combined with modern wavetable, motion,
  and effects presets. Use "inspired by" language only; never use another
  manufacturer's logo, trade dress, or claim affiliation.
- **Preset bank**: exactly **48 factory presets**, **8 groups × 6 presets**,
  with **24 Classic and 24 Modern** entries. Group order and every preset ID
  are stable. Follow `references/preset-library.md` exactly unless the owner
  approves a bank revision.
- **Level discipline**: every preset is loudness-matched and must peak at or
  below **−1.0 dBFS** on a full 16-voice chord. No preset may clip the output.
- **Performance**: 16 voices at 48 kHz / 128 samples must stay under roughly
  35% of one modern desktop core with the heaviest preset. No allocations,
  mutexes, file I/O, `juce::String`, or logging in `processBlock()`.

---

## 4. Look & Feel Mandates: Modern Liquid Glass

- **One screen, no settings.** A full-bleed, slowly moving art field behind
  translucent smoked-glass panels. Deep near-black base, restrained chromatic
  bloom, polished edge light, fine grain, and soft caustic refraction.
- **Every preset has a different image.** The hero artwork must change on preset
  selection: palette, motif, bloom geometry, and seed are all derived from the
  preset's `ArtworkSpec`. Use a 300–450 ms crossfade between the outgoing and
  incoming artwork. Never show the same hero image for two presets.
- **Layout**: fixed 1280×800 design canvas, 16:10 aspect, resizable from
  960×600 to 1920×1200 via an `AffineTransform` on a content component.
  Left glass rail (collection filter, eight groups, preset list), main hero
  stage (artwork, preset name, lineage, character, tags), top status strip
  (voice count / MIDI activity), and previous/next controls.
- **No synth controls.** Do not draw knobs, faders, envelope graphs with drag
  handles, or toggle switches. Read-only signal-flow or "recipe" chips are fine.
- **Type**: bundled OFL font family (reuse the Lato TTFs from
  `/home/brians/bls/sound/plugin_poc/Resources/`), tracked uppercase micro-labels,
  large light-weight preset names, tabular numerals. Ensure WCAG-AA contrast
  over artwork with a localized scrim.
- **Motion**: slow art drift (≤ 0.1 Hz), restrained hover transitions, and
  preset crossfade. Repaint the animated stage at 30 fps; do not repaint the
  entire editor at 60 fps. Respect host CPU by pausing animation when the
  editor is not visible.

---

## 5. CI, Versions & Releases

- `.github/workflows/build.yml` must build and test:
  - **Windows x64** on `windows-2022` with static MSVC runtime;
  - **Linux x64** on `ubuntu-latest` with ALSA/X11/FreeType/Fontconfig dev
    packages and Ninja;
  - **macOS Universal** (`arm64;x86_64`, deployment target 11.0) on a supported
    Intel runner, ad-hoc signed.
- Run the DSP/preset tests and the VST3 host smoke test on every platform.
- Package **versioned** `SynthSet-<Platform>-v<version>.zip` files plus
  `.sha256` checksums. Version comes from `project(SynthSet VERSION x.y.z ...)`
  in `CMakeLists.txt`; do not hard-code a second version.
- Pushing a `v*` tag, or dispatching the workflow with a non-empty
  `release_tag`, publishes all three distributions and checksums to a GitHub
  Release. Keep `contents: read` at workflow level and elevate only the release
  job.
- Generate and upload a headless UI preview on Linux as a CI artifact.

---

## 6. Local Verification Checklist

Run these in order before declaring any implementation task complete:

1. `./scripts/build.sh`
2. `./build/SynthSetTests_artefacts/Release/SynthSetTests`
3. `./build/SynthSetHostSmokeTest_artefacts/Release/SynthSetHostSmokeTest dist/SynthSet.vst3`
4. Render at least three contrasting presets, inspect each with `read_image`,
   and confirm the artwork, accent palette, and glass treatment visibly change:
   ```bash
   ./build/SynthSetScreenshot_artefacts/Release/SynthSetScreenshot \
     --preset classic.pad.106-dream --out=dist/synthset-106-dream.png
   ./build/SynthSetScreenshot_artefacts/Release/SynthSetScreenshot \
     --preset modern.pad.aurora-glass --out=dist/synthset-aurora-glass.png
   ./build/SynthSetScreenshot_artefacts/Release/SynthSetScreenshot \
     --preset modern.bass.glass-sub --out=dist/synthset-glass-sub.png
   ```
5. Call `present` on the strongest preview PNG and embed it in the final reply.

Screen-render success is not visual acceptance. Check for text clipping, weak
contrast, identical-looking preset images, accidental controls, and glass
effects that obscure the preset name.

---

## 7. Engineering Discipline

- Follow the `dsh` read-before-edit/write policy on every existing file.
- Keep the audio thread deterministic and allocation-free. Preallocate voices,
  delay lines, wavetables, and scratch buffers in `prepareToPlay()`.
- Prefer plain, trivially copyable patch structs (`float`, `int`, enums) over
  `juce::var`, JSON, or `String` on the audio path.
- Do not introduce a network dependency merely for sound generation. If a
  third-party DSP library is proposed, justify license, binary size, build
  reproducibility, and why JUCE plus custom code is insufficient.
- Add tests with every DSP change. A preset that sounds good but produces NaN,
  denormals, clicks, or inconsistent loudness is not finished.
- Do not commit `external/`, `deps/`, `build*/`, `dist/`, `stage*/`, archives,
  or checksums. Keep generated preview images under `docs/` only when they are
  intentional documentation.
