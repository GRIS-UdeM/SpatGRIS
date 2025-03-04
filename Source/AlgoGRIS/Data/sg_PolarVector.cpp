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

#include "StrongTypes/sg_CartesianVector.hpp"
#include "sg_PolarVector.hpp"

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

namespace gris
{
//==============================================================================
PolarVector::PolarVector(CartesianVector const & cartesian) noexcept
{
    // Mathematically, the azimuth angle should start from the pole and be equal to 90 degrees at the equator.
    // We have to accomodate for a slightly different coordinate system where the azimuth angle starts at the equator
    // and is equal to 90 degrees at the north pole.

    // This is quite dangerous because any trigonometry done outside of this class might get things wrong.

    length = std::sqrt(cartesian.x * cartesian.x + cartesian.y * cartesian.y + cartesian.z * cartesian.z);
    if (length == 0.0f) {
        return;
    }

    elevation = HALF_PI - radians_t{ std::acos(std::clamp(cartesian.z / length, -1.0f, 1.0f)) };

    if (cartesian.x == 0.0f && cartesian.y == 0.0f) {
        return;
    }

    azimuth = radians_t{
        std::acos(
            std::clamp(cartesian.x / std::sqrt(cartesian.x * cartesian.x + cartesian.y * cartesian.y), -1.0f, 1.0f))
        * (cartesian.y < 0.0f ? -1.0f : 1.0f)
    };
}

} // namespace gris
