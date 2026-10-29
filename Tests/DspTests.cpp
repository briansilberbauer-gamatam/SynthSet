#include "PluginProcessor.h"
#include "dsp/Envelope.h"
#include "dsp/Filters.h"
#include "dsp/Lfo.h"
#include "dsp/Oscillator.h"
#include "dsp/SynthEngine.h"
#include "preset/PresetBank.h"

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace
{
int failures = 0;

void check (bool condition, const std::string& message)
{
    std::cout << (condition ? "  PASS  " : "  FAIL  ") << message << '\n';

    if (! condition)
        ++failures;
}

bool finiteBuffer (const juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            if (! std::isfinite (buffer.getReadPointer (ch)[i]))
                return false;

    return true;
}

float peakOf (const juce::AudioBuffer<float>& buffer)
{
    float peak = 0.0f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        peak = std::max (peak, buffer.getMagnitude (ch, 0, buffer.getNumSamples()));

    return peak;
}

double db (double linear)
{
    return 20.0 * std::log10 (std::max (1.0e-12, linear));
}

bool selectProgram (synthset::SynthEngine& engine, int index, double sampleRate)
{
    juce::AudioBuffer<float> silence (2, 512);
    juce::MidiBuffer midi;

    for (int attempt = 0; attempt < 80; ++attempt)
    {
        if (engine.getAppliedProgram() == index)
            return true;

        silence.clear();
        engine.process (silence, midi, index, 120.0);
    }

    juce::ignoreUnused (sampleRate);
    return engine.getAppliedProgram() == index;
}

struct RenderResult
{
    bool finite = true;
    float peak = 0.0f;
    double rms = 0.0;
    double maxShortTermRms = 0.0;
    int samples = 0;
};

RenderResult renderChord (synthset::SynthEngine& engine, int programIndex,
                          double sampleRate, const std::vector<int>& notes,
                          double holdSeconds, double releaseSeconds,
                          bool measureTail = false)
{
    RenderResult result;
    juce::AudioBuffer<float> buffer (2, 512);
    const int holdBlocks = std::max (1, static_cast<int> (std::ceil (holdSeconds * sampleRate / 512.0)));
    const int releaseBlocks = std::max (1, static_cast<int> (std::ceil (releaseSeconds * sampleRate / 512.0)));
    double sumSquares = 0.0;
    double windowSquares = 0.0;
    int windowSamples = 0;
    constexpr int shortTermWindow = 2048;

    for (int block = 0; block < holdBlocks + releaseBlocks; ++block)
    {
        buffer.clear();
        juce::MidiBuffer midi;

        if (block == 0)
        {
            int offset = 0;

            for (const int note : notes)
            {
                midi.addEvent (juce::MidiMessage::noteOn (1, note, 0.68f), offset);
                offset = (offset + 9) % 120;
            }
        }

        if (block == holdBlocks)
        {
            for (const int note : notes)
                midi.addEvent (juce::MidiMessage::noteOff (1, note), 0);
        }

        engine.process (buffer, midi, programIndex, 120.0);
        result.finite = result.finite && finiteBuffer (buffer);
        result.peak = std::max (result.peak, peakOf (buffer));

        const bool include = measureTail ? block >= holdBlocks / 2
                                         : block < holdBlocks;
        if (include)
        {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                const auto* data = buffer.getReadPointer (ch);
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                {
                    const double square = static_cast<double> (data[i]) * data[i];
                    sumSquares += square;
                    windowSquares += square;
                    ++windowSamples;

                    if (windowSamples >= shortTermWindow * buffer.getNumChannels())
                    {
                        const double windowRms = std::sqrt (windowSquares / windowSamples);
                        result.maxShortTermRms = std::max (result.maxShortTermRms, windowRms);
                        windowSquares = 0.0;
                        windowSamples = 0;
                    }
                }
            }

            result.samples += buffer.getNumSamples() * buffer.getNumChannels();
        }
    }

    result.rms = result.samples > 0 ? std::sqrt (sumSquares / result.samples) : 0.0;
    return result;
}

