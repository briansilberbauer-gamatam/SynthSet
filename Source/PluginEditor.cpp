#include "PluginEditor.h"

#include "ui/EditorContent.h"
#include "ui/Theme.h"

namespace
{
constexpr int designWidth = 1280;
constexpr int designHeight = 800;
} // namespace

SynthSetAudioProcessorEditor::SynthSetAudioProcessorEditor (SynthSetAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setOpaque (true);
    setWantsKeyboardFocus (true);

    content = std::make_unique<synthset::ui::EditorContent> (processor);
    addAndMakeVisible (*content);

    setResizable (true, true);
    setResizeLimits (960, 600, 1920, 1200);

    if (auto* boundsConstrainer = getConstrainer())
        boundsConstrainer->setFixedAspectRatio (static_cast<double> (designWidth) / static_cast<double> (designHeight));

    setSize (designWidth, designHeight);
}

SynthSetAudioProcessorEditor::~SynthSetAudioProcessorEditor()
{
    content.reset();
}

void SynthSetAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (synthset::ui::baseDeep);
}

void SynthSetAudioProcessorEditor::resized()
{
    if (content == nullptr)
        return;

    const float scale = juce::jmin (static_cast<float> (getWidth()) / static_cast<float> (designWidth),
                                    static_cast<float> (getHeight()) / static_cast<float> (designHeight));
    const float offsetX = (static_cast<float> (getWidth()) - static_cast<float> (designWidth) * scale) * 0.5f;
    const float offsetY = (static_cast<float> (getHeight()) - static_cast<float> (designHeight) * scale) * 0.5f;

    content->setBounds (0, 0, designWidth, designHeight);
    content->setTransform (juce::AffineTransform::scale (scale).translated (offsetX, offsetY));
}

bool SynthSetAudioProcessorEditor::keyPressed (const juce::KeyPress& key)
{
    return content != nullptr && content->keyPressed (key);
}

void SynthSetAudioProcessorEditor::setSnapshotMode (bool frozen)
{
    if (content != nullptr)
        content->setFrozen (frozen);
}
