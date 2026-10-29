# SynthSet — Modern Liquid-Glass UI & Per-Preset Artwork Recipes

This reference defines the visual language and JUCE 8 drawing recipes for
SynthSet. The rule is simple: **one screen, no sound controls, and a different
image for every preset.** The interface should feel like a high-end instrument
viewing a living glass sculpture, not like a generic plugin with a preset menu.

Read this before writing `Source/ui/`, `PluginEditor.cpp`, or the screenshot
harness.

---

## 1. Visual Thesis

- **Mood**: nocturnal, refined, immersive, slightly cinematic.
- **Materials**: smoked glass over a deep blue-black light field, polished
  edges, caustic refraction, fine optical grain, restrained chromatic bloom.
- **Composition**: left library rail / large hero artwork / quiet status and
  detail typography. Generous negative space.
- **Motion**: slow art drift, a soft crossfade when presets change, small hover
  transitions. No bouncing, pulsing, or continuous full-window animation.
- **Product truth**: no knobs, faders, toggles, or edit affordances. The only
  interactive elements are collection filter, group rows, preset rows, and
  previous/next.

### What to avoid

- Gunmetal "hardware" panels, screws, brushed metal, or faux wood.
- Opaque black boxes with neon outlines.
- Flat glassmorphism with no depth or image-specific color.
- A single static background reused for all presets.
- Rainbow gradients everywhere. The accent is a highlight, not the whole UI.
- Drop shadows so heavy that the UI looks like floating stickers.
- Text over busy art without a local scrim.

---

## 2. Design Canvas and Layout

Fixed design canvas: **1280 × 800** (16:10). Resize from 960×600 to 1920×1200
with a single `AffineTransform`; never reflow individual controls per pixel.

```text
┌──────────────────────────────────────────────────────────────────────────────┐
│  outer 24 px inset                                                          │
│  ┌───────────────────────┐  ┌────────────────────────────────────────────┐  │
│  │ SYNTHSET              │  │ PADS · 2/6              ● MIDI   16 VOICES │  │
│  │                       │  │                              ‹     ›      │  │
│  │ [ ALL ][CLASSIC][MOD] │  │  ┌──────────────────────────────────────┐  │  │
│  │                       │  │  │                                      │  │  │
│  │  BASS              6  │  │  │        PER-PRESET HERO ARTWORK       │  │  │
│  │  PADS              6  │  │  │        (glass refraction + bloom)    │  │  │
│  │  KEYS              6  │  │  │                                      │  │  │
│  │  LEADS             6  │  │  │  106 Dream                           │  │  │
│  │  PLUCKS & ARPS     6  │  │  │  CLASSIC · 1980s six-voice inspired  │  │  │
│  │  BRASS & STRINGS   6  │  │  │  A slow, chorus-rich pad...          │  │  │
│  │  TEXTURES & ATMOS  6  │  │  │  [LUSH] [CHORUS] [WIDE]              │  │  │
│  │  MOTION & RHYTHMIC 6  │  │  │  2 OSC · SVF LP · BBD CHORUS         │  │  │
│  │                       │  │  └──────────────────────────────────────┘  │  │
│  │  PRESETS              │  │                                            │  │
│  │  106 Dream         ●  │  │  [read-only detail strip / footer]         │  │
│  │  Five Voice Halo      │  │                                            │  │
│  │  Sunset Pad           │  │                                            │  │
│  │  ...                  │  │                                            │  │
│  └───────────────────────┘  └────────────────────────────────────────────┘  │
│         left rail 300 px                 main stage 912 px                  │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 2.1 Bounds

```cpp
constexpr int designW = 1280;
constexpr int designH = 800;

auto outer = juce::Rectangle<int> (0, 0, designW, designH).reduced (24);
auto rail  = outer.removeFromLeft (300);
outer.removeFromLeft (20);
auto main  = outer;

auto status = main.removeFromTop (36);
main.removeFromTop (12);
auto hero = main.removeFromTop (540);
main.removeFromTop (12);
auto detail = main;             // remaining ~140 px
```

- Rail corner radius 26; hero radius 30; detail strip radius 22.
- Vertical rhythm: 12/16/24/32; avoid a different gap on every row.
- Keep a 24 px safe inset; no text touches a glass edge.
- Use a 2× render target for screenshots; scale the whole content component, not
  individual fonts.

### 2.2 Editor scaling

```cpp
// PluginEditor constructor
setResizable (true, true);
setResizeLimits (960, 600, 1920, 1200);
if (auto* c = getConstrainer())
    c->setFixedAspectRatio (1280.0 / 800.0);
