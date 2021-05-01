/*
 This file is part of SpatGRIS.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

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
    // Mathematically, the azimuth angle should start from the pole and be equal to 90 degrees at the equator.
    // We have to accomodate for a slightly different coordinate system where the azimuth angle starts at the equator
    // and is equal to 90 degrees at the north pole.

    // This is quite dangerous because any trigonometry done outside of this class might get things wrong.

    auto const inverseElevation{ HALF_PI - elevation };

    auto const x{ length * SIN(inverseElevation.get()) * COS(azimuth.get()) };
    auto const y{ length * SIN(inverseElevation.get()) * SIN(azimuth.get()) };
    auto const z{ length * COS(inverseElevation.get()) };

    CartesianVector const result{ x, y, z };
    return result;
}

//==============================================================================
PolarVector PolarVector::fromCartesian(CartesianVector const & pos) noexcept
{
    // Mathematically, the azimuth angle should start from the pole and be equal to 90 degrees at the equator.
    // We have to accomodate for a slightly different coordinate system where the azimuth angle starts at the equator
    // and is equal to 90 degrees at the north pole.

    // This is quite dangerous because any trigonometry done outside of this class might get things wrong.

    auto const length{ std::sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z) };
    if (length == 0.0f) {
        return PolarVector{ radians_t{}, radians_t{}, 0.0f };
    }

    radians_t const zenith{ std::acos(pos.z / length) };
    auto const inverseZenith{ HALF_PI - zenith };

    if (pos.x == 0.0f && pos.y == 0.0f) {
        return PolarVector{ radians_t{}, inverseZenith, length };
    }

    radians_t const azimuth{ std::acos(pos.x / std::sqrt(pos.x * pos.x + pos.y * pos.y))
                             * (pos.y < 0.0f ? -1.0f : 1.0f) };

    PolarVector const result{ azimuth, inverseZenith, length };
    return result;
}
