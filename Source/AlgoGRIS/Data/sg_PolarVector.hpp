/*
 This file is part of SpatGRIS.

 Developers: Gaël Lane Lépine, Samuel Béland, Olivier Bélanger, Nicolas Masson

 SpatGRIS is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "sg_Macros.hpp"
#include "StrongTypes/sg_Radians.hpp"

namespace gris
{
struct CartesianVector;

static constexpr radians_t DEFAULT_ELEVATION_COMPARE_TOLERANCE{ degrees_t{ 5.0f } };

//==============================================================================
struct PolarVector {
    radians_t azimuth{};
    radians_t elevation{};
    float length{};
    //==============================================================================
    PolarVector() = default;
    constexpr PolarVector(radians_t newAzimuth, radians_t newElevation, float newLength) noexcept;
    explicit PolarVector(CartesianVector const & cartesianVector) noexcept;
    ~PolarVector() = default;
    SG_DEFAULT_COPY_AND_MOVE(PolarVector)
    //==============================================================================
    [[nodiscard]] constexpr bool operator==(PolarVector const & other) const noexcept;
    [[nodiscard]] constexpr PolarVector normalized() const noexcept;
    [[nodiscard]] constexpr bool isOnSameElevation(PolarVector const & other,
                                                   radians_t tolerance
                                                   = DEFAULT_ELEVATION_COMPARE_TOLERANCE) const noexcept;
    //==============================================================================
    [[nodiscard]] constexpr PolarVector withAzimuth(radians_t newAzimuth) const noexcept;
    [[nodiscard]] constexpr PolarVector withBalancedAzimuth(radians_t newAzimuth) const noexcept;
    [[nodiscard]] constexpr PolarVector withElevation(radians_t newElevation) const noexcept;
    [[nodiscard]] constexpr PolarVector withClippedElevation(radians_t newElevation) const noexcept;
    [[nodiscard]] constexpr PolarVector withRadius(float newRadius) const noexcept;
    [[nodiscard]] constexpr PolarVector withPositiveRadius(float newRadius) const noexcept;
    [[nodiscard]] constexpr PolarVector rotatedAzimuth(radians_t azimuthDelta) const noexcept;
    [[nodiscard]] constexpr PolarVector rotatedBalancedAzimuth(radians_t azimuthDelta) const noexcept;
    [[nodiscard]] constexpr PolarVector elevated(radians_t elevationDelta) const noexcept;
    [[nodiscard]] constexpr PolarVector elevatedClipped(radians_t elevationDelta) const noexcept;
    [[nodiscard]] constexpr PolarVector pushed(float radiusDelta) const noexcept;
    [[nodiscard]] constexpr PolarVector pushedWithPositiveRadius(float radiusDelta) const noexcept;
};

//==============================================================================
constexpr PolarVector::PolarVector(radians_t const newAzimuth,
                                   radians_t const newElevation,
                                   float const newLength) noexcept
    : azimuth(newAzimuth)
    , elevation(newElevation)
    , length(newLength)
{
}

//==============================================================================
constexpr bool PolarVector::operator==(PolarVector const & other) const noexcept
{
    return azimuth == other.azimuth && elevation == other.elevation && length == other.length;
}

//==============================================================================
constexpr PolarVector PolarVector::normalized() const noexcept
{
    // NOTE : removing this check doesn't seem to break anything, but I'm pretty I put that in for a reason!

    // if (length == 0.0f) {
    //    return PolarVector{ HALF_PI, radians_t{}, 1.0f };
    //}

    return PolarVector{ azimuth, elevation, 1.0f };
}

//==============================================================================
constexpr bool PolarVector::isOnSameElevation(PolarVector const & other, radians_t const tolerance) const noexcept
{
    return elevation > other.elevation - tolerance && elevation < other.elevation + tolerance;
}

//==============================================================================
constexpr PolarVector PolarVector::withAzimuth(radians_t const newAzimuth) const noexcept
{
    auto result{ *this };
    result.azimuth = newAzimuth;
    return result;
}

//==============================================================================
constexpr PolarVector PolarVector::withBalancedAzimuth(radians_t const newAzimuth) const noexcept
{
    auto result{ *this };
    result.azimuth = newAzimuth.balanced();
    return result;
}

//==============================================================================
constexpr PolarVector PolarVector::withElevation(radians_t const newElevation) const noexcept
{
    auto result{ *this };
    result.elevation = newElevation;
    return result;
}

//==============================================================================
constexpr PolarVector PolarVector::withClippedElevation(radians_t const newElevation) const noexcept
{
    auto result{ *this };
    if (newElevation >= TWO_PI || newElevation < -HALF_PI) {
        result.elevation = radians_t{ 0.0f };
    } else if (newElevation >= HALF_PI && newElevation <= PI) {
        // replace with this line to wrap around elevation
        // result.elevation = std::clamp(newElevation, PI + HALF_PI, TWO_PI);
        result.elevation = HALF_PI;
    } else if (newElevation < PI + HALF_PI && newElevation > PI) {
        // replace with this line to wrap around elevation
        // result.elevation = std::clamp(newElevation, radians_t{ 0.0f }, HALF_PI);
        result.elevation = PI + HALF_PI;
    } else if (newElevation < HALF_PI && newElevation >= radians_t{ 0.0f }) {
        result.elevation = std::clamp(newElevation, radians_t{ 0.0f }, HALF_PI);
    } else if (newElevation >= PI + HALF_PI && newElevation < TWO_PI) {
        result.elevation = std::clamp(newElevation, PI + HALF_PI, TWO_PI);
    } else if (newElevation < radians_t{ 0.0f }) {
        result.elevation = TWO_PI + newElevation;
    }
    return result;
}

//==============================================================================
constexpr PolarVector PolarVector::withRadius(float const newRadius) const noexcept
{
    auto result{ *this };
    result.length = newRadius;
    return result;
}

//==============================================================================
constexpr PolarVector PolarVector::withPositiveRadius(float const newRadius) const noexcept
{
    auto result{ *this };
    result.length = std::max(newRadius, 0.0f);
    return result;
}

//==============================================================================
constexpr PolarVector PolarVector::rotatedAzimuth(radians_t const azimuthDelta) const noexcept
{
    return withAzimuth(azimuth + azimuthDelta);
}

//==============================================================================
constexpr PolarVector PolarVector::rotatedBalancedAzimuth(radians_t const azimuthDelta) const noexcept
{
    return withBalancedAzimuth(azimuth + azimuthDelta);
}

//==============================================================================
constexpr PolarVector PolarVector::elevated(radians_t const elevationDelta) const noexcept
{
    return withElevation(elevation + elevationDelta);
}

//==============================================================================
constexpr PolarVector PolarVector::elevatedClipped(radians_t const elevationDelta) const noexcept
{
    return withClippedElevation(elevation + elevationDelta);
}

//==============================================================================
constexpr PolarVector PolarVector::pushed(float const radiusDelta) const noexcept
{
    return withRadius(length + radiusDelta);
}

//==============================================================================
constexpr PolarVector PolarVector::pushedWithPositiveRadius(float const radiusDelta) const noexcept
{
    return withPositiveRadius(length + radiusDelta);
}

//==============================================================================
static_assert(std::is_trivially_destructible_v<PolarVector>);
} // namespace gris
