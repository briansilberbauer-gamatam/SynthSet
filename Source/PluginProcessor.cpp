#include "PluginProcessor.h"

#include "PluginEditor.h"

#include <algorithm>

SynthSetAudioProcessor::SynthSetAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    hostDisplayedProgram = presetManager.getAppliedProgram();
    startTimerHz (10);
}

SynthSetAudioProcessor::~SynthSetAudioProcessor()
{
    stopTimer();
}

void SynthSetAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    hostBpm = 120.0;
    engine.prepare (sampleRate, samplesPerBlock);
    setLatencySamples (0);
}

void SynthSetAudioProcessor::releaseResources()
{
    engine.reset();
}

void SynthSetAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    if (buffer.getNumChannels() <= 0 || buffer.getNumSamples() <= 0)
        return;

    double bpm = 120.0;

    if (auto* transport = getPlayHead())
    {
        if (auto position = transport->getPosition())
        {
            const auto bpmValue = position->getBpm();

            if (bpmValue.hasValue())
                bpm = std::clamp (*bpmValue, 20.0, 300.0);
        }
    }

    hostBpm = bpm;
    engine.process (buffer, midiMessages, presetManager.getPendingProgram(), hostBpm);

    const int applied = engine.getAppliedProgram();

    if (applied != presetManager.getAppliedProgram())
    {
        presetManager.markApplied (applied);
        presetManager.requestProgram (applied);
    }

}

void SynthSetAudioProcessor::timerCallback()
{
    const int applied = presetManager.getAppliedProgram();

    if (applied != hostDisplayedProgram)
    {
        hostDisplayedProgram = applied;
        updateHostDisplay (juce::AudioProcessor::ChangeDetails().withProgramChanged (true));
    }
}

juce::AudioProcessorEditor* SynthSetAudioProcessor::createEditor()
{
    return new SynthSetAudioProcessorEditor (*this);
}

int SynthSetAudioProcessor::getNumPrograms()
{
    return synthset::kNumPresets;
}

int SynthSetAudioProcessor::getCurrentProgram()
{
    return presetManager.getAppliedProgram();
}

void SynthSetAudioProcessor::setCurrentProgram (int index)
{
    // Some VST3 hosts replay a stale program parameter immediately after
    // restoring component state. Ignore that single stale value so the
    // preset ID stored in our state remains authoritative.
    if (restoreGuardProgram >= 0
        && index == restoreGuardProgram
        && juce::Time::getMillisecondCounter() < restoreGuardUntil)
        return;

    presetManager.requestProgram (index);
}

const juce::String SynthSetAudioProcessor::getProgramName (int index)
{
    if (index < 0 || index >= synthset::kNumPresets)
        return {};

    return synthset::getFactoryBank()[static_cast<size_t> (index)].name;
}

void SynthSetAudioProcessor::changeProgramName (int, const juce::String&)
{
    // The factory bank is immutable.
}

void SynthSetAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    presetManager.getStateInformation (destData);
}

void SynthSetAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    restoreGuardProgram = presetManager.getAppliedProgram();
    restoreGuardUntil = juce::Time::getMillisecondCounter() + 750.0;
    presetManager.setStateInformation (data, sizeInBytes);
}

bool SynthSetAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();

    if (output != juce::AudioChannelSet::mono()
        && output != juce::AudioChannelSet::stereo())
        return false;

    const auto input = layouts.getMainInputChannelSet();

    if (! input.isDisabled() && input != output)
        return false;

    return true;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SynthSetAudioProcessor();
}
