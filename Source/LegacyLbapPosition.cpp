#include "LegacyLbapPosition.h"

//==============================================================================
PolarVector LegacyLbapPosition::toPolar() const noexcept
{
    return PolarVector::fromCartesian(toCartesian());
}

//==============================================================================
CartesianVector LegacyLbapPosition::toCartesian() const noexcept
{
    auto const x{ floorDistance * std::cos(azimuth.get()) };
    auto const y{ floorDistance * std::sin(azimuth.get()) };
    auto const z{ 1.0f - (HALF_PI - elevation) / HALF_PI };

    return CartesianVector{ x, y, z };
}
