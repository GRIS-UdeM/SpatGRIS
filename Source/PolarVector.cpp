#include "PolarVector.h"

#define FAST_TRIGO 1

#ifdef FAST_TRIGO
using fast = juce::dsp::FastMathApproximations;
    #define SIN(x) fast::sin(x)
    #define COS(x) fast::cos(x)
    #define ACOS(x) fast::acos(x)
#else
    #define SIN(x) std::sin(x)
    #define COS(x) std::cos(x)
    #define ACOS(x) std::acos(x)
#endif

//==============================================================================
CartesianVector PolarVector::toCartesian() const noexcept
{
    auto const x{ length * SIN(elevation.get()) * COS(azimuth.get()) };
    auto const y{ length * SIN(elevation.get()) * SIN(azimuth.get()) };
    auto const z{ length * COS(elevation.get()) };

    CartesianVector const result{ x, y, z };
    return result;
}

//==============================================================================
PolarVector PolarVector::fromCartesian(CartesianVector const & pos) noexcept
{
    auto const length{ std::sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z) };
    radians_t const azimuth{ std::acos(pos.x / std::sqrt(pos.x * pos.x + pos.y * pos.y))
                             * (pos.y < 0.0f ? -1.0f : 1.0f) };
    radians_t const zenith{ std::acos(pos.z / length) };

    PolarVector const result{ azimuth, zenith, length };
    return result;
}