void testPresetBank()
{
    std::cout << "\n[Preset bank]\n";
    const auto& bank = synthset::getFactoryBank();
    check (bank.size() == 48, "exactly 48 factory presets");
    check (static_cast<int> (bank.size()) == synthset::getNumFactoryPresets(),
           "bank size and accessor agree");

    int classic = 0;
    int modern = 0;
    std::set<std::string> ids;
    std::set<std::string> names;
    std::set<float> seeds;

    for (const auto& preset : bank)
    {
        check (preset.id != nullptr && *preset.id != '\0', "preset has a stable ID");
        check (preset.name != nullptr && *preset.name != '\0', "preset has a display name");
        check (preset.description != nullptr && *preset.description != '\0',
               "preset has a description");
        check (std::strlen (preset.description) <= 110, "description is at most 110 characters");
        check (ids.insert (preset.id).second, "preset ID is unique");
        check (names.insert (preset.name).second, "preset name is unique");
        check (seeds.insert (preset.artwork.seed).second, "artwork seed is unique");

        int tagCount = 0;

        for (const auto* tag : preset.tags)
            if (tag != nullptr && *tag != '\0')
                ++tagCount;

        check (tagCount >= 2 && tagCount <= 4, "preset has two to four tags");

        if (preset.collection == synthset::Collection::Classic)
            ++classic;
        else
            ++modern;
    }

    check (classic == 24, "24 Classic presets");
    check (modern == 24, "24 Modern presets");

    for (const auto group : synthset::allGroups)
    {
        int total = 0;
        int groupClassic = 0;
        int groupModern = 0;

        for (const auto& preset : bank)
        {
            if (preset.group != group)
                continue;

            ++total;

            if (preset.collection == synthset::Collection::Classic)
                ++groupClassic;
            else
                ++groupModern;
        }

        check (total == 6, std::string ("six presets in ") + synthset::groupDisplayName (group));
        check (groupClassic == 3, std::string ("three Classic in ") + synthset::groupDisplayName (group));
        check (groupModern == 3, std::string ("three Modern in ") + synthset::groupDisplayName (group));
    }

    const int defaultIndex = synthset::getDefaultPresetIndex();
    check (defaultIndex >= 0, "default preset resolves");
    check (defaultIndex >= 0
               && std::strcmp (bank[static_cast<size_t> (defaultIndex)].id,
                               synthset::defaultPresetId) == 0,
           "default preset is classic.keys.five-voice-keys");
    check (synthset::findPresetById ("does.not.exist") == nullptr,
           "unknown preset ID does not resolve");
    check (synthset::findPresetById ("classic.pad.106-dream") != nullptr,
           "known preset ID resolves");
}

void testProcessorContract()
{
    std::cout << "\n[Processor contract and state]\n";
    SynthSetAudioProcessor processor;
    check (processor.getParameters().size() == 0, "getParameters() is empty");
    check (processor.getNumPrograms() == 48, "getNumPrograms() is 48");
    check (processor.getLatencySamples() == 0, "reported latency remains zero");
    check (processor.acceptsMidi(), "MIDI input is enabled");
    check (! processor.producesMidi(), "MIDI output is disabled");

    processor.setCurrentProgram (7);
    juce::MemoryBlock state;
    processor.getStateInformation (state);

    auto xml = juce::XmlDocument::parse (juce::String::fromUTF8 (
        static_cast<const char*> (state.getData()), static_cast<int> (state.getSize())));
    check (xml != nullptr && xml->hasTagName ("SYNTHSET"), "state XML has the SYNTHSET root");
    check (xml != nullptr && xml->getIntAttribute ("schema") == 1, "state schema is version 1");
    check (xml != nullptr && xml->getStringAttribute ("presetId")
                                 == synthset::getFactoryBank()[7].id,
           "state stores the selected preset ID");

    SynthSetAudioProcessor restored;
    restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    check (restored.getPresetManager().getPendingProgram() == 7,
           "state restores by preset ID");

    const char* bogus = "<SYNTHSET schema=\"1\" presetId=\"classic.nope.missing\"/>";
    restored.setStateInformation (bogus, static_cast<int> (std::strlen (bogus)));
    check (restored.getPresetManager().getPendingProgram() == synthset::getDefaultPresetIndex(),
           "unknown preset ID falls back to the default");

    restored.setPlayConfigDetails (2, 2, 48000.0, 512);
    restored.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> buffer (2, 512);

    for (int i = 0; i < 20; ++i)
    {
        buffer.clear();
        juce::MidiBuffer midi;
        restored.processBlock (buffer, midi);
    }

    check (restored.getCurrentProgram() == synthset::getDefaultPresetIndex(),
           "restored pending default is applied through the audio engine");

    restored.releaseResources();

    SynthSetAudioProcessor midiProgramProcessor;
    midiProgramProcessor.setPlayConfigDetails (2, 2, 48000.0, 512);
    midiProgramProcessor.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> midiBuffer (2, 512);
    juce::MidiBuffer programMidi;
    programMidi.addEvent (juce::MidiMessage::programChange (1, 21), 0);

    for (int i = 0; i < 30; ++i)
    {
        midiBuffer.clear();
        juce::MidiBuffer midi = i == 0 ? programMidi : juce::MidiBuffer {};
        midiProgramProcessor.processBlock (midiBuffer, midi);
    }

    check (midiProgramProcessor.getCurrentProgram() == 21,
           "MIDI program change selects a factory preset");
    midiProgramProcessor.releaseResources();
}

