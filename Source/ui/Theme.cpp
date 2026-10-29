#include "Theme.h"

#include <BinaryData.h>

#include <cmath>

namespace synthset::ui
{
namespace
{
juce::Typeface::Ptr makeTypeface (const void* data, int size)
{
    return juce::Typeface::createSystemTypefaceFor (data, static_cast<size_t> (size));
}

const juce::Typeface::Ptr& regularTypeface()
{
    static const auto typeface = makeTypeface (BinaryData::LatoRegular_ttf,
                                                BinaryData::LatoRegular_ttfSize);
    return typeface;
}

const juce::Typeface::Ptr& semiboldTypeface()
{
    static const auto typeface = makeTypeface (BinaryData::LatoSemibold_ttf,
                                                BinaryData::LatoSemibold_ttfSize);
    return typeface;
}

const juce::Typeface::Ptr& blackTypeface()
{
    static const auto typeface = makeTypeface (BinaryData::LatoBlack_ttf,
                                                BinaryData::LatoBlack_ttfSize);
    return typeface;
}

const juce::Image& grainTile()
{
    static const juce::Image tile = []
    {
        juce::Image image (juce::Image::ARGB, 128, 128, true);
        juce::Graphics g (image);
        juce::Random random (0x53594e54);

        for (int y = 0; y < 128; ++y)
        {
            for (int x = 0; x < 128; ++x)
            {
                const auto value = static_cast<juce::uint8> (random.nextInt (256));
                g.setColour (juce::Colours::white.withAlpha (static_cast<float> (value) / 255.0f * 0.12f));
                g.fillRect (x, y, 1, 1);
            }
        }

        return image;
    }();

    return tile;
}
} // namespace

juce::Font font (float height, FontWeight weight, float tracking)
{
    const auto& typeface = weight == FontWeight::black ? blackTypeface()
                         : weight == FontWeight::semibold ? semiboldTypeface()
                                                          : regularTypeface();

    juce::Font result { juce::FontOptions (typeface).withHeight (height) };

    if (std::abs (tracking) > 1.0e-5f)
        result = result.withExtraKerningFactor (tracking);

    return result;
}

void drawGlassPanel (juce::Graphics& g, juce::Rectangle<float> r, float radius,
                     juce::Colour accent, float intensity)
{
    g.setColour (juce::Colours::black.withAlpha (0.28f * intensity));
    g.fillRoundedRectangle (r.translated (0.0f, 10.0f).expanded (8.0f, 4.0f),
                            radius + 8.0f);

    juce::ColourGradient body (
        juce::Colours::white.withAlpha (0.075f * intensity), r.getTopLeft(),
        juce::Colours::black.withAlpha (0.16f * intensity), r.getBottomLeft(), false);
    body.addColour (0.55, accent.withAlpha (0.025f * intensity));
    g.setGradientFill (body);
    g.fillRoundedRectangle (r, radius);

    {
        juce::Graphics::ScopedSaveState state (g);
        juce::Path clip;
        clip.addRoundedRectangle (r, radius);
        g.reduceClipRegion (clip);

        juce::ColourGradient bloom (
            accent.withAlpha (0.10f * intensity), r.getBottomRight(),
            juce::Colours::transparentBlack, r.getCentre(), true);
        g.setGradientFill (bloom);
        g.fillRoundedRectangle (r, radius);
    }

    g.setColour (juce::Colours::white.withAlpha (0.20f * intensity));
    g.drawRoundedRectangle (r.reduced (0.75f), radius - 0.75f, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f * intensity));
    g.drawRoundedRectangle (r.reduced (0.25f), radius - 0.25f, 1.0f);

    juce::Path topEdge;
    topEdge.startNewSubPath (r.getX() + radius, r.getY() + 1.0f);
    topEdge.lineTo (r.getRight() - radius, r.getY() + 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.28f * intensity));
    g.strokePath (topEdge, juce::PathStrokeType (1.0f));
}

void drawGlassSheen (juce::Graphics& g, juce::Rectangle<float> r, float radius)
{
    juce::Graphics::ScopedSaveState state (g);
    juce::Path clip;
    clip.addRoundedRectangle (r, radius);
    g.reduceClipRegion (clip);

    juce::Path sheen;
    sheen.startNewSubPath (r.getX(), r.getY() + r.getHeight() * 0.08f);
    sheen.lineTo (r.getX() + r.getWidth() * 0.46f, r.getY());
    sheen.lineTo (r.getX() + r.getWidth() * 0.20f, r.getY() + r.getHeight());
    sheen.lineTo (r.getX(), r.getY() + r.getHeight() * 0.72f);
    sheen.closeSubPath();

    juce::ColourGradient gradient (
        juce::Colours::white.withAlpha (0.075f), r.getTopLeft(),
        juce::Colours::transparentWhite, r.getBottomLeft(), false);
    g.setGradientFill (gradient);
    g.fillPath (sheen);
}

void drawGrain (juce::Graphics& g, juce::Rectangle<float> bounds, float opacity)
{
    g.setTiledImageFill (grainTile(), 0, 0, opacity);
    g.fillRect (bounds);
}

void drawHairline (juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour colour)
{
    g.setColour (colour);
    g.fillRect (bounds);
}

void drawChevron (juce::Graphics& g, juce::Rectangle<float> bounds, bool pointsRight,
                  juce::Colour colour, float thickness)
{
    const auto c = bounds.getCentre();
    const float direction = pointsRight ? 1.0f : -1.0f;
    const float halfWidth = bounds.getWidth() * 0.15f;
    const float halfHeight = bounds.getHeight() * 0.18f;

    juce::Path path;
    path.startNewSubPath (c.x - halfWidth * direction, c.y - halfHeight);
    path.lineTo (c.x + halfWidth * direction, c.y);
    path.lineTo (c.x - halfWidth * direction, c.y + halfHeight);
    g.setColour (colour);
    g.strokePath (path, juce::PathStrokeType (thickness, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
}

juce::String paddedIndex (int index, int total)
{
    juce::ignoreUnused (total);
    return juce::String (index + 1).paddedLeft ('0', 2);
}
} // namespace synthset::ui
