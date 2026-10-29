#include "HeroPanel.h"
#include "Theme.h"

#include <array>
#include <cmath>

namespace synthset::ui
{
namespace
{
juce::String shortFilterName (const Preset& preset)
{
    const auto model = preset.patch.filter.model == FilterModel::Ladder4P
                           ? "4-POLE LADDER" : "2-POLE SVF";
    const auto type = preset.patch.filter.type == FilterType::LowPass
                          ? "LP" : preset.patch.filter.type == FilterType::HighPass ? "HP" : "BP";
    return juce::String (model) + " · " + type;
}

juce::String oscillatorName (const Preset& preset)
{
    const auto& p = preset.patch;
    juce::String text;
    int count = 0;

    if (p.osc1.level > 0.01f) ++count;
    if (p.osc2.level > 0.01f) ++count;

    text << count << (count == 1 ? " OSC" : " OSC");

    if (p.subLevel > 0.01f) text << " + SUB";
    if (p.noiseLevel > 0.01f) text << " + NOISE";
    if (p.ringLevel > 0.01f) text << " + RING";
    return text;
}

juce::String motionName (const Preset& preset)
{
    const auto& p = preset.patch;

    if (p.arp.enabled)
    {
        const int div = p.arp.rateDiv;
        return juce::String ("ARP 1/") + juce::String (div == 4 ? 16 : div == 8 ? 8 : 4);
    }

    bool hasLfoRoute = false;

    for (const auto& route : p.mod)
        if ((route.source == ModSource::Lfo1 || route.source == ModSource::Lfo2)
            && route.dest != ModDest::None)
            hasLfoRoute = true;

    if (! hasLfoRoute)
        return "STATIC";

    if (p.lfo1.tempoSync || p.lfo2.tempoSync)
        return "SYNC LFO";

    return "LFO MOTION";
}

juce::String spaceName (const Preset& preset)
{
    const auto& p = preset.patch;
    juce::StringArray parts;

    if (p.chorus.mode == 1) parts.add ("BBD I");
    else if (p.chorus.mode == 2) parts.add ("BBD II");
    else if (p.chorus.mode == 3) parts.add ("ENSEMBLE");

    if (p.delay.mix > 0.02f) parts.add ("DELAY");
    if (p.reverb.mix > 0.02f) parts.add ("PLATE");
    if (p.phaser.stages > 0) parts.add ("PHASER");

    if (parts.isEmpty())
        return "DRY";

    return parts.joinIntoString (" · ");
}

void drawChip (juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& text,
               juce::Colour accent, bool outlineOnly)
{
    if (outlineOnly)
    {
        g.setColour (juce::Colours::white.withAlpha (0.11f));
        g.fillRoundedRectangle (bounds, 6.0f);
        g.setColour (juce::Colours::white.withAlpha (0.20f));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 0.8f);
        g.setColour (textSecondary);
    }
    else
    {
        g.setColour (accent.withAlpha (0.11f));
        g.fillRoundedRectangle (bounds, 6.0f);
        g.setColour (accent.withAlpha (0.35f));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 0.8f);
        g.setColour (textPrimary.withAlpha (0.88f));
    }

    g.setFont (font (9.2f, FontWeight::semibold, 0.11f));
    g.drawFittedText (text, bounds.reduced (8.0f, 0.0f).toNearestInt(),
                      juce::Justification::centred, 1, 0.85f);
}

class ArrowButton final : public juce::Component
{
public:
    ArrowButton (bool shouldPointRight, std::function<void()> callback)
        : pointsRight (shouldPointRight), onClick (std::move (callback))
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setRepaintsOnMouseActivity (true);
    }

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (juce::Colours::white.withAlpha (isMouseOver() ? 0.13f : 0.065f));
        g.fillRoundedRectangle (bounds, 10.0f);
        g.setColour (juce::Colours::white.withAlpha (isMouseOver() ? 0.33f : 0.19f));
        g.drawRoundedRectangle (bounds, 10.0f, 0.9f);
        drawChevron (g, bounds, pointsRight,
                     isMouseOver() ? textPrimary : textSecondary, 1.5f);
    }

    void mouseUp (const juce::MouseEvent& event) override
    {
        if (event.mouseWasClicked() && onClick)
            onClick();
    }

private:
    bool pointsRight = false;
    std::function<void()> onClick;
};
} // namespace

class HeroPanel::Overlay final : public juce::Component
{
public:
    Overlay()
    {
        setInterceptsMouseClicks (false, true);
        previousButton = std::make_unique<ArrowButton> (false, [this] { if (onPrevious) onPrevious(); });
        nextButton = std::make_unique<ArrowButton> (true, [this] { if (onNext) onNext(); });
        addAndMakeVisible (*previousButton);
        addAndMakeVisible (*nextButton);
    }

    void setPreset (const Preset& preset)
    {
        current = &preset;
        repaint();
    }

    void setCallbacks (std::function<void()> previous, std::function<void()> next)
    {
        onPrevious = std::move (previous);
        onNext = std::move (next);
    }

