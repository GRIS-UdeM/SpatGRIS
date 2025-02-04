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
class degrees_t;

//==============================================================================
/** Strongly-typed radians. */
class radians_t final : public StrongFloat<float, radians_t, struct RadiansT>
{
public:
    static constexpr type RADIAN_PER_DEGREE{ juce::MathConstants<type>::twoPi / static_cast<type>(360) };
    //==============================================================================
    radians_t() = default;
    explicit constexpr radians_t(type const & value);
    explicit constexpr radians_t(degrees_t const & degrees);
    //==============================================================================
    [[nodiscard]] constexpr radians_t balanced() const noexcept;
    [[nodiscard]] constexpr radians_t madePositive() const noexcept;
};

} // namespace gris

//==============================================================================
#include "sg_Degrees.hpp"

namespace gris
{
//==============================================================================
constexpr radians_t::radians_t(degrees_t const & degrees) : StrongFloat(degrees.get() * RADIAN_PER_DEGREE)
{
}

//==============================================================================
constexpr radians_t::radians_t(type const & value) : StrongFloat(value)
{
}

//==============================================================================
[[nodiscard]] constexpr radians_t radians_t::balanced() const noexcept
{
    return centeredAroundZero(juce::MathConstants<type>::twoPi);
}

//==============================================================================
[[nodiscard]] constexpr radians_t radians_t::madePositive() const noexcept
{
    return radians_t{ mValue < 0 ? mValue + juce::MathConstants<type>::twoPi : mValue };
}

//==============================================================================
constexpr radians_t QUARTER_PI{ juce::MathConstants<radians_t::type>::halfPi / 2.0f };
constexpr radians_t HALF_PI{ juce::MathConstants<radians_t::type>::halfPi };
constexpr radians_t PI{ juce::MathConstants<radians_t::type>::pi };
constexpr radians_t TWO_PI{ juce::MathConstants<radians_t::type>::twoPi };

} // namespace gris
