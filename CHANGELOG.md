# Changelog

All notable changes to SynthSet are documented here. Versions follow the
`project(SynthSet VERSION …)` value in `CMakeLists.txt`.

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