setSize (1280, 800);

// PluginEditor::resized()
const float scale = juce::jmin ((float) getWidth()  / 1280.0f,
                                (float) getHeight() / 800.0f);
const float ox = ((float) getWidth()  - 1280.0f * scale) * 0.5f;
const float oy = ((float) getHeight() - 800.0f * scale) * 0.5f;
content.setBounds (0, 0, 1280, 800);
content.setTransform (juce::AffineTransform::scale (scale).translated (ox, oy));
```

`Content` owns all child components and paints in design coordinates. Do not
apply `AffineTransform` to child components individually.

---

## 3. Design Tokens (`Source/ui/Theme.h`)

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace synthset::ui
{
// Deep studio night / glass base
inline const juce::Colour baseDeep      { 0xff04060b };
inline const juce::Colour baseMid       { 0xff08111b };
inline const juce::Colour baseLift      { 0xff0c1724 };
inline const juce::Colour textPrimary   { 0xfff2f6fb };
inline const juce::Colour textSecondary { 0xffa9b6c7 };
inline const juce::Colour textMuted     { 0xff657386 };
inline const juce::Colour hairline      { 0x22ffffff };
inline const juce::Colour blackScrim    { 0xcc03060b };

// Default glass tint and accent; per-preset colors override the accent.
inline const juce::Colour glassTint     { 0x18d9ecff };
inline const juce::Colour glassEdge     { 0x38ffffff };
inline const juce::Colour glassShadow   { 0x66000000 };
inline const juce::Colour accentDefault { 0xff8ae8ff };

inline juce::Colour fromArgb (juce::uint32 argb) { return juce::Colour (argb); }

// Artwork palettes are per-preset; these are fallbacks.
inline juce::Colour accentForGroup (int /*group*/) { return accentDefault; }
}
```

For per-preset accents, read `preset.artwork.accent` and `accent2`; never hard-code
one accent in the rail or selected row.

### 3.1 Type

Bundle `LatoRegular.ttf`, `LatoSemibold.ttf`, `LatoBlack.ttf`, and
`Lato-OFL.txt` from `/home/brians/bls/sound/plugin_poc/Resources/` (or another
OFL family with the same weights). Do not rely on system fonts for screenshots
or CI.

```cpp
static juce::Font font (float height, int weight = 400, float kerning = 0.0f)
{
    // Cache typefaces once; do not create Typeface objects per paint.
    static auto regular = juce::Typeface::createSystemTypefaceFor (
        BinaryData::LatoRegular_ttf, BinaryData::LatoRegular_ttfSize);
    static auto semibold = juce::Typeface::createSystemTypefaceFor (
        BinaryData::LatoSemibold_ttf, BinaryData::LatoSemibold_ttfSize);
    static auto black = juce::Typeface::createSystemTypefaceFor (
        BinaryData::LatoBlack_ttf, BinaryData::LatoBlack_ttfSize);

    auto tf = weight >= 700 ? black : (weight >= 500 ? semibold : regular);
    auto f = juce::Font { juce::FontOptions (tf).withHeight (height) };
    f.setExtraKerningFactor (kerning);
    return f;
}
```

Typographic scale:

| Role | Size | Weight | Kerning | Case |
| :--- | ---: | ---: | ---: | :--- |
| Preset name | 48–54 | 300/Regular | 0 | Title |
| Group title / section | 12–13 | 600 | 0.16 | UPPER |
| Collection chips | 10–11 | 600 | 0.12 | UPPER |
| Preset row | 13.5 | 400/500 | 0.01 | Title |
| Description | 14–15 | 400 | 0.005 | Sentence |
| Tags / status | 9.5–10.5 | 500 | 0.10 | UPPER |
| Footer | 9.5 | 400 | 0.08 | UPPER |

- Numerals must be tabular for counters (`2/6`) if the font supports it; otherwise
  lay out the counter in a fixed-width area.
- Never set body text below 12.5 px at 1× design scale.
- Use title case for names, sentence case for descriptions, uppercase only for
  micro-labels.

---

## 4. Glass Rendering Recipes

### 4.1 Precompose the backdrop

Painting many translucent panels and radial blooms every frame is wasteful.
Build a cached backdrop image on resize:

1. Render the full artwork at 1/4 or 1/8 resolution.
2. Scale it up with bilinear interpolation to suggest a soft blur.
3. Draw a dark gradient scrim over it.
4. Draw the static glass panels, grain, and edge highlights once into a
   `juce::Image` cache.
5. Repaint only the animated hero and hover states at 30 fps.

```cpp
void paint (juce::Graphics& g) override
{
    if (backdrop.isValid())
        g.drawImageAt (backdrop, 0, 0);
    else
        g.fillAll (ui::baseDeep);

    // Animated hero on top; it owns its own cached images.
    hero.paintEntireComponent (g, false);
}
```

### 4.2 Glass panel helper

```cpp
void drawGlassPanel (juce::Graphics& g,
                     juce::Rectangle<float> r,
                     float radius,
                     juce::Colour accent,
                     float intensity = 1.0f)
{
    // 1. Very soft ambient shadow, not a sticker drop shadow
    g.setColour (juce::Colours::black.withAlpha (0.28f * intensity));
    g.fillRoundedRectangle (r.translated (0.0f, 10.0f).expanded (8.0f, 4.0f),
                            radius + 8.0f);

    // 2. Tinted smoked-glass body
    juce::ColourGradient body (
        juce::Colours::white.withAlpha (0.075f * intensity), r.getTopLeft(),
        juce::Colours::black.withAlpha (0.16f * intensity),  r.getBottomLeft(), false);
    body.addColour (0.55, accent.withAlpha (0.025f * intensity));
    g.setGradientFill (body);
    g.fillRoundedRectangle (r, radius);

    // 3. Accent refraction in the lower-right, very low alpha
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

    // 4. Polished rim: light top-left, dark bottom-right
    g.setColour (juce::Colours::white.withAlpha (0.20f * intensity));
    g.drawRoundedRectangle (r.reduced (0.75f), radius - 0.75f, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f * intensity));
    g.drawRoundedRectangle (r.reduced (0.25f), radius - 0.25f, 1.0f);

    // 5. Thin top-edge light pipe
    juce::Path topEdge;
    topEdge.startNewSubPath (r.getX() + radius, r.getY() + 1.0f);
    topEdge.lineTo (r.getRight() - radius, r.getY() + 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.28f * intensity));
    g.strokePath (topEdge, juce::PathStrokeType (1.0f));
}
```

### 4.3 Glass sheen

A single broad diagonal specular band, clipped to the panel, sells the material.
Keep it subtle; two competing highlights look like plastic.

```cpp
void drawGlassSheen (juce::Graphics& g,
                     juce::Rectangle<float> r,
                     float radius)
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

    juce::ColourGradient grad (
        juce::Colours::white.withAlpha (0.075f), r.getTopLeft(),
        juce::Colours::transparentWhite, r.getBottomLeft(), false);
    g.setGradientFill (grad);
    g.fillPath (sheen);
}
```

### 4.4 Optical grain

Generate one 128×128 ARGB tile once, then tile it. Do not generate noise per
paint.

```cpp
juce::Image makeGrainTile()
{
    juce::Image tile (juce::Image::ARGB, 128, 128, true);
    juce::Graphics g (tile);
    juce::Random rng (0x53594e54); // "SYNT"

    for (int y = 0; y < 128; ++y)
        for (int x = 0; x < 128; ++x)
        {
            const auto v = (juce::uint8) rng.nextInt (256);
            g.setColour (juce::Colours::white.withAlpha ((v / 255.0f) * 0.035f));
            g.fillRect (x, y, 1, 1);
        }
    return tile;
}
```

Draw with a low alpha and `Graphics::setTiledImageFill` or a single scaled image.
Do not tile with thousands of `fillRect` calls each frame.

---

## 5. Per-Preset Hero Artwork

### 5.1 `ArtworkSpec`

```cpp
struct ArtworkSpec
{
    juce::uint32 top, mid, bottom;
    juce::uint32 accent, accent2;
    Motif motif;
    float seed;
    float energy;   // 0..1 bloom/line intensity
    float haze;     // 0..1 diffusion
};
```

`seed` is derived from the stable preset ID (FNV-1a), so a preset always gets the
same composition. `energy`/`haze` differ between Classic and Modern. The group
palette comes from `preset-library.md`; per-preset accents may be hue-shifted
slightly for distinction.

### 5.2 Render pipeline