void testEnvelopes()
{
    std::cout << "\n[Envelope timing]\n";
    synthset::Adsr env;
    env.prepare (48000.0);
    synthset::AdsrParams params;
    params.attackMs = 50.0f;
    params.decayMs = 100.0f;
    params.sustain = 0.5f;
    params.releaseMs = 120.0f;
    params.curve = 0.5f;
    env.setParams (params);
    env.noteOn();

    int attackSamples = 0;

    while (env.getStage() == synthset::Adsr::Stage::Attack && attackSamples < 10000)
    {
        env.process();
        ++attackSamples;
    }

    const int expectedAttack = static_cast<int> (50.0 * 48.0);
    check (std::abs (attackSamples - expectedAttack) <= expectedAttack / 10,
           "50 ms attack timing is within 10 percent");

    for (int i = 0; i < 6000; ++i)
        env.process();

    env.noteOff();
    int releaseSamples = 0;

    while (env.getStage() != synthset::Adsr::Stage::Idle && releaseSamples < 20000)
    {
        env.process();
        ++releaseSamples;
    }

    const int expectedRelease = static_cast<int> (120.0 * 48.0);
    check (std::abs (releaseSamples - expectedRelease) <= expectedRelease / 10,
           "120 ms release timing is within 10 percent");
    check (env.getLevel() <= 1.0e-9f, "release reaches exactly zero");
}

void testFilters()
{
    std::cout << "\n[Filter stability]\n";
    bool allFinite = true;
    float maximum = 0.0f;

    for (const double sampleRate : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        for (const auto model : { synthset::FilterModel::Ladder4P, synthset::FilterModel::Svf2P })
        {
            for (const auto type : { synthset::FilterType::LowPass, synthset::FilterType::HighPass,
                                     synthset::FilterType::BandPass })
            {
                for (const float cutoff : { 20.0f, 90.0f, 1000.0f, 12000.0f, 20000.0f })
                {
                    for (const float resonance : { 0.0f, 0.5f, 0.95f, 1.0f })
                    {
                        synthset::FilterParams params;
                        params.model = model;
                        params.type = type;
                        params.cutoffHz = cutoff;
                        params.resonance = resonance;
                        params.drive = 1.0f;
                        params.highPassHz = 20.0f;
                        synthset::FilterState state;
                        const auto coeffs = synthset::makeFilterCoeffs (params, cutoff, resonance, sampleRate);

                        for (int i = 0; i < 4096; ++i)
                        {
                            const float input = (i == 0 ? 1.0f : 0.0f)
                                + 0.2f * std::sin (juce::MathConstants<float>::twoPi * 440.0f
                                                   * static_cast<float> (i) / static_cast<float> (sampleRate));
                            const float output = state.process (input, coeffs, model, type);
                            allFinite = allFinite && std::isfinite (output);
                            maximum = std::max (maximum, std::abs (output));
                        }
                    }
                }
            }
        }
    }

    check (allFinite, "all filter models remain finite across sample rates and extremes");
    check (maximum < 20.0f, "filter output remains bounded at maximum resonance");
}

