#include "SynthEngine.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace synthset
{
using juce::jlimit;

SynthEngine::SynthEngine()
{
    activeIndex = getDefaultPresetIndex();
    targetIndex = activeIndex;
    appliedProgram.store (activeIndex, std::memory_order_relaxed);
}

void SynthEngine::prepare (double newSampleRate, int maximumBlockSize)
{
    sampleRate = std::max (8000.0, newSampleRate);
    maximumBlock = std::max (32, maximumBlockSize);
    scratch.setSize (2, std::max (maximumBlock, 8192), false, false, true);

    for (auto& voice : voices)
        voice.prepare (sampleRate);

    globalLfo1.prepare (sampleRate);
    globalLfo2.prepare (sampleRate);
    (void) getSharedWavetableBank();
    chorus.prepare (sampleRate);
    phaser.prepare (sampleRate);
    delay.prepare (sampleRate);
    reverb.prepare (sampleRate);
    outputStage.prepare (sampleRate);

    fadeIncrement = static_cast<float> (1.0 / (0.015 * sampleRate));
    fadeGain = 1.0f;
    fadeState = FadeState::Idle;
    resetVoicesAndEffects();
    configureActivePatch();

    peakLeft.store (0.0f, std::memory_order_relaxed);
    peakRight.store (0.0f, std::memory_order_relaxed);
    voiceCountMeter.store (0, std::memory_order_relaxed);
}

void SynthEngine::reset()
{
    resetVoicesAndEffects();
    configureActivePatch();
    fadeState = FadeState::Idle;
    fadeGain = 1.0f;
    pitchBend = modWheel = aftertouch = 0.0f;
    appliedProgram.store (activeIndex, std::memory_order_relaxed);
    peakLeft.store (0.0f, std::memory_order_relaxed);
    peakRight.store (0.0f, std::memory_order_relaxed);
}

const SynthPatch& SynthEngine::activePatch() const noexcept
{
    return getFactoryBank()[static_cast<size_t> (activeIndex)].patch;
}

void SynthEngine::configureActivePatch()
{
    const auto& patch = activePatch();
    globalLfo1.setParams (patch.lfo1);
    globalLfo2.setParams (patch.lfo2);
    globalLfo1.reset (patch.seed ^ 0xabcdef01u, 0.0f);
    globalLfo2.reset (patch.seed ^ 0x10203040u, 0.0f);
    outputStage.setTrimDb (patch.outputTrimDb);
    fadeIncrement = static_cast<float> (1.0 / (0.015 * sampleRate));
}

void SynthEngine::resetVoicesAndEffects()
{
    for (auto& voice : voices)
        voice.forceStop();

    chorus.reset();
    phaser.reset();
    delay.reset();
    reverb.reset();
    outputStage.reset();
    outputStage.setTrimDb (activePatch().outputTrimDb);
    clearArp();
    monoHeldCount = 0;
    sustainedNotes.fill (false);
    sustainDown = false;
    arpRandomState = 0x53594e54u;
}

void SynthEngine::clearArp()
{
    arpHeldCount = 0;
    arpStepIndex = 0;
    arpDirection = 1;
    arpCurrentNote = -1;
    arpSamplesToStep = 0.0;
    arpGateCountdown = 0.0;
}

int SynthEngine::countActiveVoices() const noexcept
{
    int count = 0;

    for (const auto& voice : voices)
        if (voice.isActive())
            ++count;

    return count;
}

int SynthEngine::chooseActiveVoiceToSteal() const noexcept
{
    int oldestRelease = -1;
    int quietest = 0;
    int oldest = 0;
    float quietestLevel = std::numeric_limits<float>::max();
    std::uint64_t oldestReleaseOrder = std::numeric_limits<std::uint64_t>::max();
    std::uint64_t oldestOrder = std::numeric_limits<std::uint64_t>::max();

    for (int i = 0; i < kNumVoices; ++i)
    {
        const auto& voice = voices[static_cast<size_t> (i)];

        if (! voice.isActive())
            continue;

        if (voice.isReleasing() && voice.getStartOrder() < oldestReleaseOrder)
        {
            oldestReleaseOrder = voice.getStartOrder();
            oldestRelease = i;
        }

        if (voice.getAmpLevel() < quietestLevel)
        {
            quietestLevel = voice.getAmpLevel();
            quietest = i;
        }

        if (voice.getStartOrder() < oldestOrder)
        {
            oldestOrder = voice.getStartOrder();
            oldest = i;
        }
    }

    if (oldestRelease >= 0)
        return oldestRelease;

    return quietestLevel < 0.02f ? quietest : oldest;
}

int SynthEngine::chooseVoiceToSteal() const noexcept
{
    int inactive = -1;
    int oldestRelease = -1;
    int quietest = 0;
    int oldest = 0;
    float quietestLevel = std::numeric_limits<float>::max();
    std::uint64_t oldestReleaseOrder = std::numeric_limits<std::uint64_t>::max();
    std::uint64_t oldestOrder = std::numeric_limits<std::uint64_t>::max();

    for (int i = 0; i < kNumVoices; ++i)
    {
        const auto& voice = voices[static_cast<size_t> (i)];

        if (! voice.isActive())
        {
            if (inactive < 0)
                inactive = i;
            continue;
        }

        if (voice.isReleasing() && voice.getStartOrder() < oldestReleaseOrder)
        {
            oldestReleaseOrder = voice.getStartOrder();
            oldestRelease = i;
        }

        if (voice.getAmpLevel() < quietestLevel)
        {
            quietestLevel = voice.getAmpLevel();
            quietest = i;
        }

        if (voice.getStartOrder() < oldestOrder)
        {
            oldestOrder = voice.getStartOrder();
            oldest = i;
        }
    }

    if (inactive >= 0)
        return inactive;

    if (oldestRelease >= 0)
        return oldestRelease;

    return quietestLevel < 0.020f ? quietest : oldest;
}

void SynthEngine::startVoice (int voiceIndex, int noteNumber, float velocity,
                              int layerCount, float glideFrom)
{
    if (voiceIndex < 0 || voiceIndex >= kNumVoices)
        return;

    ++startOrderCounter;
    voices[static_cast<size_t> (voiceIndex)].start (activePatch(), noteNumber, velocity,
                                                    startOrderCounter, layerCount,
                                                    glideFrom, pitchBend, modWheel,
                                                    aftertouch);
}

void SynthEngine::handleNoteOn (int note, float velocity)
{
    if (note < 0 || note > 127)
        return;

    midiCounter.fetch_add (1, std::memory_order_relaxed);
    const auto& patch = activePatch();
    sustainedNotes[static_cast<size_t> (note)] = false;

    if (patch.arp.enabled)
    {
        if (arpHeldCount == 0)
        {
            arpStepIndex = 0;
            arpDirection = 1;
            arpSamplesToStep = 0.0;
        }

        bool alreadyHeld = false;

        for (int i = 0; i < arpHeldCount; ++i)
            if (arpHeldNotes[static_cast<size_t> (i)] == note)
                alreadyHeld = true;

        if (! alreadyHeld && arpHeldCount < static_cast<int> (arpHeldNotes.size()))
        {
            arpHeldNotes[static_cast<size_t> (arpHeldCount)] = note;
            arpHeldVelocities[static_cast<size_t> (arpHeldCount)] = velocity;
            ++arpHeldCount;
        }

        return;
    }

    if (patch.voiceMode == VoiceMode::Mono || patch.voiceMode == VoiceMode::Legato)
    {
        int existing = -1;

        for (int i = 0; i < monoHeldCount; ++i)
            if (monoHeld[static_cast<size_t> (i)] == note)
                existing = i;

        if (existing >= 0)
        {
            for (int i = existing; i + 1 < monoHeldCount; ++i)
                monoHeld[static_cast<size_t> (i)] = monoHeld[static_cast<size_t> (i + 1)];
            --monoHeldCount;
        }

        if (monoHeldCount < static_cast<int> (monoHeld.size()))
            monoHeld[static_cast<size_t> (monoHeldCount++)] = note;

        if (patch.voiceMode == VoiceMode::Legato && voices[0].isActive()
            && ! voices[0].isReleasing())
        {
            voices[0].retarget (note, velocity);
        }
        else
        {
            const float glideFrom = voices[0].isActive()
                                        ? static_cast<float> (voices[0].getNote())
                                        : static_cast<float> (note);
            startVoice (0, note, velocity, 1, glideFrom);
        }

        return;
    }

    const int layers = patch.voiceMode == VoiceMode::Unison
                           ? jlimit (1, kMaxUnisonLayers, patch.unisonVoices)
                           : 1;
    const int maximumNotes = std::max (1, kMaxOscillatorLayers / layers);
    const int activeCount = countActiveVoices();

    for (int i = 0; i < kNumVoices; ++i)
    {
        auto& voice = voices[static_cast<size_t> (i)];

        if (voice.isActive() && ! voice.isReleasing() && voice.getNote() == note)
        {
            startVoice (i, note, velocity, layers, static_cast<float> (note));
            return;
        }
    }

    int destination = activeCount >= maximumNotes ? chooseActiveVoiceToSteal()
                                                  : chooseVoiceToSteal();

    startVoice (destination, note, velocity, layers, static_cast<float> (note));
}

void SynthEngine::handleNoteOff (int note)
{
    if (note < 0 || note > 127)
        return;

    const auto& patch = activePatch();

    if (patch.arp.enabled)
    {
        if (! patch.arp.latch)
        {
            int found = -1;

            for (int i = 0; i < arpHeldCount; ++i)
                if (arpHeldNotes[static_cast<size_t> (i)] == note)
                    found = i;

            if (found >= 0)
            {
                for (int i = found; i + 1 < arpHeldCount; ++i)
                {
                    arpHeldNotes[static_cast<size_t> (i)] = arpHeldNotes[static_cast<size_t> (i + 1)];
                    arpHeldVelocities[static_cast<size_t> (i)] = arpHeldVelocities[static_cast<size_t> (i + 1)];
                }
                --arpHeldCount;
            }
        }

        if (arpCurrentNote == note)
        {
            voices[0].release();
            arpCurrentNote = -1;
        }

        if (arpHeldCount == 0)
            arpCurrentNote = -1;

        return;
    }

    if (patch.voiceMode == VoiceMode::Mono || patch.voiceMode == VoiceMode::Legato)
    {
        int found = -1;

        for (int i = 0; i < monoHeldCount; ++i)
            if (monoHeld[static_cast<size_t> (i)] == note)
                found = i;

        if (found >= 0)
        {
            for (int i = found; i + 1 < monoHeldCount; ++i)
                monoHeld[static_cast<size_t> (i)] = monoHeld[static_cast<size_t> (i + 1)];
            --monoHeldCount;
        }

        if (sustainDown)
        {
            sustainedNotes[static_cast<size_t> (note)] = true;
            return;
        }

        if (monoHeldCount > 0)
        {
            const int fallback = monoHeld[static_cast<size_t> (monoHeldCount - 1)];
            const float glideFrom = static_cast<float> (voices[0].getNote());

            if (patch.voiceMode == VoiceMode::Legato)
                voices[0].retarget (fallback, voices[0].getVelocity());
            else
                startVoice (0, fallback, voices[0].getVelocity(), 1, glideFrom);
        }
        else
        {
            voices[0].release();
        }

        return;
    }

    if (sustainDown)
    {
        sustainedNotes[static_cast<size_t> (note)] = true;
        return;
    }

    for (auto& voice : voices)
        if (voice.isActive() && voice.getNote() == note)
            voice.release();
}

void SynthEngine::updateSustain (bool down)
{
    sustainDown = down;

    if (down)
        return;

    for (auto& voice : voices)
        if (voice.isActive() && sustainedNotes[static_cast<size_t> (voice.getNote())])
            voice.release();

    sustainedNotes.fill (false);
}

void SynthEngine::handleProgramChange (int program)
{
    if (program >= 0 && program < kNumPresets)
        midiProgramRequest = program;

    midiCounter.fetch_add (1, std::memory_order_relaxed);
}

void SynthEngine::allNotesOff (bool immediate)
{
    for (auto& voice : voices)
    {
        if (immediate)
            voice.forceStop();
        else if (voice.isActive())
            voice.release();
    }

    clearArp();
    monoHeldCount = 0;
    sustainedNotes.fill (false);
    sustainDown = false;
}

void SynthEngine::handleMidiMessage (const juce::MidiMessage& message)
{
    if (message.isProgramChange())
    {
        handleProgramChange (message.getProgramChangeNumber());
        return;
    }

    if (message.isNoteOn())
    {
        handleNoteOn (message.getNoteNumber(), message.getFloatVelocity());
        return;
    }

    if (message.isNoteOff())
    {
        handleNoteOff (message.getNoteNumber());
        return;
    }

    if (message.isPitchWheel())
    {
        pitchBend = jlimit (-1.0f, 1.0f,
                            (static_cast<float> (message.getPitchWheelValue()) - 8192.0f) / 8192.0f);
        midiCounter.fetch_add (1, std::memory_order_relaxed);
        return;
    }

    if (message.isChannelPressure())
    {
        aftertouch = static_cast<float> (message.getChannelPressureValue()) / 127.0f;
        midiCounter.fetch_add (1, std::memory_order_relaxed);
        return;
    }

    if (message.isController())
    {
        const int controller = message.getControllerNumber();
        const int value = message.getControllerValue();

        if (controller == 1)
            modWheel = static_cast<float> (value) / 127.0f;
        else if (controller == 64)
            updateSustain (value >= 64);
        else if (controller == 123)
            allNotesOff (false);
        else if (controller == 120)
            allNotesOff (true);

        midiCounter.fetch_add (1, std::memory_order_relaxed);
    }
}

void SynthEngine::triggerArpStep (double bpm)
{
    const auto& patch = activePatch();

    if (arpHeldCount <= 0)
        return;

    std::array<int, 16> ordered = arpHeldNotes;
    const int count = arpHeldCount;
    const int octaves = jlimit (1, 3, patch.arp.octaves);
    const int total = count * octaves;

    if (patch.arp.pattern != 4)
        std::sort (ordered.begin(), ordered.begin() + count);

    const int sequence = arpStepIndex++;
    int step = 0;

    switch (patch.arp.pattern)
    {
        case 0: step = sequence % total; break;
        case 1: step = total - 1 - (sequence % total); break;
        case 2:
        {
            const int period = std::max (1, total * 2 - 2);
            const int phase = sequence % period;
            step = phase < total ? phase : total * 2 - 2 - phase;
            break;
        }
        case 3:
        {
            arpRandomState ^= arpRandomState << 13;
            arpRandomState ^= arpRandomState >> 17;
            arpRandomState ^= arpRandomState << 5;
            step = static_cast<int> (arpRandomState % static_cast<std::uint32_t> (total));
            break;
        }
        case 4:
        default: step = sequence % total; break;
    }

    const int noteIndex = step % count;
    const int octave = step / count;
    const int note = jlimit (0, 127, ordered[static_cast<size_t> (noteIndex)] + octave * 12);
    const float velocity = arpHeldVelocities[static_cast<size_t> (noteIndex)];

    startVoice (0, note, velocity, 1, static_cast<float> (note));
    arpCurrentNote = note;

    const double stepSamples = std::max (16.0, (60.0 / std::max (20.0, bpm))
                                               * (static_cast<double> (patch.arp.rateDiv) / 16.0)
                                               * sampleRate);
    arpGateCountdown = stepSamples * std::clamp (patch.arp.gate, 0.05f, 0.95f);
}

void SynthEngine::updateArp (int numSamples, double bpm)
{
    const auto& patch = activePatch();

    if (! patch.arp.enabled)
        return;

    const double stepSamples = std::max (16.0, (60.0 / std::max (20.0, bpm))
                                               * (static_cast<double> (patch.arp.rateDiv) / 16.0)
                                               * sampleRate);

    if (arpHeldCount <= 0)
    {
        if (arpCurrentNote >= 0)
        {
            voices[0].release();
            arpCurrentNote = -1;
        }

        arpSamplesToStep = 0.0;
        arpGateCountdown = 0.0;
        return;
    }

    if (arpSamplesToStep <= 0.0)
    {
        triggerArpStep (bpm);
        arpSamplesToStep = stepSamples;
    }

    if (arpGateCountdown > 0.0)
    {
        arpGateCountdown -= numSamples;

        if (arpGateCountdown <= 0.0 && arpCurrentNote >= 0)
        {
            voices[0].release();
            arpCurrentNote = -1;
        }
    }

    arpSamplesToStep -= numSamples;
}

void SynthEngine::renderSegment (juce::AudioBuffer<float>& destination,
                                 int destinationOffset, int numSamples, double bpm)
{
    if (numSamples <= 0 || destination.getNumChannels() <= 0)
        return;

    const auto& patch = activePatch();
    updateArp (numSamples, bpm);

    const float globalOne = globalLfo1.advance (numSamples, lfoRateHz (patch.lfo1, bpm));
    const float globalTwo = globalLfo2.advance (numSamples, lfoRateHz (patch.lfo2, bpm));

    scratch.clear (0, numSamples);
    float* left = scratch.getWritePointer (0);
    float* right = scratch.getWritePointer (1);

    float chorusMod = 0.0f;
    float delayMod = 0.0f;
    float reverbMod = 0.0f;
    int activeVoices = 0;

    for (auto& voice : voices)
    {
        if (! voice.isActive())
            continue;

        voice.process (patch, left, right, numSamples, globalOne, globalTwo, bpm);
        chorusMod += voice.getChorusDepthMod();
        delayMod += voice.getDelaySendMod();
        reverbMod += voice.getReverbSendMod();
        ++activeVoices;
    }

    if (activeVoices > 0)
    {
        chorusMod /= static_cast<float> (activeVoices);
        delayMod /= static_cast<float> (activeVoices);
        reverbMod /= static_cast<float> (activeVoices);
    }

    ChorusParams chorusParams = patch.chorus;
    chorus.setParams (chorusParams);

    PhaserParams phaserParams = patch.phaser;
    phaser.setParams (phaserParams);

    DelayParams delayParams = patch.delay;
    delayParams.mix = std::clamp (delayParams.mix * (1.0f + delayMod * 0.7f), 0.0f, 1.0f);
    delay.setParams (delayParams, bpm);

    ReverbParams reverbParams = patch.reverb;
    reverbParams.mix = std::clamp (reverbParams.mix * (1.0f + reverbMod * 0.7f), 0.0f, 1.0f);
    reverb.setParams (reverbParams);
    outputStage.setTrimDb (patch.outputTrimDb);

    const float driveNorm = 0.7f + patch.drive * 2.8f;
    const float driveMakeup = 1.0f + patch.drive * 0.25f;

    float localPeakL = 0.0f;
    float localPeakR = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float sampleL = left[i];
        float sampleR = right[i];

        sampleL = std::tanh (sampleL * driveNorm) / driveNorm * driveMakeup;
        sampleR = std::tanh (sampleR * driveNorm) / driveNorm * driveMakeup;

        chorus.process (sampleL, sampleR, chorusMod);
        phaser.process (sampleL, sampleR);
        delay.process (sampleL, sampleR);
        reverb.process (sampleL, sampleR);
        outputStage.process (sampleL, sampleR);

        if (fadeState == FadeState::Out)
            fadeGain = std::max (0.0f, fadeGain - fadeIncrement);
        else if (fadeState == FadeState::In)
            fadeGain = std::min (1.0f, fadeGain + fadeIncrement);

        sampleL *= fadeGain;
        sampleR *= fadeGain;
        localPeakL = std::max (localPeakL, std::abs (sampleL));
        localPeakR = std::max (localPeakR, std::abs (sampleR));

        const int destinationIndex = destinationOffset + i;
        destination.setSample (0, destinationIndex,
                               destination.getNumChannels() == 1 ? 0.5f * (sampleL + sampleR) : sampleL);

        if (destination.getNumChannels() > 1)
            destination.setSample (1, destinationIndex, sampleR);

        for (int ch = 2; ch < destination.getNumChannels(); ++ch)
            destination.setSample (ch, destinationIndex, 0.0f);
    }

    peakLeft.store (std::max (peakLeft.load (std::memory_order_relaxed), localPeakL),
                    std::memory_order_relaxed);
    peakRight.store (std::max (peakRight.load (std::memory_order_relaxed), localPeakR),
                     std::memory_order_relaxed);
}

