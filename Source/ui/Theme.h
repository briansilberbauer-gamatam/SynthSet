#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace synthset::ui
{
inline const juce::Colour baseDeep      { 0xff04060b };
inline const juce::Colour baseMid       { 0xff08111b };
inline const juce::Colour baseLift      { 0xff0c1724 };
inline const juce::Colour textPrimary   { 0xfff2f6fb };
inline const juce::Colour textSecondary { 0xffa9b6c7 };
inline const juce::Colour textMuted     { 0xff7c8a9d };
inline const juce::Colour hairline      { 0x22ffffff };
inline const juce::Colour blackScrim    { 0xcc03060b };

inline const juce::Colour glassTint     { 0x18d9ecff };
inline const juce::Colour glassEdge     { 0x38ffffff };
inline const juce::Colour glassShadow   { 0x66000000 };
inline const juce::Colour accentDefault { 0xff8ae8ff };

enum class FontWeight { regular, semibold, black };

juce::Font font (float height, FontWeight weight = FontWeight::regular, float tracking = 0.0f);

inline juce::Colour colourFrom (juce::uint32 argb)
{
    return juce::Colour (argb);
}

inline juce::Colour accentForSpec (juce::uint32 accent)
{
    return juce::Colour (accent);
}

void drawGlassPanel (juce::Graphics&, juce::Rectangle<float> bounds, float radius,
                     juce::Colour accent, float intensity = 1.0f);

void drawGlassSheen (juce::Graphics&, juce::Rectangle<float> bounds, float radius);

void drawGrain (juce::Graphics&, juce::Rectangle<float> bounds, float opacity = 0.35f);

void drawHairline (juce::Graphics&, juce::Rectangle<float> bounds, juce::Colour colour);

void drawChevron (juce::Graphics&, juce::Rectangle<float> bounds, bool pointsRight,
                  juce::Colour colour, float thickness = 1.4f);

juce::String paddedIndex (int index, int total);
} // namespace synthset::ui