void testLfos()
{
    std::cout << "\n[LFO shapes and sync]\n";
    synthset::Lfo lfo;
    lfo.prepare (48000.0);

    for (const auto shape : { synthset::LfoShape::Sine, synthset::LfoShape::Triangle,
                              synthset::LfoShape::Saw, synthset::LfoShape::Square,
                              synthset::LfoShape::SampleHold })
    {
        synthset::LfoParams params;
        params.shape = shape;
        params.rateHz = 2.0f;
        params.fadeInMs = 0.0f;
        lfo.setParams (params);
        lfo.reset (0x1234u);
        float low = 1.0f;
        float high = -1.0f;

        for (int i = 0; i < 48000; ++i)
        {
            const float value = lfo.advance (16, 2.0f);
            low = std::min (low, value);
            high = std::max (high, value);
        }

        check (std::isfinite (low) && std::isfinite (high), "LFO produces finite values");
        check (high - low > 0.2f, "LFO shape is moving");
        check (low >= -1.01f && high <= 1.01f, "LFO remains in the normalised range");
    }

    synthset::LfoParams synced;
    synced.tempoSync = true;
    synced.syncBeats = 1.0f;
    check (std::abs (synthset::lfoRateHz (synced, 120.0) - 2.0f) < 0.001f,
           "one-beat sync at 120 BPM equals 2 Hz");
    check (std::abs (synthset::lfoRateHz (synced, 90.0) - 1.5f) < 0.001f,
           "one-beat sync at 90 BPM equals 1.5 Hz");
}

void testOscillatorAliasing()
{
    std::cout << "\n[Oscillator anti-aliasing]\n";
    constexpr int fftOrder = 15;
    constexpr int fftSize = 1 << fftOrder;
    constexpr double sampleRate = 48000.0;
    constexpr double frequency = 1046.5;
    const double increment = frequency / sampleRate;

    auto aliasEnergy = [&] (bool polyBlep)
    {
        std::vector<float> data (static_cast<size_t> (fftSize) * 2, 0.0f);
        double phase = 0.0;

        for (int i = 0; i < fftSize; ++i)
        {
            phase += increment;
            if (phase >= 1.0)
                phase -= 1.0;

            const float value = polyBlep
                ? synthset::sawWave (phase, increment)
                : static_cast<float> (2.0 * phase - 1.0);
            const double window = 0.5 - 0.5 * std::cos (2.0 * juce::MathConstants<double>::pi
                                                        * i / (fftSize - 1));
            data[static_cast<size_t> (i)] = static_cast<float> (value * window);
        }

        juce::dsp::FFT fft (fftOrder);
        fft.performFrequencyOnlyForwardTransform (data.data(), true);

        double energy = 0.0;

        for (int bin = 1; bin < fftSize / 2; ++bin)
        {
            const double binHz = bin * sampleRate / fftSize;

            if (binHz < 5000.0 || binHz > 20000.0)
                continue;

            const double harmonic = std::round (binHz / frequency);

            if (std::abs (binHz - harmonic * frequency) < 32.0)
                continue;

            const double magnitude = data[static_cast<size_t> (bin)];
            energy += magnitude * magnitude;
        }

        return energy;
    };

    const double naive = aliasEnergy (false);
    const double blep = aliasEnergy (true);
    const double improvementDb = 10.0 * std::log10 (std::max (1.0e-20, naive)
                                                    / std::max (1.0e-20, blep));
    std::cout << "  INFO  alias energy improvement: " << std::fixed << std::setprecision (1)
              << improvementDb << " dB\n";
    check (improvementDb >= 12.0, "PolyBLEP improves alias energy by at least 12 dB");
}