`ArtworkStage::renderArtwork (spec, width, height)`:

1. **Base gradient** from `top → mid → bottom`, diagonal from upper-left to
   lower-right.
2. **Primary bloom**: large radial gradient at a seeded position, accent alpha
   `0.22 * energy`.
3. **Secondary bloom**: smaller, complementary accent, opposite quadrant,
   alpha `0.14 * energy`.
4. **Haze bands**: 2–4 wide, blurred-looking ribbons using low-alpha thick
   strokes and transparent gradients.
5. **Motif**: one of the eight subject treatments below.
6. **Caustic lines**: 3–6 thin, semi-transparent arcs crossing the bloom.
7. **Vignette**: radial darkening toward the edges, alpha 0.35–0.65, to protect
   text.
8. **Grain**: tiled noise at 2–4% alpha.
9. **Color grade**: a translucent accent wash at 3–6% alpha over the whole stage.

Do not draw the preset name inside the artwork image. Text is a live UI layer so
it stays crisp at every scale and can receive a local scrim.

### 5.3 Seeded composition

```cpp
juce::Random rng ((juce::int64) (spec.seed * 1.0e9f) ^ 0x9e3779b9);

auto pointIn = [&] (float inset)
{
    return juce::Point<float> (
        inset + rng.nextFloat() * (width  - inset * 2.0f),
        inset + rng.nextFloat() * (height - inset * 2.0f));
};

const auto bloom1 = pointIn (width * 0.12f);
const auto bloom2 = pointIn (width * 0.18f);
const float r1 = width * (0.22f + 0.20f * rng.nextFloat());
```

Use `juce::Random` only on the message thread during image generation.

### 5.4 Motifs

Each motif must be recognizable but abstract:

| Motif | Visual treatment |
| :--- | :--- |
| **Aurora** | 3–5 horizontal wavy ribbons with soft vertical gradient, layered in depth. |
| **Prism** | 3–5 translucent triangular shards and one sharp refracted light beam. |
| **Bloom** | Concentric petal-like bezier curves opening from a bright core. |
| **Ribbon** | Two long sweeping cubic ribbons crossing behind the text zone. |
| **Caustic** | Overlapping elliptical rings with thin bright arcs, like light through water. |
| **Grid** | A subtle perspective grid with a few accent lines; keep the center clear for text. |
| **Tide** | Layered sine-wave bands, low and horizontal, with a dark upper area. |
| **Halo** | Large soft ring(s) around a diffuse core, strongest at the top third. |

### 5.5 Radial bloom helper

```cpp
void drawBloom (juce::Graphics& g,
                juce::Point<float> centre,
                float radius,
                juce::Colour colour,
                float alpha)
{
    juce::ColourGradient grad (
        colour.withAlpha (alpha), centre.x, centre.y,
        colour.withAlpha (0.0f), centre.x + radius, centre.y, true);
    g.setGradientFill (grad);
    g.fillEllipse (centre.x - radius, centre.y - radius,
                   radius * 2.0f, radius * 2.0f);
}
```

For a softer falloff, `ColourGradient::addColour` stops at 0.35 and 0.7:

```cpp
grad.addColour (0.35, colour.withAlpha (alpha * 0.55f));
grad.addColour (0.70, colour.withAlpha (alpha * 0.18f));
```

### 5.6 Ribbon path

```cpp
void drawRibbon (juce::Graphics& g,
                 juce::Point<float> a,
                 juce::Point<float> b,
                 juce::Point<float> c1,
                 juce::Point<float> c2,
                 juce::Colour colour,
                 float width,
                 float alpha)
{
    juce::Path p;
    p.startNewSubPath (a);
    p.cubicTo (c1, c2, b);

    for (int pass = 3; pass >= 1; --pass)
    {
        const float w = width * (1.0f + pass * 1.6f);
        const float a2 = alpha * (pass == 1 ? 1.0f : 0.16f / pass);
        g.setColour (colour.withAlpha (a2));
        g.strokePath (p, juce::PathStrokeType (w, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    }
}
```

### 5.7 Caching and crossfade

- Render to `juce::Image (ARGB, stageW * scale, stageH * scale, true)`.
- Redraw only on resize or preset change.
- Keep exactly two images: `current` and `outgoing`. Do not cache all 48.
- On preset change:
  ```cpp
  outgoing = current;
  current = renderArtwork (newSpec, w, h);
  fade = 0.0f;
  startTimerHz (30);
  ```
