#include "Wavetable.h"

#include <algorithm>
#include <cmath>

namespace synthset
{
namespace
{
constexpr double twoPi = 6.28318530717958647692;

float harmonicWeight (int frame, int harmonic)
{
    const double h = static_cast<double> (harmonic);

    switch (frame)
    {
        case 0:
            return static_cast<float> ((harmonic % 2 == 0 ? 0.78 : 1.0)
                                       / std::pow (h, 1.62));
        case 1:
            return harmonic % 2 == 1
                       ? static_cast<float> (1.0 / std::pow (h, 1.25))
                       : 0.0f;
        case 2:
            return static_cast<float> (1.0 / std::pow (h, 1.05));
        case 3:
        {
            const double a = std::exp (-0.5 * std::pow ((h - 7.0) / 2.4, 2.0));
            const double b = std::exp (-0.5 * std::pow ((h - 17.0) / 3.4, 2.0));
            return static_cast<float> ((a + 0.65 * b) / std::pow (h, 0.35));
        }
        default:
            return 0.0f;
    }
}
} // namespace

void WavetableBank::prepare() noexcept
{
    if (prepared)
        return;

    for (int frame = 0; frame < numFrames; ++frame)
    {
        for (int mip = 0; mip < numMips; ++mip)
        {
            auto& table = tables[static_cast<size_t> (frame)][static_cast<size_t> (mip)];
            const int cap = harmonicCaps[static_cast<size_t> (mip)];

            for (int i = 0; i < tableSize; ++i)
            {
                const double phase = static_cast<double> (i) / static_cast<double> (tableSize);
                double value = 0.0;

                for (int h = 1; h <= cap; ++h)
                {
                    const float weight = harmonicWeight (frame, h);

                    if (weight != 0.0f)
                        value += weight * std::sin (twoPi * phase * h);
                }

                table[static_cast<size_t> (i)] = static_cast<float> (value);
            }

            float peak = 0.0f;

            for (const auto sample : table)
                peak = std::max (peak, std::abs (sample));

            if (peak > 1.0e-6f)
                for (auto& sample : table)
                    sample *= 0.9f / peak;
        }
    }

    prepared = true;
}

float WavetableBank::sample (float position, float phase, float frequency,
                             double sampleRate) const noexcept
{
    if (! prepared)
        return 0.0f;

    position = std::clamp (position, 0.0f, 1.0f);
    phase = phase - std::floor (phase);

    const float framePosition = position * static_cast<float> (numFrames - 1);
    const int frameA = std::min (numFrames - 1, static_cast<int> (framePosition));
    const int frameB = std::min (numFrames - 1, frameA + 1);
    const float frameMix = framePosition - static_cast<float> (frameA);

    const double allowedHarmonics = std::max (1.0, 0.45 * sampleRate
                                                       / std::max (1.0f, frequency));
    int mip = numMips - 1;

    for (int level = 0; level < numMips; ++level)
    {
        if (static_cast<double> (harmonicCaps[static_cast<size_t> (level)]) <= allowedHarmonics)
        {
            mip = level;
            break;
        }
    }

    const float tablePosition = phase * static_cast<float> (tableSize);
    int index0 = static_cast<int> (tablePosition);
    if (index0 >= tableSize)
        index0 = 0;
    const int index1 = (index0 + 1) & (tableSize - 1);
    const float fraction = tablePosition - static_cast<float> (index0);

    auto readFrame = [&] (int frame)
    {
        const auto& table = tables[static_cast<size_t> (frame)][static_cast<size_t> (mip)];
        const float a = table[static_cast<size_t> (index0)];
        const float b = table[static_cast<size_t> (index1)];
        return a + (b - a) * fraction;
    };

    const float a = readFrame (frameA);
    const float b = readFrame (frameB);
    return a + (b - a) * frameMix;
}

const WavetableBank& getSharedWavetableBank() noexcept
{
    static const WavetableBank bank = []
    {
        WavetableBank b;
        b.prepare();
        return b;
    }();

    return bank;
}
} // namespace synthset
