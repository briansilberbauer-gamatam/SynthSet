#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include <cmath>
#include <iostream>
#include <set>

namespace
{
int failures = 0;

void check (bool condition, const char* message)
{
    std::cout << (condition ? "  PASS  " : "  FAIL  ") << message << '\n';

    if (! condition)
    {
        std::cout << "::error::" << message << '\n';
        ++failures;
    }
}

bool allFinite (const juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            if (! std::isfinite (buffer.getReadPointer (ch)[i]))
                return false;

    return true;
}

void silence (juce::AudioBuffer<float>& buffer)
{
    buffer.clear();
}

void renderBlocks (juce::AudioProcessor& processor, juce::AudioBuffer<float>& buffer,
                   int blocks, juce::MidiBuffer firstMidi = {})
{
    for (int block = 0; block < blocks; ++block)
    {
        silence (buffer);
        juce::MidiBuffer midi = block == 0 ? firstMidi : juce::MidiBuffer {};
        processor.processBlock (buffer, midi);
    }
}
} // namespace

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    if (argc < 2)
    {
        std::cerr << "Usage: SynthSetHostSmokeTest /path/to/SynthSet.vst3\n";
        return 2;
    }

    const juce::File pluginFile { juce::String (argv[1]) };

    if (! pluginFile.exists())
    {
        std::cerr << "Plugin bundle not found: " << pluginFile.getFullPathName() << '\n';
        return 2;
    }

    juce::AudioPluginFormatManager formatManager;
    formatManager.addDefaultFormats();

    juce::OwnedArray<juce::PluginDescription> descriptions;

    for (auto* format : formatManager.getFormats())
        if (auto* vst3 = dynamic_cast<juce::VST3PluginFormat*> (format))
            vst3->findAllTypesForFile (descriptions, pluginFile.getFullPathName());

    check (! descriptions.isEmpty(), "VST3 scanner discovers SynthSet");

    if (descriptions.isEmpty())
        return 1;

    auto& description = *descriptions[0];
    std::cout << "  INFO  " << description.name << " / " << description.manufacturerName
              << " / " << description.version << '\n';

    juce::String error;
    auto instance = formatManager.createPluginInstance (description, 48000.0, 512, error);
    check (instance != nullptr, "plugin instance can be created");

    if (instance == nullptr)
    {
        std::cerr << "  ERROR " << error << '\n';
        return 1;
    }

    int soundParameters = 0;

    for (auto* parameter : instance->getParameters())
    {
        const auto name = parameter->getName (64);
        std::cout << "  INFO  host parameter: " << name << '\n';

        if (name != "Program" && name != "Bypass")
            ++soundParameters;
    }

    std::cout << "  INFO  host parameter count: " << instance->getParameters().size()
              << " (sound: " << soundParameters << ")\n";
    check (soundParameters == 0,
           "the host sees no sound parameters beyond VST3 Program/Bypass");
    check (instance->getNumPrograms() == 48, "48 factory programs are exposed");
    check (instance->acceptsMidi(), "MIDI input is accepted");
    check (! instance->producesMidi(), "MIDI output is not advertised");
    check (instance->getBusCount (false) >= 1, "at least one output bus is exposed");
    check (instance->getLatencySamples() == 0, "reported latency is zero");

    std::set<juce::String> names;

    for (int i = 0; i < instance->getNumPrograms(); ++i)
        names.insert (instance->getProgramName (i));

    check (names.size() == 48, "all program names are unique");

    instance->setPlayConfigDetails (2, 2, 48000.0, 512);
    instance->prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer chord;

    for (const int note : { 48, 52, 55, 60, 64, 67, 72, 76 })
        chord.addEvent (juce::MidiMessage::noteOn (1, note, 0.72f), 12 + note % 11);

    silence (buffer);
    instance->processBlock (buffer, chord);
    check (allFinite (buffer), "a chord produces finite output");

    float peak = 0.0f;

    for (int block = 0; block < 36; ++block)
    {
        silence (buffer);
        juce::MidiBuffer midi;
        const int note = 40 + (block % 20);
        midi.addEvent (juce::MidiMessage::noteOn (1, note, 0.65f), block % 64);
        midi.addEvent (juce::MidiMessage::noteOff (1, note), 320);
        instance->processBlock (buffer, midi);
        peak = std::max (peak, buffer.getMagnitude (0, buffer.getNumSamples()));
        check (allFinite (buffer), "note rendering remains finite");
    }

    check (peak > 0.0001f, "note rendering produces audible output");

    const int targetProgram = 31;
    instance->setCurrentProgram (targetProgram);
    renderBlocks (*instance, buffer, 24);
    check (instance->getCurrentProgram() == targetProgram,
           "setCurrentProgram is applied through the fade");

    instance->setCurrentProgram (10);
    renderBlocks (*instance, buffer, 24);
    check (instance->getCurrentProgram() == 10,
           "host program change selects a factory preset");

    juce::MemoryBlock state;
    instance->getStateInformation (state);
    check (state.getSize() > 0, "state serialises a compact preset ID");

    const int savedProgram = instance->getCurrentProgram();
    std::cout << "  INFO  saved program: " << savedProgram << '\n';
    instance->releaseResources();

    juce::String restoreError;
    auto restoredInstance = formatManager.createPluginInstance (description, 48000.0, 512, restoreError);
    check (restoredInstance != nullptr, "a fresh host instance can be created for state restore");

    if (restoredInstance != nullptr)
    {
        restoredInstance->setPlayConfigDetails (2, 2, 48000.0, 512);
        restoredInstance->prepareToPlay (48000.0, 512);
        restoredInstance->setStateInformation (state.getData(), static_cast<int> (state.getSize()));
        renderBlocks (*restoredInstance, buffer, 100);

        // Hosts surface program changes asynchronously. Wait for the
        // message-thread program notification instead of relying on a single
        // 80 ms pump, which is racy on slower Windows CI runners.
        for (int attempt = 0; attempt < 80
             && restoredInstance->getCurrentProgram() != savedProgram; ++attempt)
            juce::MessageManager::getInstance()->runDispatchLoopUntil (20);

        std::cout << "  INFO  restored program: " << restoredInstance->getCurrentProgram() << '\n';
        check (restoredInstance->getCurrentProgram() == savedProgram,
               "state restores the saved preset ID in a fresh instance");
        restoredInstance->releaseResources();
    }

    std::cout << (failures == 0 ? "\nHost smoke test passed.\n" : "\nHost smoke test failed.\n");
    return failures == 0 ? 0 : 1;
}