void SynthEngine::switchToProgram (int index)
{
    activeIndex = jlimit (0, kNumPresets - 1, index);
    targetIndex = activeIndex;
    appliedProgram.store (activeIndex, std::memory_order_release);
    resetVoicesAndEffects();
    configureActivePatch();
    fadeGain = 0.0f;
    fadeState = FadeState::In;
    startOrderCounter = 0;
}

void SynthEngine::process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi,
                           int pendingProgram, double bpm)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0 || buffer.getNumChannels() <= 0)
        return;

    peakLeft.store (0.0f, std::memory_order_relaxed);
    peakRight.store (0.0f, std::memory_order_relaxed);

    int requested = jlimit (0, kNumPresets - 1, pendingProgram);

    if (midiProgramRequest >= 0)
    {
        requested = jlimit (0, kNumPresets - 1, midiProgramRequest);
        midiProgramRequest = -1;
    }

    if (fadeState == FadeState::Idle)
    {
        if (requested != activeIndex)
        {
            targetIndex = requested;
            fadeState = FadeState::Out;
            fadeGain = 1.0f;
        }
    }
    else if (fadeState == FadeState::Out)
    {
        if (requested != activeIndex)
            targetIndex = requested;
    }
    else // FadeState::In
    {
        if (requested == activeIndex)
        {
            targetIndex = activeIndex;
        }
        else if (requested != targetIndex)
        {
            targetIndex = requested;
            fadeState = FadeState::Out;
        }
    }

    int cursor = 0;

    for (const auto metadata : midi)
    {
        const int position = jlimit (cursor, numSamples, metadata.samplePosition);

        if (position > cursor)
        {
            renderSegment (buffer, cursor, position - cursor, bpm);
            cursor = position;
        }

        handleMidiMessage (metadata.getMessage());
    }

    if (cursor < numSamples)
        renderSegment (buffer, cursor, numSamples - cursor, bpm);

    if (fadeState == FadeState::Out && fadeGain <= 0.0f)
        switchToProgram (targetIndex);

    voiceCountMeter.store (countActiveVoices(), std::memory_order_relaxed);
}
} // namespace synthset
