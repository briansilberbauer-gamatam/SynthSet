#include "Filters.h"

namespace synthset
{
namespace
{
constexpr double pi = 3.14159265358979323846;

double clampCutoff (double cutoff, double sampleRate) noexcept
{
    return std::clamp (cutoff, 18.0, sampleRate * 0.45);
}
} // namespace

FilterCoeffs makeFilterCoeffs (const FilterParams& params, float cutoffHz,
                               float resonance, double sampleRate) noexcept
{
    FilterCoeffs c;

    const double cutoff = clampCutoff (static_cast<double> (cutoffHz), sampleRate);
    const double res = std::clamp (static_cast<double> (resonance), 0.0, 1.0);
    const double g = std::tan (pi * cutoff / sampleRate);
    const double a = g / (1.0 + g);
    const double b = 1.0 - a;

    c.ladderA = a;
    c.ladderB = b;
    c.ladderA4 = a * a * a * a;
    c.ladderK = res * 3.85;
    c.inputGain = 1.0f + static_cast<float> (params.drive) * 3.0f;
    c.outputGain = 1.0f / (1.0f + static_cast<float> (params.drive) * 0.55f);

    const double svfQ = 0.5 + res * 11.5;
    const double k = 1.0 / std::max (0.05, svfQ);
    const double svfG = std::tan (pi * cutoff / sampleRate);
    c.svfA1 = 1.0 / (1.0 + svfG * (svfG + k));
    c.svfA2 = svfG * c.svfA1;
    c.svfA3 = svfG * c.svfA2;
    c.svfK = k;

    const double hpf = clampCutoff (static_cast<double> (params.highPassHz), sampleRate);
    c.hpfG = 1.0 - std::exp (-2.0 * pi * hpf / sampleRate);
    return c;
}

float FilterState::process (float input, const FilterCoeffs& c,
                            FilterModel model, FilterType type) noexcept
{
    if (! std::isfinite (input))
        input = 0.0f;

    const double driven = std::tanh (static_cast<double> (input) * c.inputGain);
    double output = driven;

    if (model == FilterModel::Ladder4P)
    {
        const double s = c.ladderA * c.ladderA * c.ladderA * c.ladderB * l1
                       + c.ladderA * c.ladderA * c.ladderB * l2
                       + c.ladderA * c.ladderB * l3
                       + c.ladderB * l4;
        const double y4 = (c.ladderA4 * driven + s) / (1.0 + c.ladderK * c.ladderA4);
        const double u = driven - c.ladderK * y4;

        const double y1 = c.ladderA * u + c.ladderB * l1;
        l1 = 2.0 * y1 - l1;
        const double y2 = c.ladderA * y1 + c.ladderB * l2;
        l2 = 2.0 * y2 - l2;
        const double y3 = c.ladderA * y2 + c.ladderB * l3;
        l3 = 2.0 * y3 - l3;
        const double y4b = c.ladderA * y3 + c.ladderB * l4;
        l4 = 2.0 * y4b - l4;

        output = y4b;
    }
    else
    {
        const double v3 = driven - svfIc2;
        const double v1 = c.svfA1 * svfIc1 + c.svfA2 * v3;
        const double v2 = svfIc2 + c.svfA2 * svfIc1 + c.svfA3 * v3;
        svfIc1 = 2.0 * v1 - svfIc1;
        svfIc2 = 2.0 * v2 - svfIc2;

        switch (type)
        {
            case FilterType::LowPass:  output = v2; break;
            case FilterType::BandPass: output = v1; break;
            case FilterType::HighPass: output = driven - c.svfK * v1 - v2; break;
        }
    }

    // The post-filter high-pass removes rumble and keeps drive from building DC.
    hpfState += c.hpfG * (output - hpfState);
    output -= hpfState;

    if (! std::isfinite (output))
    {
        reset();
        return 0.0f;
    }

    // Bound pathological states without masking normal filter behaviour.
    l1 = std::clamp (l1, -16.0, 16.0);
    l2 = std::clamp (l2, -16.0, 16.0);
    l3 = std::clamp (l3, -16.0, 16.0);
    l4 = std::clamp (l4, -16.0, 16.0);
    svfIc1 = std::clamp (svfIc1, -16.0, 16.0);
    svfIc2 = std::clamp (svfIc2, -16.0, 16.0);

    return static_cast<float> (output * c.outputGain);
}
} // namespace synthset
