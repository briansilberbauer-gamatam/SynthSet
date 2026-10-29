#include "EditorContent.h"
#include "Theme.h"
#include "../Version.h"

#include <array>
#include <cmath>

namespace synthset::ui
{
namespace
{
juce::String oscillatorText (const Preset& preset)
{
    const auto& p = preset.patch;
    int count = 0;

    if (p.osc1.level > 0.01f) ++count;
    if (p.osc2.level > 0.01f) ++count;

    juce::String text = juce::String (count) + " OSC";

    if (p.subLevel > 0.01f) text << " + SUB";
    if (p.noiseLevel > 0.01f) text << " + NOISE";
    if (p.ringLevel > 0.01f) text << " + RING";
    return text;
}

juce::String filterText (const Preset& preset)
{
    const auto model = preset.patch.filter.model == FilterModel::Ladder4P
                           ? "4-POLE LADDER" : "2-POLE SVF";
    const auto type = preset.patch.filter.type == FilterType::LowPass
                          ? "LP" : preset.patch.filter.type == FilterType::HighPass ? "HP" : "BP";
    return juce::String (model) + " · " + type;
}

juce::String motionText (const Preset& preset)
{
    const auto& p = preset.patch;

    if (p.arp.enabled)
        return juce::String ("ARP 1/") + juce::String (p.arp.rateDiv == 4 ? 16 : p.arp.rateDiv == 8 ? 8 : 4);

    bool hasLfoRoute = false;

    for (const auto& route : p.mod)
        if ((route.source == ModSource::Lfo1 || route.source == ModSource::Lfo2)
            && route.dest != ModDest::None)
            hasLfoRoute = true;

    if (! hasLfoRoute)
        return "STATIC";

    if (p.lfo1.tempoSync || p.lfo2.tempoSync)
        return "SYNC LFO";

    return "LFO " + juce::String (p.lfo1.rateHz, 2) + " HZ";
}

juce::String spaceText (const Preset& preset)
{
    const auto& p = preset.patch;
    juce::StringArray parts;

    if (p.chorus.mode == 1) parts.add ("BBD I");
    else if (p.chorus.mode == 2) parts.add ("BBD II");
    else if (p.chorus.mode == 3) parts.add ("ENSEMBLE");

    if (p.delay.mix > 0.02f) parts.add ("DELAY");
    if (p.reverb.mix > 0.02f) parts.add ("PLATE");
    if (p.phaser.stages > 0) parts.add ("PHASER");

    return parts.isEmpty() ? "DRY" : parts.joinIntoString (" · ");
}

void bloom (juce::Graphics& g, juce::Point<float> centre, float radius,
            juce::Colour colour, float alpha)
{
    juce::ColourGradient gradient (colour.withAlpha (alpha), centre.x, centre.y,
                                    colour.withAlpha (0.0f), centre.x + radius, centre.y, true);
    gradient.addColour (0.45, colour.withAlpha (alpha * 0.45f));
    gradient.addColour (0.75, colour.withAlpha (alpha * 0.12f));
    g.setGradientFill (gradient);
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
}

void drawCellIcon (juce::Graphics& g, juce::Rectangle<float> area, int index,
                   juce::Colour accent)
{
    g.setColour (accent.withAlpha (0.8f));

    if (index == 0)
    {
        juce::Path wave;
        wave.startNewSubPath (area.getX(), area.getCentreY());

        for (int i = 0; i <= 16; ++i)
        {
            const float t = static_cast<float> (i) / 16.0f;
            const float x = area.getX() + area.getWidth() * t;
            const float y = area.getCentreY() - std::sin (t * juce::MathConstants<float>::twoPi * 2.0f)
                                                    * area.getHeight() * 0.28f;
            wave.lineTo (x, y);
        }

        g.strokePath (wave, juce::PathStrokeType (1.1f));
    }
    else if (index == 1)
    {
        juce::Path curve;
        curve.startNewSubPath (area.getX(), area.getY() + area.getHeight() * 0.35f);
        curve.cubicTo (area.getCentreX(), area.getY() + area.getHeight() * 0.30f,
                       area.getX() + area.getWidth() * 0.55f, area.getBottom(),
                       area.getRight(), area.getBottom());
        g.strokePath (curve, juce::PathStrokeType (1.1f));
        g.setColour (accent.withAlpha (0.35f));
        g.drawLine (area.getX(), area.getY() + area.getHeight() * 0.35f,
                    area.getRight(), area.getY() + area.getHeight() * 0.35f, 0.6f);
    }
    else if (index == 2)
    {
        for (int i = 0; i < 5; ++i)
        {
            const float x = area.getX() + area.getWidth() * static_cast<float> (i) / 4.0f;
            const float y = area.getCentreY() + ((i % 2 == 0) ? -1.0f : 1.0f) * area.getHeight() * 0.24f;
            g.fillEllipse (x - 1.4f, y - 1.4f, 2.8f, 2.8f);
        }
        g.setColour (accent.withAlpha (0.25f));
        g.drawLine (area.getX(), area.getCentreY(), area.getRight(), area.getCentreY(), 0.6f);
    }
    else
    {
        for (int i = 0; i < 3; ++i)
        {
            const float inset = static_cast<float> (i) * 4.2f;
            g.setColour (accent.withAlpha (0.72f - 0.18f * static_cast<float> (i)));
            g.drawEllipse (area.reduced (inset).toFloat(), 0.8f);
        }
    }
}

void drawDetailCell (juce::Graphics& g, juce::Rectangle<float> bounds, int index,
                     const juce::String& label, const juce::String& value,
                     juce::Colour accent, bool divider)
{
    if (divider)
    {
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.fillRect (bounds.getX(), bounds.getY() + 20.0f, 1.0f, bounds.getHeight() - 40.0f);
    }

    const auto icon = juce::Rectangle<float> (bounds.getRight() - 64.0f, bounds.getY() + 24.0f,
                                               42.0f, 24.0f);
    drawCellIcon (g, icon, index, accent);

    g.setColour (textMuted);
    g.setFont (font (8.3f, FontWeight::semibold, 0.17f));
    g.drawText (label, bounds.withTrimmedLeft (18.0f).withTrimmedTop (20.0f).withHeight (12.0f).toNearestInt(),
                juce::Justification::centredLeft, false);

    g.setColour (textPrimary.withAlpha (0.90f));
    g.setFont (font (11.4f, FontWeight::regular, 0.01f));
    g.drawFittedText (value,
                      bounds.withTrimmedLeft (18.0f).withTrimmedTop (37.0f)
                          .withWidth (bounds.getWidth() - 86.0f).withHeight (18.0f).toNearestInt(),
                      juce::Justification::centredLeft, 1, 0.8f);

    g.setColour (accent.withAlpha (0.20f));
    g.fillRoundedRectangle (bounds.getX() + 18.0f, bounds.getBottom() - 22.0f,
                            26.0f, 1.2f, 0.6f);
}
} // namespace

EditorContent::EditorContent (SynthSetAudioProcessor& p)
    : processor (p)
{
    setOpaque (true);
    setWantsKeyboardFocus (true);
    addMouseListener (this, true);

    addAndMakeVisible (rail);
    addAndMakeVisible (hero);
    addAndMakeVisible (status);

    rail.onPresetSelected = [this] (int index)
    {
        processor.setCurrentProgram (index);
        showPreset (index);
    };

    hero.setNavigationCallbacks ([this] { rail.step (-1); },
                                 [this] { rail.step (1); });

    displayedIndex = juce::jlimit (0, synthset::kNumPresets - 1,
                                   processor.getPresetManager().getPendingProgram());
    showPreset (displayedIndex);

    frozen = juce::SystemStats::getEnvironmentVariable ("SYNTHSET_FROZEN", "0") == "1";

    if (frozen)
        hero.setFrozen (true);
    else
        startTimerHz (30);
}

EditorContent::~EditorContent()
{
    stopTimer();
}

const Preset& EditorContent::currentPreset() const
{
    return synthset::getFactoryBank()[static_cast<size_t> (displayedIndex)];
}

void EditorContent::showPreset (int bankIndex)
{
    displayedIndex = juce::jlimit (0, synthset::kNumPresets - 1, bankIndex);
    const auto& preset = currentPreset();

    rail.setCurrentPreset (displayedIndex, true);
    hero.setPreset (preset, true);
    status.setPreset (preset);
    rebuildBackdrop();
    repaint();
}

void EditorContent::setFrozen (bool shouldFreeze)
{
    frozen = shouldFreeze;
    hero.setFrozen (frozen);

    if (frozen)
        stopTimer();
    else
        startTimerHz (30);
}

void EditorContent::timerCallback()
{
    const int pending = processor.getPresetManager().getPendingProgram();

    if (pending != displayedIndex)
        showPreset (pending);

    const auto counter = processor.getMidiCounter();

    if (counter != lastMidiCounter)
    {
        lastMidiCounter = counter;
        midiFlashUntil = juce::Time::getMillisecondCounterHiRes() + 320.0;
    }

    const bool midiActive = juce::Time::getMillisecondCounterHiRes() < midiFlashUntil;
    status.setLive (processor.getVoiceCount(), midiActive);
}

void EditorContent::resized()
{
    rail.setBounds (24, 24, 300, 752);
    status.setBounds (344, 24, 912, 36);
    hero.setBounds (344, 72, 912, 540);
}

bool EditorContent::keyPressed (const juce::KeyPress& key)
{
    if (key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown())
        return false;

    if (key == juce::KeyPress::leftKey)
    {
        rail.step (-1);
        return true;
    }

    if (key == juce::KeyPress::rightKey)
    {
        rail.step (1);
        return true;
    }

    if (key == juce::KeyPress::upKey)
    {
        rail.stepWithinGroup (-1);
        return true;
    }

    if (key == juce::KeyPress::downKey)
    {
        rail.stepWithinGroup (1);
        return true;
    }

    if (key == juce::KeyPress::homeKey)
    {
        rail.jumpToStart();
        return true;
    }

    if (key == juce::KeyPress::endKey)
    {
        rail.jumpToEnd();
        return true;
    }

    return false;
}

void EditorContent::mouseDown (const juce::MouseEvent&)
{
    grabKeyboardFocus();
}

void EditorContent::visibilityChanged()
{
    if (isVisible() && ! frozen)
        startTimerHz (30);
    else
        stopTimer();
}

void EditorContent::parentHierarchyChanged()
{
    if (isShowing() && ! frozen)
        startTimerHz (30);
    else
        stopTimer();
}

void EditorContent::paint (juce::Graphics& g)
{
    if (backdrop.isValid())
        g.drawImageAt (backdrop, 0, 0);
    else
        g.fillAll (baseDeep);

    paintDetailStrip (g);

    // The stage footer is intentionally non-interactive.
    g.setColour (textMuted.withAlpha (0.58f));
    g.setFont (font (8.1f, FontWeight::regular, 0.10f));
    g.drawText (juce::String ("SYNTHSET v") + version + "  ·  BLS AUDIO  ·  FACTORY PRESET INSTRUMENT",
                344, 784, 912, 12, juce::Justification::centredRight, false);
}

void EditorContent::paintDetailStrip (juce::Graphics& g)
{
    const auto bounds = juce::Rectangle<float> (344.0f, 624.0f, 912.0f, 152.0f);
    const auto accent = colourFrom (currentPreset().artwork.accent);

    drawGlassPanel (g, bounds, 22.0f, accent, 0.85f);

    g.setColour (textMuted);
    g.setFont (font (8.2f, FontWeight::semibold, 0.18f));
    g.drawText ("PRESET RECIPE  /  READ-ONLY", 366, 636, 260, 12,
                juce::Justification::centredLeft, false);

    const std::array<juce::String, 4> labels { "OSCILLATORS", "FILTER", "MOTION", "SPACE" };
    const std::array<juce::String, 4> values {
        oscillatorText (currentPreset()), filterText (currentPreset()),
        motionText (currentPreset()), spaceText (currentPreset())
    };

    const float cellWidth = (bounds.getWidth() - 36.0f) / 4.0f;

    for (int i = 0; i < 4; ++i)
    {
        const auto cell = juce::Rectangle<float> (bounds.getX() + 18.0f + cellWidth * static_cast<float> (i),
                                                   bounds.getY() + 4.0f, cellWidth,
                                                   bounds.getHeight() - 24.0f);
        drawDetailCell (g, cell, i, labels[static_cast<size_t> (i)],
                        values[static_cast<size_t> (i)], accent, i > 0);
    }

    g.setColour (textMuted.withAlpha (0.6f));
    g.setFont (font (8.0f, FontWeight::regular, 0.08f));
    g.drawText ("NO EDITABLE PARAMETERS  ·  SELECT A PRESET TO CHANGE THE SOUND",
                366, 746, 860, 12, juce::Justification::centredLeft, false);
}

void EditorContent::rebuildBackdrop()
{
    constexpr int width = 1280;
    constexpr int height = 800;
    backdrop = juce::Image (juce::Image::ARGB, width, height, true);
    juce::Graphics g (backdrop);

    const auto& art = currentPreset().artwork;
    const auto accent = colourFrom (art.accent);
    const auto accent2 = colourFrom (art.accent2);
    const float tint = art.seed * 541.0f - std::floor (art.seed * 541.0f);
    const auto top = colourFrom (art.top).interpolatedWith (accent, 0.045f + 0.04f * tint);
    const auto mid = colourFrom (art.mid).interpolatedWith (accent2, 0.06f + 0.06f * (1.0f - tint));
    const auto bottom = colourFrom (art.bottom);

    juce::ColourGradient gradient (top, 0.0f, 0.0f, bottom, static_cast<float> (width),
                                    static_cast<float> (height), false);
    gradient.addColour (0.48, mid);
    g.setGradientFill (gradient);
    g.fillRect (0, 0, width, height);

    juce::Random random (static_cast<juce::int64> (art.seed * 1.0e9f) ^ 0x51f15e);

    bloom (g, { width * (0.60f + 0.20f * random.nextFloat()),
                height * (0.12f + 0.24f * random.nextFloat()) },
           width * 0.42f, accent, 0.16f * art.energy);
    bloom (g, { width * (0.15f + 0.22f * random.nextFloat()),
                height * (0.58f + 0.18f * random.nextFloat()) },
           width * 0.32f, accent2, 0.11f * art.energy);

    g.setColour (juce::Colours::black.withAlpha (0.48f));
    g.fillRect (0, 0, width, height);

    juce::ColourGradient vignette (juce::Colours::transparentBlack, width * 0.5f, height * 0.42f,
                                    juce::Colours::black.withAlpha (0.54f), 0.0f, 0.0f, true);
    g.setGradientFill (vignette);
    g.fillRect (0, 0, width, height);

    drawGrain (g, { 0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height) }, 0.35f);
}
} // namespace synthset::ui