void testAudioAcrossSampleRates()
{
    std::cout << "\n[All presets, finite audio, all sample rates]\n";
    const std::vector<int> notes { 48, 52, 55, 60, 64, 67, 72, 76 };
    const double peakLimit = 0.891251;

    for (const double sampleRate : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        synthset::SynthEngine engine;
        engine.prepare (sampleRate, 512);
        bool finite = true;
        float peak = 0.0f;
        bool silentPreset = false;

        for (int index = 0; index < synthset::kNumPresets; ++index)
        {
            check (selectProgram (engine, index, sampleRate),
                   std::string ("preset applies at ") + std::to_string (static_cast<int> (sampleRate))
                       + " Hz: " + synthset::getFactoryBank()[static_cast<size_t> (index)].id);

            const auto result = renderChord (engine, index, sampleRate, notes, 0.22, 0.08);
            finite = finite && result.finite;
            peak = std::max (peak, result.peak);

            if (result.peak < 0.00001f)
                silentPreset = true;
        }

        check (finite, std::string ("all presets render finite audio at ")
                          + std::to_string (static_cast<int> (sampleRate)) + " Hz");
        check (peak <= peakLimit, std::string ("all presets peak at or below -1 dBFS at ")
                                      + std::to_string (static_cast<int> (sampleRate)) + " Hz");
        check (! silentPreset, std::string ("no preset is silent at ")
                                    + std::to_string (static_cast<int> (sampleRate)) + " Hz");
    }
}

void testSixteenVoicePeak()
{
    std::cout << "\n[Sixteen-voice peak safety]\n";
    const std::vector<int> notes { 36, 38, 40, 43, 45, 48, 50, 52, 55, 57, 60, 62, 64, 67, 69, 72 };
    synthset::SynthEngine engine;
    engine.prepare (48000.0, 512);
    float peak = 0.0f;

    for (const int index : { 12, 36, 45 })
    {
        check (selectProgram (engine, index, 48000.0), "16-voice test preset applies");
        const auto result = renderChord (engine, index, 48000.0, notes, 0.35, 0.10);
        peak = std::max (peak, result.peak);
    }

    check (peak <= 0.891251f, "16-voice chords stay below -1 dBFS");
}

void testPresetSwitchClick()
{
    std::cout << "\n[Click-free preset switching]\n";
    synthset::SynthEngine engine;
    engine.prepare (48000.0, 512);
    check (selectProgram (engine, 6, 48000.0), "start preset applies");

    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer chord;
    chord.addEvent (juce::MidiMessage::noteOn (1, 48, 0.7f), 0);
    chord.addEvent (juce::MidiMessage::noteOn (1, 60, 0.7f), 4);
    chord.addEvent (juce::MidiMessage::noteOn (1, 67, 0.7f), 8);
    float previousLeft = 0.0f;
    float maximumDelta = 0.0f;
    bool finite = true;

    auto processBlock = [&] (int pending, juce::MidiBuffer& midi)
    {
        buffer.clear();
        engine.process (buffer, midi, pending, 120.0);
        finite = finite && finiteBuffer (buffer);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float current = buffer.getSample (0, i);
            maximumDelta = std::max (maximumDelta, std::abs (current - previousLeft));
            previousLeft = current;
        }
    };

    for (int i = 0; i < 20; ++i)
    {
        juce::MidiBuffer midi = i == 0 ? chord : juce::MidiBuffer {};
        processBlock (6, midi);
    }

    juce::MidiBuffer none;

    for (int i = 0; i < 40; ++i)
        processBlock (7, none);

    check (engine.getAppliedProgram() == 7, "second preset applies after the switch");
    check (finite, "switch output remains finite");
    check (maximumDelta < 0.32f, "preset switch has no click above the safety threshold");
}

