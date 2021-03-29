#include "CartesianVector.h"

#include <algorithm>

//==============================================================================
CartesianVector CartesianVector::crossProduct(CartesianVector const & other) const noexcept
{
    auto const newX = (y * other.z) - (z * other.y);
    auto const newY = (z * other.x) - (x * other.z);
    auto const newZ = (x * other.y) - (y * other.x);
    CartesianVector const unscaledResult{ newX, newY, newZ };

    auto const length = unscaledResult.length();
    auto const result{ unscaledResult / length };

    return result;
}

//==============================================================================
float CartesianVector::angleWith(CartesianVector const & other) const noexcept
{
    auto inner = dotProduct(other) / std::sqrt(length2() * other.length2());
    inner = std::clamp(inner, -1.0f, 1.0f);
    return std::abs(std::acos(inner));
}

//==============================================================================
PolarVector CartesianVector::toPolar() const noexcept
{
    jassertfalse;
    return {};
}
