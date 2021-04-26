#pragma once

#include "PolarVector.h"

struct LegacyLbapPosition {
    radians_t azimuth{};
    radians_t elevation{};
    float floorDistance{};

    [[nodiscard]] PolarVector toPolar() const noexcept;

    [[nodiscard]] CartesianVector toCartesian() const noexcept;
};