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

#include "sg_StrongFloat.hpp"

#include <JuceHeader.h>

namespace gris
{
class radians_t;

//==============================================================================
/** Strongly-typed degrees. */
class degrees_t final : public StrongFloat<float, degrees_t, struct DegreesT>
{
public:
    static constexpr type DEGREE_PER_RADIAN{ static_cast<type>(360) / juce::MathConstants<type>::twoPi };
    //==============================================================================
    degrees_t() = default;
    explicit constexpr degrees_t(type const & value) : StrongFloat(value) {}
    explicit constexpr degrees_t(radians_t const & radians);
    //==============================================================================
    [[nodiscard]] constexpr degrees_t centered() const noexcept;
    [[nodiscard]] constexpr degrees_t madePositive() const noexcept;
};

} // namespace gris

//==============================================================================
#include "sg_Radians.hpp"

namespace gris
{
//==============================================================================
constexpr degrees_t::degrees_t(radians_t const & radians) : StrongFloat(radians.get() * DEGREE_PER_RADIAN)
{
}

//==============================================================================
[[nodiscard]] constexpr degrees_t degrees_t::centered() const noexcept
{
    return centeredAroundZero(static_cast<type>(360));
}

//==============================================================================
[[nodiscard]] constexpr degrees_t degrees_t::madePositive() const noexcept
{
    return degrees_t{ mValue < 0 ? mValue + static_cast<type>(360) : mValue };
}

} // namespace gris
