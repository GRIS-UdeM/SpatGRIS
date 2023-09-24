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

#include "sg_Position.hpp"

namespace gris
{
//==============================================================================
Position & Position::operator=(PolarVector const & polar) noexcept
{
    mPolar = polar;
    updateCartesianFromPolar();
    return *this;
}

//==============================================================================
Position & Position::operator=(CartesianVector const & cartesian) noexcept
{
    mCartesian = cartesian;
    updatePolarFromCartesian();
    return *this;
}

//==============================================================================
Position Position::withAzimuth(radians_t const azimuth) const noexcept
{
    return Position{ mPolar.withAzimuth(azimuth) };
}

//==============================================================================
Position Position::withBalancedAzimuth(radians_t const azimuth) const noexcept
{
    return Position{ mPolar.withBalancedAzimuth(azimuth) };
}

//==============================================================================
Position Position::withClippedElevation(radians_t const elevation) const noexcept
{
    return Position{ mPolar.withClippedElevation(elevation) };
}

//==============================================================================
Position Position::rotatedBalancedAzimuth(radians_t const azimuthDelta) const noexcept
{
    return Position{ mPolar.rotatedBalancedAzimuth(azimuthDelta) };
}

//==============================================================================
Position Position::elevatedClipped(radians_t const elevationDelta) const noexcept
{
    return Position{ mPolar.elevatedClipped(elevationDelta) };
}

//==============================================================================
Position Position::withElevation(radians_t const elevation) const noexcept
{
    return Position{ mPolar.withElevation(elevation) };
}

//==============================================================================
Position Position::withRadius(float const radius) const noexcept
{
    return Position{ mPolar.withRadius(radius) };
}

//==============================================================================
Position Position::withPositiveRadius(float const radius) const noexcept
{
    return Position{ mPolar.withPositiveRadius(radius) };
}

//==============================================================================
Position Position::pushedWithPositiveRadius(float const radiusDelta) const noexcept
{
    return Position{ mPolar.pushedWithPositiveRadius(radiusDelta) };
}

//==============================================================================
Position Position::normalized() const noexcept
{
    return Position{ mPolar.normalized() };
}

//==============================================================================
Position Position::withX(float const x) const noexcept
{
    return Position{ mCartesian.withX(x) };
}

//==============================================================================
Position Position::withY(float const y) const noexcept
{
    return Position{ mCartesian.withY(y) };
}

//==============================================================================
Position Position::withZ(float const z) const noexcept
{
    return Position{ mCartesian.withZ(z) };
}

//==============================================================================
Position Position::rotatedAzimuth(radians_t const azimuthDelta) const noexcept
{
    return Position{ mPolar.rotatedAzimuth(azimuthDelta) };
}

//==============================================================================
Position Position::elevated(radians_t const elevationDelta) const noexcept
{
    return Position{ mPolar.elevated(elevationDelta) };
}

//==============================================================================
Position Position::pushed(float const radiusDelta) const noexcept
{
    return Position{ mPolar.pushed(radiusDelta) };
}

//==============================================================================
Position Position::translatedX(float const delta) const noexcept
{
    return Position{ mCartesian.translatedX(delta) };
}

//==============================================================================
Position Position::translatedY(float const delta) const noexcept
{
    return Position{ mCartesian.translatedY(delta) };
}

//==============================================================================
Position Position::translatedZ(float const delta) const noexcept
{
    return Position{ mCartesian.translatedZ(delta) };
}

//==============================================================================
void Position::updatePolarFromCartesian() noexcept
{
    mPolar = PolarVector{ mCartesian };
}

//==============================================================================
void Position::updateCartesianFromPolar() noexcept
{
    mCartesian = CartesianVector{ mPolar };
}

} // namespace gris
