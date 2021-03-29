#pragma once

#include "CartesianVector.h"
#include "StrongTypes.hpp"

static constexpr radians_t DEFAULT_ELEVATION_COMPARE_TOLERANCE{ degrees_t{ 5.0f } };

struct PolarVector {
    radians_t azimuth;
    radians_t elevation;
    float length;

    /* Converts a vector from angular to cartesian coordinates. */
    [[nodiscard]] CartesianVector toCartesian() const noexcept;

    [[nodiscard]] constexpr bool isOnSameElevation(PolarVector const & other,
                                                   radians_t const tolerance
                                                   = DEFAULT_ELEVATION_COMPARE_TOLERANCE) const noexcept
    {
        return elevation > other.elevation - tolerance && elevation < other.elevation + tolerance;
    }

    [[nodiscard]] static PolarVector fromCartesian(CartesianVector const & cartesianVector);
};