void testMidiAndVoiceLifecycle()
{
    std::cout << "\n[Voices, sustain and arp]\n";
    synthset::SynthEngine engine;
    engine.prepare (48000.0, 512);
    check (selectProgram (engine, 12, 48000.0), "poly test preset applies");
    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;

    for (int note = 40; note < 72; ++note)
        midi.addEvent (juce::MidiMessage::noteOn (1, note, 0.6f), note % 64);

    buffer.clear();
    engine.process (buffer, midi, 12, 120.0);
    check (engine.getVoiceCount() <= synthset::kNumVoices, "voice count is capped at 16");
    check (engine.getVoiceCount() > 0, "voice allocation produces active voices");
    check (finiteBuffer (buffer), "voice stress output is finite");

    juce::MidiBuffer allOff;
    allOff.addEvent (juce::MidiMessage::controllerEvent (1, 123, 0), 0);
    buffer.clear();
    engine.process (buffer, allOff, 12, 120.0);

    for (int i = 0; i < 240; ++i)
    {
        buffer.clear();
        juce::MidiBuffer blank;
        engine.process (buffer, blank, 12, 120.0);
    }

    check (engine.getVoiceCount() == 0, "all-notes-off recycles every voice");

    check (selectProgram (engine, 12, 48000.0), "sustain test preset applies");
    juce::MidiBuffer noteOn;
    noteOn.addEvent (juce::MidiMessage::noteOn (1, 60, 0.7f), 0);
    buffer.clear();
    engine.process (buffer, noteOn, 12, 120.0);

    juce::MidiBuffer sustainDown;
    sustainDown.addEvent (juce::MidiMessage::controllerEvent (1, 64, 127), 0);
    sustainDown.addEvent (juce::MidiMessage::noteOff (1, 60), 8);
    buffer.clear();
    engine.process (buffer, sustainDown, 12, 120.0);

    for (int i = 0; i < 30; ++i)
    {
        buffer.clear();
        juce::MidiBuffer blank;
        engine.process (buffer, blank, 12, 120.0);
    }

    check (engine.getVoiceCount() > 0, "sustain pedal defers note release");

    juce::MidiBuffer sustainUp;
    sustainUp.addEvent (juce::MidiMessage::controllerEvent (1, 64, 0), 0);
    buffer.clear();
    engine.process (buffer, sustainUp, 12, 120.0);

    for (int i = 0; i < 240; ++i)
    {
        buffer.clear();
        juce::MidiBuffer blank;
        engine.process (buffer, blank, 12, 120.0);
    }

    check (engine.getVoiceCount() == 0, "sustain-up releases held voices");

    check (selectProgram (engine, 45, 48000.0), "arp test preset applies");
    juce::MidiBuffer arpNote;
    arpNote.addEvent (juce::MidiMessage::noteOn (1, 57, 0.8f), 0);
    arpNote.addEvent (juce::MidiMessage::noteOn (1, 64, 0.8f), 6);
    buffer.clear();
    engine.process (buffer, arpNote, 45, 120.0);
    bool arpFinite = finiteBuffer (buffer);

    for (int i = 0; i < 60; ++i)
    {
        buffer.clear();
        juce::MidiBuffer blank;
        engine.process (buffer, blank, 45, 120.0);
        arpFinite = arpFinite && finiteBuffer (buffer);
    }

    check (arpFinite, "arpeggiator output is finite");
    check (engine.getVoiceCount() > 0, "arpeggiator triggers voices");

    juce::MidiBuffer panic;
    panic.addEvent (juce::MidiMessage::controllerEvent (1, 120, 0), 0);

    for (int i = 0; i < 80; ++i)
    {
        buffer.clear();
        juce::MidiBuffer blank = i == 0 ? panic : juce::MidiBuffer {};
        engine.process (buffer, blank, 45, 120.0);
    }

    check (engine.getVoiceCount() == 0, "all-sound-off stops the arpeggiator");
}

std::vector<int> phraseForPreset (const synthset::Preset& preset)
{
    if (preset.patch.voiceMode == synthset::VoiceMode::Mono
        || preset.patch.voiceMode == synthset::VoiceMode::Legato)
        return { preset.group == synthset::Group::Bass ? 36 : 55 };

    if (preset.patch.arp.enabled)
        return { 48, 55, 60, 64 };

    return { 48, 52, 55, 60, 64, 67, 72, 76 };
}

void measureBank()
{
    synthset::SynthEngine engine;
    engine.prepare (48000.0, 512);

    for (int index = 0; index < synthset::kNumPresets; ++index)
    {
        const auto& preset = synthset::getFactoryBank()[static_cast<size_t> (index)];
        const auto notes = phraseForPreset (preset);

        selectProgram (engine, index, 48000.0);
        const auto result = renderChord (engine, index, 48000.0, notes, 2.5, 0.25);
        std::cout << "MEASURE\t" << preset.id << "\t" << std::fixed << std::setprecision (3)
                  << db (result.peak) << "\t" << db (result.rms) << "\t"
                  << db (result.maxShortTermRms) << '\n';
    }
}

