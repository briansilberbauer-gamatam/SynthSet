#include "StatusStrip.h"
#include "Theme.h"

namespace synthset::ui
{
void StatusStrip::setPreset (const Preset& preset)
{
    current = &preset;
    repaint();
}

void StatusStrip::setLive (int voiceCount, bool midiActive)
{
    if (voices == voiceCount && midi == midiActive)
        return;

    voices = voiceCount;
    midi = midiActive;
    repaint();
}

void StatusStrip::paint (juce::Graphics& g)
{
    if (current == nullptr)
        return;

    const auto accent = colourFrom (current->artwork.accent);
    int indexInCollection = 0;
    int collectionTotal = 0;

    for (const auto& preset : getFactoryBank())
    {
        if (preset.group == current->group && preset.collection == current->collection)
        {
            ++collectionTotal;

            if (&preset == current)
                ++indexInCollection;
        }
    }

    const auto label = juce::String (groupSlug (current->group)) + " / "
                       + collectionSlug (current->collection) + "  ·  "
                       + juce::String (indexInCollection) + " OF "
                       + juce::String (collectionTotal);

    g.setColour (textMuted);
    g.setFont (font (9.4f, FontWeight::semibold, 0.14f));
    g.drawText (label, 2, 6, 520, 24, juce::Justification::centredLeft, false);

    const auto voiceText = juce::String (voices) + (voices == 1 ? " VOICE" : " VOICES");
    g.setColour (textSecondary);
    g.setFont (font (9.4f, FontWeight::semibold, 0.13f));
    g.drawText (voiceText, getWidth() - 112, 6, 110, 24,
                juce::Justification::centredRight, false);

    const auto dotCentre = juce::Point<float> (static_cast<float> (getWidth()) - 168.0f, 18.0f);
    g.setColour (midi ? accent : textMuted.withAlpha (0.4f));
    g.fillEllipse (dotCentre.x - 2.5f, dotCentre.y - 2.5f, 5.0f, 5.0f);

    if (midi)
    {
        g.setColour (accent.withAlpha (0.12f));
        g.fillEllipse (dotCentre.x - 6.0f, dotCentre.y - 6.0f, 12.0f, 12.0f);
    }

    g.setColour (midi ? textPrimary.withAlpha (0.82f) : textMuted.withAlpha (0.56f));
    g.setFont (font (9.0f, FontWeight::semibold, 0.15f));
    g.drawText ("MIDI", getWidth() - 160, 6, 44, 24,
                juce::Justification::centredLeft, false);

    g.setColour (hairline.withAlpha (0.32f));
    g.fillRect (0, getHeight() - 1, getWidth(), 1);
}
} // namespace synthset::ui
