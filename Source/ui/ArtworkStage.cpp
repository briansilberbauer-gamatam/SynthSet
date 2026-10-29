#include "ArtworkStage.h"
#include "Theme.h"

#include <cmath>

namespace synthset::ui
{
namespace
{
constexpr double pi = 3.14159265358979323846;

void drawBloom (juce::Graphics& g, juce::Point<float> centre, float radius,
                juce::Colour colour, float alpha)
{
    juce::ColourGradient gradient (
        colour.withAlpha (alpha), centre.x, centre.y,
        colour.withAlpha (0.0f), centre.x + radius, centre.y, true);
    gradient.addColour (0.35, colour.withAlpha (alpha * 0.55f));
    gradient.addColour (0.70, colour.withAlpha (alpha * 0.18f));
    g.setGradientFill (gradient);
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
}

void drawRibbon (juce::Graphics& g, juce::Point<float> a, juce::Point<float> b,
                 juce::Point<float> c1, juce::Point<float> c2,
                 juce::Colour colour, float width, float alpha)
{
    juce::Path path;
    path.startNewSubPath (a);
    path.cubicTo (c1, c2, b);

    for (int pass = 3; pass >= 1; --pass)
    {
        const float strokeWidth = width * (1.0f + static_cast<float> (pass) * 1.7f);
        const float passAlpha = alpha * (pass == 1 ? 1.0f : 0.16f / static_cast<float> (pass));
        g.setColour (colour.withAlpha (passAlpha));
        g.strokePath (path, juce::PathStrokeType (strokeWidth, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }
}

void drawCausticRing (juce::Graphics& g, juce::Point<float> centre, float rx, float ry,
                      float rotation, juce::Colour colour, float alpha)
{
    juce::Graphics::ScopedSaveState state (g);
    g.addTransform (juce::AffineTransform::rotation (rotation, centre.x, centre.y));
    g.setColour (colour.withAlpha (alpha));
    g.drawEllipse (centre.x - rx, centre.y - ry, rx * 2.0f, ry * 2.0f, 1.15f);
    g.setColour (colour.withAlpha (alpha * 0.45f));
    g.drawEllipse (centre.x - rx * 0.82f, centre.y - ry * 0.82f,
                   rx * 1.64f, ry * 1.64f, 0.7f);
}
} // namespace

ArtworkStage::ArtworkStage()
{
    setInterceptsMouseClicks (false, false);
    setOpaque (false);
}

void ArtworkStage::setSpec (const ArtworkSpec& newSpec, bool animate)
{
    if (haveSpec && std::abs (newSpec.seed - spec.seed) < 1.0e-9f)
    {
        spec = newSpec;
        return;
    }

    if (currentImage.isValid() && animate)
        outgoingImage = currentImage;
    else
        outgoingImage = {};

    spec = newSpec;
    haveSpec = true;
    fade = outgoingImage.isValid() ? 0.0f : 1.0f;
    currentImage = {};

    if (getWidth() > 0 && getHeight() > 0)
        rebuildImages();

    if (isVisible())
        startTimerHz (30);

    repaint();
}

void ArtworkStage::setFrozen (bool shouldFreeze)
{
    frozen = shouldFreeze;
    time = 0.5f;

    if (frozen)
        stopTimer();
    else if (isVisible())
        startTimerHz (30);
}

void ArtworkStage::resized()
{
    rebuildImages();
}

void ArtworkStage::visibilityChanged()
{
    if (isVisible())
        startTimerHz (30);
    else
        stopTimer();
}

void ArtworkStage::parentHierarchyChanged()
{
    if (isShowing())
        startTimerHz (30);
    else
        stopTimer();
}

void ArtworkStage::rebuildImages()
{
    if (! haveSpec || getWidth() <= 0 || getHeight() <= 0)
        return;

    const int renderWidth = std::max (1, getWidth() * 2);
    const int renderHeight = std::max (1, getHeight() * 2);
    currentImage = renderArtwork (spec, renderWidth, renderHeight);

    if (! frozen)
        startTimerHz (30);
}

void ArtworkStage::timerCallback()
{
    if (frozen)
        return;

    time += 1.0f / 30.0f;

    if (fade < 1.0f)
    {
        fade = std::min (1.0f, fade + (1.0f / 30.0f) / 0.35f);

        if (fade >= 1.0f)
            outgoingImage = {};
    }

    repaint();
}

void ArtworkStage::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    if (bounds.isEmpty())
        return;

    juce::Graphics::ScopedSaveState state (g);
    juce::Path clip;
    clip.addRoundedRectangle (bounds, 30.0f);
    g.reduceClipRegion (clip);

    const float smooth = fade * fade * (3.0f - 2.0f * fade);
    const float driftX = std::sin (time * 0.064f) * 4.0f;
    const float driftY = std::cos (time * 0.052f) * 3.0f;

    auto paintImage = [&] (const juce::Image& image, float opacity)
    {
        if (! image.isValid() || opacity <= 0.001f)
            return;

        const float scaleX = bounds.getWidth() / static_cast<float> (image.getWidth());
        const float scaleY = bounds.getHeight() / static_cast<float> (image.getHeight());
        const auto centre = bounds.getCentre();

        const auto transform = juce::AffineTransform::scale (scaleX, scaleY)
                                   .translated (centre.x, centre.y)
                                   .scaled (1.012f, 1.012f)
                                   .translated (-centre.x, -centre.y)
                                   .translated (driftX, driftY);

        g.setOpacity (opacity);
        g.drawImageTransformed (image, transform);
    };

    paintImage (outgoingImage, 1.0f - smooth);
    paintImage (currentImage, smooth);
    g.setOpacity (1.0f);

    // Polished glass rim.
    g.setColour (juce::Colours::white.withAlpha (0.17f));
    g.drawRoundedRectangle (bounds.reduced (0.75f), 29.5f, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.30f));
    g.drawRoundedRectangle (bounds.reduced (0.2f), 29.9f, 0.8f);
}

juce::Image ArtworkStage::renderArtwork (const ArtworkSpec& art, int width, int height) const
{
    juce::Image image (juce::Image::ARGB, width, height, true);

    if (width <= 0 || height <= 0)
        return image;

    juce::Graphics g (image);
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.addTransform (juce::AffineTransform::scale (static_cast<float> (width) / static_cast<float> (getWidth()),
                                                   static_cast<float> (height) / static_cast<float> (getHeight())));

    const auto w = static_cast<float> (getWidth() > 0 ? getWidth() : 912);
    const auto h = static_cast<float> (getHeight() > 0 ? getHeight() : 540);

    const auto accent = colourFrom (art.accent);
    const auto accent2 = colourFrom (art.accent2);
    const float variance = art.seed * 971.0f - std::floor (art.seed * 971.0f);

    // Every preset gets a small palette tilt away from its group base while the
    // family resemblance remains intact.
    const auto top = colourFrom (art.top).interpolatedWith (accent, 0.055f + 0.055f * variance);
    const auto mid = colourFrom (art.mid).interpolatedWith (accent2, 0.075f + 0.075f * (1.0f - variance));
    const auto bottom = colourFrom (art.bottom).interpolatedWith (accent, 0.025f);

    juce::ColourGradient background (top, 0.0f, 0.0f, bottom, w, h, false);
    background.addColour (0.52, mid);
    g.setGradientFill (background);
    g.fillRect (juce::Rectangle<float> (0.0f, 0.0f, w, h));

    juce::Random random (static_cast<juce::int64> (art.seed * 1.0e9f) ^ 0x9e3779b9);

    const auto bloom1 = juce::Point<float> (w * (0.42f + 0.45f * random.nextFloat()),
                                             h * (0.12f + 0.34f * random.nextFloat()));
    const auto bloom2 = juce::Point<float> (w * (0.12f + 0.48f * random.nextFloat()),
                                             h * (0.12f + 0.32f * random.nextFloat()));
    const float radius1 = w * (0.20f + 0.18f * random.nextFloat());

    drawBloom (g, bloom1, radius1, accent, 0.31f * art.energy);
    drawBloom (g, bloom2, radius1 * 0.78f, accent2, 0.20f * art.energy);
    drawBloom (g, { w * 0.83f, h * 0.16f }, w * 0.28f, accent2, 0.12f * art.energy);

    // Haze bands.
    for (int i = 0; i < 4; ++i)
    {
        const float y = h * (0.12f + 0.62f * random.nextFloat());
        const float amplitude = h * (0.03f + 0.09f * random.nextFloat());
        juce::Path haze;
        haze.startNewSubPath (-w * 0.05f, y);
        haze.cubicTo (w * 0.25f, y - amplitude, w * 0.62f, y + amplitude, w * 1.05f,
                      y + amplitude * 0.2f);
        g.setColour ((i % 2 == 0 ? accent : accent2).withAlpha (0.045f + 0.035f * art.haze));
        g.strokePath (haze, juce::PathStrokeType (24.0f + 34.0f * random.nextFloat(),
                                                  juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }

    const auto centre = juce::Point<float> (w * 0.62f, h * 0.30f);
    const float energy = art.energy;

    {
    juce::Graphics::ScopedSaveState motifState (g);
    g.addTransform (juce::AffineTransform::rotation ((variance - 0.5f) * 0.16f,
                                                     w * 0.61f, h * 0.32f));

    switch (art.motif)
    {
        case Motif::Aurora:
        {
            for (int i = 0; i < 5; ++i)
            {
                const float y = h * (0.18f + 0.07f * static_cast<float> (i));
                const float amp = h * (0.045f + 0.02f * static_cast<float> (i));
                juce::Path path;
                path.startNewSubPath (-w * 0.05f, y);
                path.cubicTo (w * 0.22f, y - amp, w * 0.35f, y + amp, w * 0.55f, y + amp * 0.3f);
                path.cubicTo (w * 0.74f, y - amp * 1.2f, w * 0.87f, y + amp * 0.8f,
                              w * 1.05f, y + amp * 0.1f);
                g.setColour (accent.withAlpha (0.09f * energy));
                g.strokePath (path, juce::PathStrokeType (46.0f, juce::PathStrokeType::curved,
                                                           juce::PathStrokeType::rounded));
                g.setColour ((i % 2 == 0 ? accent : accent2).withAlpha (0.24f * energy));
                g.strokePath (path, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved,
                                                           juce::PathStrokeType::rounded));
            }
            break;
        }

        case Motif::Prism:
        {
            for (int i = 0; i < 5; ++i)
            {
                const float x = w * (0.42f + 0.42f * random.nextFloat());
                const float y = h * (0.10f + 0.36f * random.nextFloat());
                const float size = w * (0.07f + 0.09f * random.nextFloat());
                juce::Path shard;
                shard.startNewSubPath (x, y - size);
                shard.lineTo (x + size * 0.8f, y + size * 0.45f);
                shard.lineTo (x - size * 0.7f, y + size * 0.2f);
                shard.closeSubPath();
                g.setColour ((i % 2 == 0 ? accent : accent2).withAlpha (0.08f * energy));
                g.fillPath (shard);
                g.setColour (accent2.withAlpha (0.35f * energy));
                g.strokePath (shard, juce::PathStrokeType (1.0f));
            }

            juce::Path beam;
            beam.startNewSubPath (w * 0.38f, h * 0.06f);
            beam.lineTo (w * 0.54f, h * 0.06f);
            beam.lineTo (w * 0.78f, h * 0.68f);
            beam.lineTo (w * 0.64f, h * 0.68f);
            beam.closeSubPath();
            juce::ColourGradient beamGradient (accent.withAlpha (0.15f), w * 0.46f, h * 0.05f,
                                                accent2.withAlpha (0.0f), w * 0.72f, h * 0.68f, false);
            g.setGradientFill (beamGradient);
            g.fillPath (beam);
            break;
        }

        case Motif::Bloom:
        {
            const int petals = 11;

            for (int i = 0; i < petals; ++i)
            {
                const float angle = static_cast<float> (i) * juce::MathConstants<float>::twoPi
                                    / static_cast<float> (petals) + art.seed * 2.0f;
                const float length = w * (0.10f + 0.10f * (0.5f + 0.5f * std::sin (angle * 3.0f)));
                juce::Graphics::ScopedSaveState state (g);
                g.addTransform (juce::AffineTransform::rotation (angle, centre.x, centre.y));
                juce::Path petal;
                petal.startNewSubPath (centre.x, centre.y);
                petal.cubicTo (centre.x - length * 0.25f, centre.y - length * 0.45f,
                               centre.x + length * 0.30f, centre.y - length * 1.0f,
                               centre.x, centre.y - length * 1.35f);
                petal.cubicTo (centre.x - length * 0.55f, centre.y - length * 0.85f,
                               centre.x - length * 0.35f, centre.y - length * 0.25f,
                               centre.x, centre.y);
                g.setColour ((i % 2 == 0 ? accent : accent2).withAlpha (0.055f * energy));
                g.fillPath (petal);
                g.setColour (accent.withAlpha (0.18f * energy));
                g.strokePath (petal, juce::PathStrokeType (0.8f));
            }
            drawBloom (g, centre, w * 0.12f, accent2, 0.20f * energy);
            break;
        }

        case Motif::Ribbon:
        {
            drawRibbon (g, { -w * 0.05f, h * 0.60f }, { w * 1.05f, h * 0.20f },
                        { w * 0.26f, h * 0.10f }, { w * 0.58f, h * 0.90f },
                        accent, 26.0f, 0.26f * energy);
            drawRibbon (g, { -w * 0.04f, h * 0.28f }, { w * 1.05f, h * 0.54f },
                        { w * 0.30f, h * 0.72f }, { w * 0.68f, h * 0.06f },
                        accent2, 14.0f, 0.22f * energy);
            break;
        }

        case Motif::Caustic:
        {
            for (int i = 0; i < 6; ++i)
            {
                const auto ringCentre = juce::Point<float> (w * (0.35f + 0.4f * random.nextFloat()),
                                                            h * (0.18f + 0.30f * random.nextFloat()));
                drawCausticRing (g, ringCentre, w * (0.06f + 0.09f * random.nextFloat()),
                                 w * (0.03f + 0.05f * random.nextFloat()),
                                 random.nextFloat() * 3.0f, accent, 0.22f * energy);
            }

            for (int i = 0; i < 4; ++i)
            {
                const auto arcCentre = juce::Point<float> (w * (0.3f + 0.45f * random.nextFloat()),
                                                           h * (0.2f + 0.36f * random.nextFloat()));
                g.setColour (accent2.withAlpha (0.24f * energy));
                g.drawEllipse (arcCentre.x - w * 0.14f, arcCentre.y - h * 0.11f,
                               w * 0.28f, h * 0.22f, 0.9f);
            }
            break;
        }

        case Motif::Grid:
        {
            const auto vanishing = juce::Point<float> (w * 0.67f, h * 0.34f);
            g.setColour (accent.withAlpha (0.10f * energy));

            for (int i = 0; i < 13; ++i)
            {
                const float x = w * (0.20f + 0.06f * static_cast<float> (i));
                g.drawLine (vanishing.x, vanishing.y, x, h * 0.74f, 0.7f);
            }

            for (int i = 0; i < 7; ++i)
            {
                const float t = static_cast<float> (i) / 7.0f;
                const float y = vanishing.y + (h * 0.74f - vanishing.y) * (t * t);
                g.drawLine (w * 0.18f, y, w * 0.90f, y, 0.7f);
            }

            for (int i = 0; i < 3; ++i)
            {
                const float y = h * (0.18f + 0.13f * static_cast<float> (i));
                g.setColour (accent2.withAlpha (0.28f * energy));
                g.drawLine (w * 0.18f, y, w * 0.88f, y, 1.2f);
            }
            break;
        }

        case Motif::Tide:
        {
            for (int band = 0; band < 5; ++band)
            {
                juce::Path wave;
                const float baseY = h * (0.55f + 0.065f * static_cast<float> (band));
                wave.startNewSubPath (-w * 0.05f, baseY);

                for (int segment = 0; segment < 4; ++segment)
                {
                    const float x0 = w * (-0.05f + 0.27f * static_cast<float> (segment));
                    const float x1 = w * (-0.05f + 0.27f * static_cast<float> (segment + 1));
                    const float amp = h * (0.025f + 0.012f * static_cast<float> (band));
                    wave.cubicTo (x0 + w * 0.11f, baseY - amp, x1 - w * 0.11f, baseY + amp,
                                  x1, baseY + amp * 0.15f);
                }

                wave.lineTo (w * 1.05f, h * 1.05f);
                wave.lineTo (-w * 0.05f, h * 1.05f);
                wave.closeSubPath();
                g.setGradientFill (juce::ColourGradient (
                    (band % 2 == 0 ? accent : accent2).withAlpha (0.07f * energy), 0.0f, baseY,
                    juce::Colours::transparentBlack, 0.0f, h, false));
                g.fillPath (wave);
                g.setColour (accent.withAlpha (0.16f * energy));
                g.strokePath (wave, juce::PathStrokeType (0.8f));
            }
            break;
        }

        case Motif::Halo:
        {
            for (int i = 0; i < 4; ++i)
            {
                const float radius = w * (0.11f + 0.095f * static_cast<float> (i));
                g.setColour ((i % 2 == 0 ? accent : accent2).withAlpha (0.16f * energy));
                g.drawEllipse (centre.x - radius, centre.y - radius * 0.72f,
                               radius * 2.0f, radius * 1.44f, 0.9f);
            }

            drawBloom (g, { centre.x, centre.y - h * 0.06f }, w * 0.10f, accent2, 0.22f * energy);
            break;
        }
    }
    }

    // A few fine caustic arcs.
    for (int i = 0; i < 4; ++i)
    {
        const auto c = juce::Point<float> (w * (0.30f + 0.55f * random.nextFloat()),
                                            h * (0.05f + 0.5f * random.nextFloat()));
        g.setColour (accent.withAlpha (0.18f * energy));
        g.drawEllipse (c.x - w * 0.14f, c.y - h * 0.08f, w * 0.28f, h * 0.16f, 0.6f);
    }

    // Vignette keeps text legible and gives the glass stage depth.
    juce::ColourGradient vignette (juce::Colours::transparentBlack, centre.x, centre.y,
                                    juce::Colours::black.withAlpha (0.58f), 0.0f, 0.0f, true);
    vignette.addColour (0.55, juce::Colours::black.withAlpha (0.10f));
    g.setGradientFill (vignette);
    g.fillRect (juce::Rectangle<float> (0.0f, 0.0f, w, h));

    // Soft bottom scrim for the preset typography.
    juce::ColourGradient scrim (juce::Colours::transparentBlack, 0.0f, h * 0.48f,
                                 juce::Colours::black.withAlpha (0.64f), 0.0f, h, false);
    g.setGradientFill (scrim);
    g.fillRect (juce::Rectangle<float> (0.0f, h * 0.42f, w, h * 0.58f));

    drawGrain (g, juce::Rectangle<float> (0.0f, 0.0f, w, h), 0.55f);

    g.setColour (accent.withAlpha (0.045f));
    g.fillRect (juce::Rectangle<float> (0.0f, 0.0f, w, h));

    return image;
}
} // namespace synthset::ui
