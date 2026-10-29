# Changelog

All notable changes to SynthSet are documented here. Versions follow the
`project(SynthSet VERSION …)` value in `CMakeLists.txt`.

## 1.0.3 — 2026-10-29

### Fixed

- Windows VST3 host smoke test state-restore race: the test now waits for the
  host's asynchronous program notification instead of a fixed 80 ms pump.
- Host test failures now emit GitHub Actions error annotations with the exact
  failed assertion.

## 1.0.2 — 2026-10-29

### Fixed

- Windows and macOS compile failures caused by narrowing integer-to-float
  conversions in the hero tag/recipe chip layout.
- MSVC object-size headroom and source encoding via `/bigobj` and `/utf-8`.
- Linux CI now runs the VST3 host smoke test headlessly without depending on
  `xvfb-run`/`xauth`; the screenshot job still uses Xvfb.
- CI release tags now correspond to a build that passes on all three
  distribution platforms.

## 1.0.1 — 2026-10-29

### Changed

- Version metadata aligned across CMake, the plugin/standalone binary, the UI
  footer, installer notes, and documentation.
- Distribution refresh with a rebuilt Linux x64 package and checksum, and
  release tags ready for the Windows x64 and macOS Universal CI jobs.

## 1.0.0 — 2026-10-29

### Added

- Initial preset-only SynthSet VST3 and Standalone instrument.
- 16-voice engine with PolyBLEP oscillators, mip-mapped wavetables, sub, noise,
  ring modulation, hard sync, drift, mono/legato/unison modes, and glide.
- Four-pole ZDF ladder and two-pole TPT state-variable filters with drive,
  key tracking, velocity, and post high-pass.
- Two analog-style envelopes, two LFOs, eight fixed modulation routes, and
  host-tempo-synced arpeggiator.
- BBD chorus/ensemble, phaser, stereo filtered ping-pong delay, and plate-style
  diffusion reverb with preallocated buffers.
- 48 factory presets in eight groups, 24 Classic + 24 Modern, with stable IDs,
  unique per-preset artwork specifications, and short-term loudness matching.
- Click-free 15 ms fade preset switching with FX-tail clearing, host program
  integration, MIDI program changes, and ID-only state.
- Liquid-glass 1280×800 UI with collection filter, group rail, preset list,
  crossfading per-preset hero artwork, status strip, read-only recipe panel, and
  previous/next navigation.
- Offline DSP/preset/state/loudness tests, VST3 host smoke test, and deterministic
  headless screenshot harness.
- Windows x64, Linux x64, and macOS Universal CI with versioned ZIPs, SHA-256
  checksums, and GitHub Releases.
- Bundled Lato OFL font family.
