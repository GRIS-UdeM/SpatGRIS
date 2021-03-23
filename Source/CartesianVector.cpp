#include "CartesianVector.h"

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
