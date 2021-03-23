#pragma once

#include "CartesianVector.h"

/* Angular vector for a speaker position. */
struct PolarVector {
    radians_t azimuth;
    radians_t zenith;
    float length;

    /* Converts a vector from angular to cartesian coordinates. */
    [[nodiscard]] CartesianVector toCartesian() const noexcept;

    [[nodiscard]] constexpr bool isOnSameElevation(PolarVector const & other,
                                                   radians_t const tolerance
                                                   = toRadians(degrees_t{ 5.0f })) const noexcept
    {
        return zenith > other.zenith - tolerance && zenith < other.zenith + tolerance;
    }
};