#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Version.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <iostream>
#include <memory>

#if JUCE_WINDOWS
 #include <cstdlib>
#else
 #include <cstdlib>
#endif

class SynthSetScreenshotApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "SynthSetScreenshot"; }
    const juce::String getApplicationVersion() override { return synthset::version; }
    bool moreThanOneInstanceAllowed() override { return true; }
    void anotherInstanceStarted (const juce::String&) override {}
    void systemRequestedQuit() override {}
    void shutdown() override {}

    void initialise (const juce::String& commandLine) override
    {
        juce::String presetId = synthset::defaultPresetId;
        juce::String outputPath = "synthset-ui.png";
        int width = 1280;
        int height = 800;
        float scale = 2.0f;

        const auto tokens = juce::StringArray::fromTokens (commandLine, true);

        for (int i = 0; i < tokens.size(); ++i)
        {
            const auto token = tokens[i];

            if (token.startsWith ("--preset="))
                presetId = token.substring (9);
            else if (token == "--preset" && i + 1 < tokens.size())
                presetId = tokens[++i];
            else if (token.startsWith ("--out="))
                outputPath = token.substring (6);
            else if (token == "--out" && i + 1 < tokens.size())
                outputPath = tokens[++i];
            else if (token.startsWith ("--scale="))
                scale = juce::jlimit (1.0f, 3.0f, token.substring (8).getFloatValue());
            else if (token.startsWith ("--width="))
                width = juce::jlimit (960, 1920, token.substring (8).getIntValue());
            else if (token.startsWith ("--height="))
                height = juce::jlimit (600, 1200, token.substring (9).getIntValue());
        }

        const int presetIndex = synthset::indexOfPresetId (presetId.toRawUTF8());

        if (presetIndex < 0)
        {
            std::cerr << "Unknown preset: " << presetId << std::endl;
            setApplicationReturnValue (2);
            quit();
            return;
        }

        outputFile = juce::File::isAbsolutePath (outputPath)
                         ? juce::File (outputPath)
                         : juce::File::getCurrentWorkingDirectory().getChildFile (outputPath);
        outputFile.getParentDirectory().createDirectory();

#if JUCE_WINDOWS
        _putenv_s ("SYNTHSET_FROZEN", "1");
#else
        setenv ("SYNTHSET_FROZEN", "1", 1);
#endif

        processor = std::make_unique<SynthSetAudioProcessor>();
        processor->setCurrentProgram (presetIndex);
        processor->setPlayConfigDetails (2, 2, 48000.0, 512);
        processor->prepareToPlay (48000.0, 512);

        editor = std::unique_ptr<juce::AudioProcessorEditor> (processor->createEditor());

        if (editor == nullptr)
        {
            std::cerr << "Could not create the SynthSet editor." << std::endl;
            setApplicationReturnValue (1);
            quit();
            return;
        }

        if (auto* synthEditor = dynamic_cast<SynthSetAudioProcessorEditor*> (editor.get()))
            synthEditor->setSnapshotMode (true);

        editor->setSize (width, height);
        editor->addToDesktop (0);
        editor->setTopLeftPosition (30, 30);
        editor->setVisible (true);
        editor->toFront (true);

        juce::Desktop::getInstance().getMainMouseSource().setScreenPosition ({ 3000.0f, 3000.0f });

        juce::Timer::callAfterDelay (450, [this, scale]
        {
            if (editor == nullptr)
                return;

            juce::Desktop::getInstance().getMainMouseSource().setScreenPosition ({ 3000.0f, 3000.0f });

            auto hideTooltips = [&] (auto&& self, juce::Component& component) -> void
            {
                for (auto* child : component.getChildren())
                {
                    if (dynamic_cast<juce::TooltipWindow*> (child) != nullptr)
                        child->setVisible (false);
                    else
                        self (self, *child);
                }
            };
            hideTooltips (hideTooltips, *editor);

            const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, scale);

            if (image.isValid())
            {
                outputFile.deleteFile();

                if (auto stream = std::unique_ptr<juce::FileOutputStream> (outputFile.createOutputStream()))
                {
                    juce::PNGImageFormat png;
                    const bool written = png.writeImageToStream (image, *stream);
                    stream->flush();

                    if (written)
                        std::cout << "Rendered " << outputFile.getFullPathName()
                                  << " (" << image.getWidth() << "x" << image.getHeight() << ")\n";
                    else
                        std::cerr << "Failed to encode PNG." << std::endl;
                }
                else
                {
                    std::cerr << "Could not open " << outputFile.getFullPathName() << std::endl;
                }
            }
            else
            {
                std::cerr << "Editor snapshot was invalid." << std::endl;
            }

            editor->removeFromDesktop();
            editor.reset();
            processor.reset();
            quit();
        });
    }

private:
    std::unique_ptr<SynthSetAudioProcessor> processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    juce::File outputFile;
};

START_JUCE_APPLICATION (SynthSetScreenshotApplication)