- In `paint`, draw the outgoing image at `1 - ease(fade)`, the current at
  `ease(fade)`, clipped to the hero's rounded rectangle. Use
  `juce::jlimit(0.0f, 1.0f, fade)` and a smoothstep:
  `ease = f*f*(3 - 2*f)`.
- Fade duration **350 ms**. After `fade >= 1`, drop the outgoing image and stop
  the timer if no other animation is active.
- Add a very slow drift: draw the current image scaled 1.015 and translated by
  `(sin(t*0.06)*4, cos(t*0.05)*3)`. The image itself is not regenerated.
- Repaint the stage at 30 fps only while visible; stop the timer in
  `visibilityChanged()` / when the editor closes.

### 5.8 Animation phase and determinism

For screenshots, set a fixed animation phase (e.g. `t = 0.5s`) so the PNG is
stable. For the live UI, use `juce::Time::getMillisecondCounterHiRes()` or a
`juce::Timer` counter. Do not use wall-clock time inside audio code.

---

## 6. Preset Rail

### 6.1 Structure

A custom `PresetRail` component, 300 px wide, containing:

1. **Wordmark**: `SYNTHSET` in semibold 14 px, tracked 0.22; a 6 px accent
   dot; subtitle `PRESET INSTRUMENT` in 8.5 px muted uppercase.
2. **Collection segmented control**: three text segments `ALL`, `CLASSIC`,
   `MODERN`. 36 px tall, glass track, sliding accent selection pill.
3. **Group rows**: eight 34 px rows. Left: group name, uppercase 10.5 px
   semibold. Right: count `6` or `3` depending on filter. Active row uses a
   1 px accent left bar, a soft accent wash, and `textPrimary`.
4. **Preset list**: rows for the active group and collection, 38 px tall.
   Selected row: accent wash, 2 px accent bar, title white. Hover: white 4%
   wash. Each row shows name and a small collection dot (`C`/`M` or warm/cool
   dot); no artwork thumbnails (keeps the rail calm and fast).
5. **Footer**: a tiny `48 PRESETS · 8 GROUPS` or version line, non-interactive.

No scrollbars are visible. If the filtered list exceeds the viewport, use a
`juce::Viewport` with hidden scrollbars and mouse-wheel support.

### 6.2 Interaction contract

- Clicking a group changes the visible preset list, keeps the current preset if
  it is in that group/filter, otherwise selects the first visible preset.
- Clicking `ALL/CLASSIC/MODERN` filters groups and preset rows. Group counts
  update. If the selected preset is filtered out, select the first visible one in
  the same group if possible.
- Clicking a preset selects it immediately.
- If the host or MIDI selects a preset outside the current group/collection
  filter, reveal it: switch to its collection (or `ALL`), select its group,
  scroll the row into view, and update the hero. Audio selection wins.
- Up/Down arrows move the preset selection within the visible list; Left/Right
  step previous/next globally in the filtered order.
- `Home`/`End` jump to the first/last visible preset.
- The editor takes keyboard focus; do not steal typing from the host without
  focus.
- Do not use a `ComboBox`; its popup menu breaks the visual language and makes
  the "one-screen" concept feel like a settings dialog.

### 6.3 Row painting

```cpp
void PresetRow::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (4.0f, 2.0f);

    if (isSelected)
    {
        g.setColour (accent.withAlpha (0.14f));
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (accent.withAlpha (0.9f));
        g.fillRoundedRectangle (r.withWidth (2.0f), 1.0f);
    }
    else if (isHover)
    {
        g.setColour (juce::Colours::white.withAlpha (0.04f));
        g.fillRoundedRectangle (r, 10.0f);
    }

    g.setColour (isSelected ? ui::textPrimary : ui::textSecondary);
    g.setFont (font (13.5f, isSelected ? 600 : 400, 0.01f));
    g.drawText (preset.name, r.withTrimmedLeft (14.0f).withTrimmedRight (26.0f),
                juce::Justification::centredLeft, true);

    // Collection dot, 4 px, warm for Classic, cool for Modern
    const auto dotColour = preset.collection == Collection::Classic
        ? juce::Colour (0xffffc46b) : accent;
    g.setColour (dotColour.withAlpha (isSelected ? 0.95f : 0.55f));
    g.fillEllipse (r.getRight() - 16.0f, r.getCentreY() - 2.0f, 4.0f, 4.0f);
}
```

