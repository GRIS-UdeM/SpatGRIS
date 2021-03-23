#include "PolarVector.h"

//==============================================================================
CartesianVector PolarVector::toCartesian() const noexcept
{
    using fast = juce::dsp::FastMathApproximations;

    auto const cele = fast::cos(zenith.get());
    auto const x = fast::cos(azimuth.get()) * cele;
    auto const y = fast::sin(azimuth.get()) * cele;
    auto const z = fast::sin(zenith.get());

    CartesianVector const result{ x, y, z };
    return result;
}