    void resized() override
    {
        if (previousButton != nullptr)
            previousButton->setBounds (getWidth() - 88, 25, 34, 34);

        if (nextButton != nullptr)
            nextButton->setBounds (getWidth() - 46, 25, 34, 34);
    }

    void paint (juce::Graphics& g) override
    {
        if (current == nullptr)
            return;

        const auto bounds = getLocalBounds().toFloat();
        const auto& preset = *current;
        const auto accent = colourFrom (preset.artwork.accent);
        const bool classic = preset.collection == Collection::Classic;
        const auto collectionColour = classic ? juce::Colour (0xffffc46b) : accent;

        {
            juce::ColourGradient lowerScrim (juce::Colours::transparentBlack, 0.0f, bounds.getHeight() * 0.55f,
                                             juce::Colours::black.withAlpha (0.52f), 0.0f, bounds.getHeight(), false);
            g.setGradientFill (lowerScrim);
            g.fillRect (bounds.withTrimmedTop (bounds.getHeight() * 0.51f));
        }

        const auto badge = juce::Rectangle<float> (28.0f, 25.0f, 92.0f, 26.0f);
        g.setColour (collectionColour.withAlpha (0.12f));
        g.fillRoundedRectangle (badge, 7.0f);
        g.setColour (collectionColour.withAlpha (0.38f));
        g.drawRoundedRectangle (badge.reduced (0.5f), 7.0f, 0.8f);
        g.setColour (collectionColour);
        g.setFont (font (9.6f, FontWeight::semibold, 0.14f));
        g.drawText (collectionSlug (preset.collection), badge.toNearestInt(),
                    juce::Justification::centred, false);

        const auto groupLine = juce::String (groupSlug (preset.group)) + "  ·  "
                               + collectionSlug (preset.collection);
        g.setColour (textSecondary);
        g.setFont (font (9.8f, FontWeight::semibold, 0.14f));
        g.drawText (groupLine, 34, getHeight() - 150, 620, 14,
                    juce::Justification::centredLeft, false);

        g.setColour (textPrimary);
        g.setFont (font (50.0f, FontWeight::regular, -0.015f));
        g.drawFittedText (preset.name, 30, getHeight() - 210, 760, 60,
                          juce::Justification::centredLeft, 1, 0.72f);

        g.setColour (textPrimary.withAlpha (0.86f));
        g.setFont (font (14.2f, FontWeight::regular, 0.002f));
        g.drawFittedText (preset.description, 34, getHeight() - 130, 690, 40,
                          juce::Justification::topLeft, 2, 0.92f);

        float chipX = 34.0f;

        for (const auto* tag : preset.tags)
        {
            if (tag == nullptr || *tag == '\0')
                continue;

            const auto label = juce::String (tag).toUpperCase();
            const float width = juce::GlyphArrangement::getStringWidth (
                                    font (9.2f, FontWeight::semibold, 0.11f), label) + 17.0f;
            drawChip (g, { chipX, static_cast<float> (getHeight() - 56), width, 23.0f }, label, accent, false);
            chipX += width + 8.0f;
        }

        const std::array<juce::String, 3> recipes {
            oscillatorName (preset), shortFilterName (preset), motionName (preset)
        };

        float recipeX = getWidth() - 34.0f;

        for (int i = static_cast<int> (recipes.size()) - 1; i >= 0; --i)
        {
            const auto& text = recipes[static_cast<size_t> (i)];
            const float width = juce::GlyphArrangement::getStringWidth (
                                    font (9.0f, FontWeight::semibold, 0.09f), text) + 18.0f;
            recipeX -= width;
            drawChip (g, { recipeX, static_cast<float> (getHeight() - 56), width, 23.0f }, text, accent, true);
            recipeX -= 8.0f;
        }

        g.setColour (textMuted.withAlpha (0.9f));
        g.setFont (font (8.5f, FontWeight::regular, 0.10f));
        g.drawText (spaceName (preset).toUpperCase(), 34, getHeight() - 27, 470, 12,
                    juce::Justification::centredLeft, false);
    }

private:
    const Preset* current = nullptr;
    std::unique_ptr<ArrowButton> previousButton;
    std::unique_ptr<ArrowButton> nextButton;
    std::function<void()> onPrevious;
    std::function<void()> onNext;
};

HeroPanel::HeroPanel()
{
    setInterceptsMouseClicks (false, true);
    addAndMakeVisible (artwork);
    overlay = std::make_unique<Overlay>();
    addAndMakeVisible (*overlay);
}

HeroPanel::~HeroPanel() = default;

void HeroPanel::setPreset (const Preset& preset, bool animate)
{
    currentPreset = &preset;
    artwork.setSpec (preset.artwork, animate);

    if (overlay != nullptr)
        overlay->setPreset (preset);

    repaint();
}

void HeroPanel::setNavigationCallbacks (std::function<void()> previous,
                                        std::function<void()> next)
{
    if (overlay != nullptr)
        overlay->setCallbacks (std::move (previous), std::move (next));
}

void HeroPanel::setFrozen (bool frozen)
{
    artwork.setFrozen (frozen);
}

void HeroPanel::resized()
{
    artwork.setBounds (getLocalBounds());
    overlay->setBounds (getLocalBounds());
}
} // namespace synthset::ui