Use `g.drawFittedText` or `juce::TextLayout` for names; never let a long name
clip mid-glyph. Set a maximum bank name length in the preset tests.

---

## 7. Hero Stage and Detail Panel

### 7.1 Hero

- 912 × 540, radius 30. The artwork fills the entire rect, clipped.
- Dark scrim: top 20% transparent, bottom 55% near-black; a second left-side
  gradient if the text starts at the left. Text must remain readable over any
  motif.
- Subject is abstract; leave the lower-left third quieter for text.
- A thin glass rim and top-left specular edge sit over the art.
- A small "CLASSIC" / "MODERN" pill at the top-left, 26 px high, 10 px tracked
  text, using the collection color (warm/cool) at low fill.
- The hero image must visibly change when the preset changes. The artwork is
  the primary feedback, not just the text.

### 7.2 Text block

Place the text block at the lower-left, with a 24–32 px inset. Compose it as:

```text
106 DREAM                                  [ CLASSIC ]
PADS · LUSH · CHORUS · WIDE
A slow, chorus-rich pad inspired by the classic 1980s six-voice sound.
[LUSH] [CHORUS] [WIDE]                     2 OSC · SVF LP · BBD CHORUS
```

- Preset name: 48–54 px Regular/Black depending on width. One line for names up
  to ~18 characters; use `drawFittedText` with a minimum horizontal scale of
  ~0.75 before reducing font size.
- Collection badge: 9.5 px uppercase, 0.14 tracking; fill with 18% of warm/cool
  color, border 1 px at 40%.
- Character line: 10 px uppercase, `textSecondary`, 0.12 tracking, max one
  line, built from the group and tags. Never render the internal lineage field.
- Description: 14.5 px, `textPrimary.withAlpha(0.88f)`, 2 lines max, line height
  1.35. Use a `juce::TextLayout` for wrapping; do not rely on `drawText` for
  multi-line paragraphs.
- Tags: 10 px uppercase chips, 22 px tall, glass fill, 1 px border, spaced 8 px.
- Recipe chips: read-only `2 OSC`, `SVF LP`, `BBD CHORUS`; differentiate from
  tags with an outline-only style and no fill. They are descriptive, not
  interactive.

If the description is too long for the available area, truncate with an ellipsis
at a word boundary in the preset tests (max 110 chars) and in the layout.

### 7.3 Previous/next

- Top-right of the main stage: two 34×34 glass circle/square buttons with 1 px
  rim and a subtle chevron. Accent on hover.
