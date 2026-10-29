#pragma once

#include "../preset/PresetBank.h"
#include "Effects.h"
#include "Lfo.h"
#include "Voice.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <atomic>
#include <cstdint>

namespace synthset
{
class SynthEngine
{
public:
    SynthEngine();

    void prepare (double sampleRate, int maximumBlockSize);
    void reset();

    void process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi,
                  int pendingProgram, double bpm);

    int getAppliedProgram() const noexcept { return appliedProgram.load (std::memory_order_acquire); }
    int getVoiceCount() const noexcept { return voiceCountMeter.load (std::memory_order_relaxed); }
    float getPeakLeft() const noexcept { return peakLeft.load (std::memory_order_relaxed); }
    float getPeakRight() const noexcept { return peakRight.load (std::memory_order_relaxed); }
    std::uint32_t getMidiCounter() const noexcept { return midiCounter.load (std::memory_order_relaxed); }

private:
    enum class FadeState { Idle, Out, In };

    void handleMidiMessage (const juce::MidiMessage& message);
    void handleNoteOn (int note, float velocity);
    void handleNoteOff (int note);
    void handleProgramChange (int program);
    void allNotesOff (bool immediate);
    void updateSustain (bool down);

    void updateArp (int numSamples, double bpm);
    void triggerArpStep (double bpm);
    void clearArp();

    void startVoice (int voiceIndex, int noteNumber, float velocity, int layerCount,
                     float glideFrom);
    int chooseVoiceToSteal() const noexcept;
    int chooseActiveVoiceToSteal() const noexcept;
    int countActiveVoices() const noexcept;

    void renderSegment (juce::AudioBuffer<float>& destination, int destinationOffset,
                        int numSamples, double bpm);
    void switchToProgram (int index);
    void resetVoicesAndEffects();
    void configureActivePatch();

    const SynthPatch& activePatch() const noexcept;

    std::array<Voice, kNumVoices> voices {};
    std::array<int, 16> monoHeld {};
    int monoHeldCount = 0;

    Lfo globalLfo1, globalLfo2;
    Chorus chorus;
    Phaser phaser;
    Delay delay;
    PlateReverb reverb;
    OutputStage outputStage;

    juce::AudioBuffer<float> scratch;

    double sampleRate = 48000.0;
    int maximumBlock = 512;
    int activeIndex = 0;
    int targetIndex = 0;
    int midiProgramRequest = -1;
    std::uint64_t startOrderCounter = 0;

    FadeState fadeState = FadeState::Idle;
    float fadeGain = 1.0f;
    float fadeIncrement = 0.001f;

    std::array<bool, 128> sustainedNotes {};
    bool sustainDown = false;
    float pitchBend = 0.0f;
    float modWheel = 0.0f;
    float aftertouch = 0.0f;

    // Arpeggiator state
    std::array<int, 16> arpHeldNotes {};
    std::array<float, 16> arpHeldVelocities {};
    int arpHeldCount = 0;
    int arpStepIndex = 0;
    int arpDirection = 1;
    int arpCurrentNote = -1;
    double arpSamplesToStep = 0.0;
    double arpGateCountdown = 0.0;
    std::uint32_t arpRandomState = 0x53594e54u;

    std::atomic<int> appliedProgram { 0 };
    std::atomic<int> voiceCountMeter { 0 };
    std::atomic<float> peakLeft { 0.0f };
    std::atomic<float> peakRight { 0.0f };
    std::atomic<std::uint32_t> midiCounter { 0 };
};
} // namespace synthset
