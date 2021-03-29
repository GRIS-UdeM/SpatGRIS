#include "PolarVector.h"

//==============================================================================
CartesianVector PolarVector::toCartesian() const noexcept
{
    using fast = juce::dsp::FastMathApproximations;

    auto const cele = fast::cos(elevation.get());
    auto const x = fast::cos(azimuth.get()) * cele;
    auto const y = fast::sin(azimuth.get()) * cele;
    auto const z = fast::sin(elevation.get());

    CartesianVector const result{ x, y, z };
    return result;
}

//==============================================================================
PolarVector PolarVector::fromCartesian(CartesianVector const & cartesianVector)
{
    jassertfalse; // TODO
    return {};
}
