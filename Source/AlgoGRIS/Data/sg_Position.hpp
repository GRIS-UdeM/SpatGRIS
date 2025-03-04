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

#include "StrongTypes/sg_CartesianVector.hpp"
#include "sg_PolarVector.hpp"

namespace gris
{
//==============================================================================
class Position
{
    PolarVector mPolar{};
    CartesianVector mCartesian{};

public:
    //==============================================================================
    Position() = default;
    explicit Position(PolarVector const & polar) : mPolar(polar), mCartesian(CartesianVector{ polar }) {}
    explicit Position(CartesianVector const & cartesian) : mPolar(PolarVector{ cartesian }), mCartesian(cartesian) {}
    ~Position() = default;
    SG_DEFAULT_COPY_AND_MOVE(Position)
    //==============================================================================
    [[nodiscard]] constexpr auto const & getPolar() const noexcept { return mPolar; }
    [[nodiscard]] constexpr auto const & getCartesian() const noexcept { return mCartesian; }
    //==============================================================================
    [[nodiscard]] constexpr bool operator==(Position const & other) const noexcept;
    //==============================================================================
    Position & operator=(PolarVector const & polar) noexcept;
    Position & operator=(CartesianVector const & cartesian) noexcept;
    //==============================================================================
    [[nodiscard]] Position withAzimuth(radians_t azimuth) const noexcept;
    [[nodiscard]] Position withBalancedAzimuth(radians_t azimuth) const noexcept;
    [[nodiscard]] Position withElevation(radians_t elevation) const noexcept;
    [[nodiscard]] Position withClippedElevation(radians_t elevation) const noexcept;
    [[nodiscard]] Position withRadius(float radius) const noexcept;
    [[nodiscard]] Position withPositiveRadius(float radius) const noexcept;
    [[nodiscard]] Position withX(float x) const noexcept;
    [[nodiscard]] Position withY(float y) const noexcept;
    [[nodiscard]] Position withZ(float z) const noexcept;
    [[nodiscard]] Position rotatedAzimuth(radians_t azimuthDelta) const noexcept;
    [[nodiscard]] Position rotatedBalancedAzimuth(radians_t azimuthDelta) const noexcept;
    [[nodiscard]] Position elevated(radians_t elevationDelta) const noexcept;
    [[nodiscard]] Position elevatedClipped(radians_t elevationDelta) const noexcept;
    [[nodiscard]] Position pushed(float radiusDelta) const noexcept;
    [[nodiscard]] Position pushedWithPositiveRadius(float radiusDelta) const noexcept;
    [[nodiscard]] Position normalized() const noexcept;
    [[nodiscard]] Position translatedX(float delta) const noexcept;
    [[nodiscard]] Position translatedY(float delta) const noexcept;
    [[nodiscard]] Position translatedZ(float delta) const noexcept;

private:
    //==============================================================================
    void updatePolarFromCartesian() noexcept;
    void updateCartesianFromPolar() noexcept;
};

//==============================================================================
constexpr bool Position::operator==(Position const & other) const noexcept
{
    return mCartesian == other.mCartesian;
}

//==============================================================================
static_assert(std::is_trivially_destructible_v<Position>);

} // namespace gris