- Keyboard Left/Right also steps.
- Disable/wrap? Prefer wrapping within the filtered list so the arrows never
  dead-end. If the filtered list is empty (shouldn't happen), disable.
- Announce the selected index in the status strip (`3 / 6`) rather than on the
  buttons.

### 7.4 Status strip

At the top of the main stage:

```text
PADS / CLASSIC · 2 OF 3                         ● MIDI    16 VOICES
```

- 9.5–10 px uppercase, muted; live values in `textPrimary`.
- MIDI dot is a 5 px accent circle that brightens on recent MIDI activity and
  fades after ~250 ms.
- Voice count is read-only (`3 VOICES` when playing, `0 VOICES` when idle).
- No CPU meter unless there is room; if added, it is still read-only and must not
  imply a control.

---

## 8. Detail Strip / Read-Only Signal View

The bottom 140 px strip can show a read-only "recipe" of the preset:

- Three or four horizontal glass cells:
  1. `OSCILLATORS` — `2 OSC + SUB + NOISE`, mini waveform silhouettes.
  2. `FILTER` — `4-POLE LADDER · LP` or `2-POLE SVF · BP`.
  3. `MOTION` — `LFO 1 · 0.28 Hz` / `ARP 1/16` / `OFF`.
  4. `SPACE` — `BBD CHORUS · PLATE`.
- Each cell uses 9 px muted labels and 12 px values, with a tiny static icon or
  waveform. No draggable handles, no animated meters that look like controls.
- This is optional. If it crowds the hero, drop it and keep the hero larger. The
  preset bank is the product; the recipe is supporting information.

---

## 9. Performance and Rendering Discipline

- Cache the background and static glass in a `juce::Image` at the current
  display scale; rebuild on resize.
- Keep only current/outgoing artwork images. At 2×, a 912×540 stage is
  1824×1080×4 ≈ 7.9 MB per image; two images ≈ 16 MB, acceptable. Do not cache
  48.
- Avoid `juce::ImageEffectFilter` blurs per frame. Fake blur with a downscaled
  image and bilinear upscale, or layered gradients.
- Avoid per-paint `Typeface::createSystemTypefaceFor`; cache typefaces.
- Avoid `Path` construction for every grain speck; use a tile.
- Repaint at 30 fps only while the artwork is crossfading or drifting. When
  idle and not hovered, stop the timer.
- Editor animation must never run on the audio thread and must stop when the
  editor is destroyed.
- Call `setBufferedToImage(true)` only on static panels; not on the animated hero.
- Use `g.reduceClipRegion` with rounded paths; do not draw outside the glass and
  then mask with huge opaque rectangles.
- Test at 1×, 1.5×, and 2× scaling; text and 1 px rims must remain crisp.

---

## 10. Headless Screenshot Harness

`Tools/RenderEditor.cpp` should accept:

```bash
SynthSetScreenshot --preset <id> --out <path> [--scale 2] [--width 1280] [--height 800]
```

Implementation outline:

```cpp
juce::ScopedJuceInitialiser_GUI init;

auto processor = std::make_unique<SynthSetAudioProcessor>();
processor->prepareToPlay (48000.0, 512);

const int index = processor->getPresetManager().indexOfId (presetId);
if (index < 0) { std::cerr << "Unknown preset: " << presetId << "\n"; return 2; }
processor->setCurrentProgram (index);
processor->prepareToPlay (48000.0, 512); // apply deterministically

auto editor = std::unique_ptr<juce::AudioProcessorEditor> (processor->createEditor());
editor->setSize (1280, 800);

// Let the artwork generate and the fixed animation phase settle.
juce::MessageManager::getInstance()->runDispatchLoopUntil (120);

auto snapshot = editor->createComponentSnapshot (editor->getLocalBounds(), true, 2.0f);
juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile (outPath);
juce::PNGImageFormat png;
out.deleteFile();
if (auto stream = out.createOutputStream())
    png.writeImageToStream (snapshot, *stream);
```

Notes:
- For CI, wrap with `xvfb-run -a` on Linux.
- Force a deterministic animation phase for screenshots (e.g. freeze at
  `t = 0.5 s`) so diffs are stable.
- Do not feed audio in the screenshot harness unless the UI has a live meter
  that needs it. If it does, synthesize a short chord and freeze the meter.
- Render at 2× for review; save the PNG under `dist/` locally and `docs/` only
  when intentionally committed.

---

## 11. Visual QA Checklist

For every new preset or UI iteration, inspect at least three screenshots from
different groups/collections:

- [ ] Each hero image is visibly distinct in palette, motif, and composition.
- [ ] The preset name is legible over the art at 1× and 2×.
- [ ] The rail selected row, group, collection filter, and index all agree.
- [ ] No controls other than collection/group/preset/prev-next exist.
- [ ] No knob, slider, toggle, or editable field is visible.
- [ ] Glass panels show depth (rim, sheen, refraction) without looking muddy.
- [ ] Text contrast passes WCAG AA against its local scrim.
- [ ] No clipped names, wrapped tags, or overlapping hit zones.
- [ ] The hero crossfade is smooth and does not flash white/black.
- [ ] The UI does not repaint the entire editor continuously.
- [ ] At 960×600 the rail still fits or scrolls gracefully; at 1920×1200 nothing
      is stretched into blur.
- [ ] The screenshot has no debug rectangles, placeholder text, or host window
      chrome.

If any item fails, fix it before declaring the UI complete. A successful build
is not visual acceptance.

---

## 12. Screenshot Preset Matrix (Minimum)

At minimum, render and inspect:

| Preset | Why |
| :--- | :--- |
| `classic.pad.106-dream` | Warm classic pad, aurora motif, chorus glass. |
| `classic.keys.five-voice-keys` | Default preset; must be the strongest first impression. |
| `modern.pad.aurora-glass` | Cool modern pad, high energy, wide bloom. |
| `modern.bass.glass-sub` | Dark minimal art, large type, sub-bass restraint. |
| `modern.motion.kinetic-grid` | Grid motif, rhythmic tags, sharper accent. |

Use these five when changing shared layout or glass tokens; three is the minimum
for a preset-only change. Attach the best image to the final response with
`present` and a Markdown image tag.