void testLoudnessMatching()
{
    std::cout << "\n[Factory bank loudness matching]\n";
    synthset::SynthEngine engine;
    engine.prepare (48000.0, 512);
    double lowest = 100.0;
    double highest = -100.0;
    float peak = 0.0f;
    bool finite = true;
    double sum = 0.0;

    for (int index = 0; index < synthset::kNumPresets; ++index)
    {
        const auto& preset = synthset::getFactoryBank()[static_cast<size_t> (index)];
        selectProgram (engine, index, 48000.0);
        const auto result = renderChord (engine, index, 48000.0, phraseForPreset (preset), 2.5, 0.25);
        const double loudness = db (result.maxShortTermRms);
        lowest = std::min (lowest, loudness);
        highest = std::max (highest, loudness);
        peak = std::max (peak, result.peak);
        finite = finite && result.finite;
        sum += loudness;
        std::cout << "  INFO  " << std::fixed << std::setprecision (2) << std::setw (6)
                  << loudness << " dB short-term  " << preset.id << '\n';
    }

    std::cout << std::defaultfloat;
    const double mean = sum / synthset::kNumPresets;
    check (finite, "loudness sweep renders finite audio for all presets");
    check (peak <= 0.891251f, "the calibrated bank stays below -1 dBFS");
    check (highest - lowest <= 1.5, "short-term loudness stays within a 1.5 dB window");
    check (std::abs (mean + 20.0) <= 1.5, "bank loudness sits near the -20 dBFS target");
}

void testMonoAndStereoBuses()
{
    std::cout << "\n[Mono and stereo host buses]\n";
    SynthSetAudioProcessor processor;
    processor.setPlayConfigDetails (1, 1, 48000.0, 512);
    processor.prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> mono (1, 512);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.7f), 0);
    mono.clear();
    processor.processBlock (mono, midi);
    check (finiteBuffer (mono), "mono host bus produces finite output");
    processor.releaseResources();

    processor.setPlayConfigDetails (2, 2, 48000.0, 512);
    processor.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> stereo (2, 512);
    juce::MidiBuffer midiStereo;
    midiStereo.addEvent (juce::MidiMessage::noteOn (1, 64, 0.7f), 0);
    stereo.clear();
    processor.processBlock (stereo, midiStereo);
    check (finiteBuffer (stereo), "stereo host bus produces finite output");
    processor.releaseResources();
}
} // namespace

void benchmarkEngine()
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 128;
    constexpr double seconds = 12.0;
    synthset::SynthEngine engine;
    engine.prepare (sampleRate, blockSize);
    selectProgram (engine, 12, sampleRate);

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer firstMidi;

    for (int i = 0; i < 16; ++i)
        firstMidi.addEvent (juce::MidiMessage::noteOn (1, 36 + i * 3, 0.7f), i * 2);

    const auto start = juce::Time::getHighResolutionTicks();
    const int blocks = static_cast<int> (seconds * sampleRate / blockSize);

    for (int block = 0; block < blocks; ++block)
    {
        buffer.clear();
        juce::MidiBuffer midi = block == 0 ? firstMidi : juce::MidiBuffer {};
        engine.process (buffer, midi, 12, 120.0);
    }

    const auto elapsed = juce::Time::highResolutionTicksToSeconds (
        juce::Time::getHighResolutionTicks() - start);
    const double cpuPercent = elapsed / seconds * 100.0;
    std::cout << "BENCH 16 voices / 48 kHz / 128 samples: " << std::fixed
              << std::setprecision (1) << cpuPercent << "% of one core ("
              << elapsed << " s for " << seconds << " s audio)\n";
}

int main (int argc, char** argv)
{
    juce::ScopedNoDenormals noDenormals;

    if (argc > 1 && std::string (argv[1]) == "--benchmark")
    {
        benchmarkEngine();
        return 0;
    }

    if (argc > 1 && std::string (argv[1]) == "--measure")
    {
        measureBank();
        return 0;
    }

    std::cout << "SynthSet tests\n==============\n";
    testPresetBank();
    testProcessorContract();
    testEnvelopes();
    testFilters();
    testLfos();
    testOscillatorAliasing();
    testAudioAcrossSampleRates();
    testSixteenVoicePeak();
    testPresetSwitchClick();
    testMidiAndVoiceLifecycle();
    testLoudnessMatching();
    testMonoAndStereoBuses();

    std::cout << "\n" << (failures == 0 ? "All tests passed." : "Tests failed.") << '\n';
    return failures == 0 ? 0 : 1;
}
