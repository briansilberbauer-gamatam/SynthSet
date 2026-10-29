#pragma once

#include "SynthPatch.h"

#include <array>
#include <cstdint>
#include <type_traits>

namespace synthset
{
inline constexpr std::array<Group, kNumGroups> allGroups {
    Group::Bass, Group::Pads, Group::Keys, Group::Leads,
    Group::PlucksArps, Group::BrassStrings, Group::Textures, Group::Motion
};

constexpr std::uint32_t fnv1a (const char* text) noexcept
{
    std::uint32_t hash = 2166136261u;

    while (text != nullptr && *text != '\0')
    {
        hash ^= static_cast<std::uint32_t> (static_cast<unsigned char> (*text++));
        hash *= 16777619u;
    }

    return hash;
}

constexpr float seedFromId (const char* id) noexcept
{
    return static_cast<float> (fnv1a (id) & 0xffffffu) / 16777215.0f;
}

constexpr const char* groupDisplayName (Group group) noexcept
{
    switch (group)
    {
        case Group::Bass:         return "Bass";
        case Group::Pads:         return "Pads";
        case Group::Keys:         return "Keys";
        case Group::Leads:        return "Leads";
        case Group::PlucksArps:   return "Plucks & Arps";
        case Group::BrassStrings: return "Brass & Strings";
        case Group::Textures:     return "Textures & Atmos";
        case Group::Motion:       return "Motion & Rhythmic";
        case Group::Count:
        default:                  return "Unknown";
    }
}

constexpr const char* groupSlug (Group group) noexcept
{
    switch (group)
    {
        case Group::Bass:         return "BASS";
        case Group::Pads:         return "PADS";
        case Group::Keys:         return "KEYS";
        case Group::Leads:        return "LEADS";
        case Group::PlucksArps:   return "PLUCKS & ARPS";
        case Group::BrassStrings: return "BRASS & STRINGS";
        case Group::Textures:     return "TEXTURES & ATMOS";
        case Group::Motion:       return "MOTION & RHYTHMIC";
        case Group::Count:
        default:                  return "UNKNOWN";
    }
}

constexpr const char* collectionDisplayName (Collection collection) noexcept
{
    return collection == Collection::Classic ? "Classic" : "Modern";
}

constexpr const char* collectionSlug (Collection collection) noexcept
{
    return collection == Collection::Classic ? "CLASSIC" : "MODERN";
}

struct ArtworkSpec
{
    std::uint32_t top = 0xff05070c;
    std::uint32_t mid = 0xff0b1420;
    std::uint32_t bottom = 0xff02040a;
    std::uint32_t accent = 0xff7fd8ff;
    std::uint32_t accent2 = 0xffb38cff;
    Motif motif = Motif::Aurora;
    float seed = 0.0f;
    float energy = 0.5f;
    float haze = 0.5f;
};

struct Preset
{
    const char* id = "";
    const char* name = "";
    const char* lineage = "";
    const char* description = "";
    std::array<const char*, 4> tags { nullptr, nullptr, nullptr, nullptr };
    Group group = Group::Bass;
    Collection collection = Collection::Classic;
    ArtworkSpec artwork {};
    SynthPatch patch {};
};

static_assert (std::is_trivially_copyable_v<Preset>,
               "Preset metadata must be safe to read from immutable memory");

} // namespace synthset
