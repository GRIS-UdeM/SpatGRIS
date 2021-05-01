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

#include "CartesianVector.h"

#include <algorithm>

juce::String const CartesianVector::XmlTags::MAIN_TAG = "POSITION";
juce::String const CartesianVector::XmlTags::X = "X";
juce::String const CartesianVector::XmlTags::Y = "Y";
juce::String const CartesianVector::XmlTags::Z = "Z";

//==============================================================================
CartesianVector CartesianVector::crossProduct(CartesianVector const & other) const noexcept
{
    auto const newX = (y * other.z) - (z * other.y);
    auto const newY = (z * other.x) - (x * other.z);
    auto const newZ = (x * other.y) - (y * other.x);
    CartesianVector const unscaledResult{ newX, newY, newZ };

    auto const length = unscaledResult.length();
    auto const result{ unscaledResult / length };

    return result;
}

//==============================================================================
float CartesianVector::angleWith(CartesianVector const & other) const noexcept
{
    auto inner = dotProduct(other) / std::sqrt(length2() * other.length2());
    inner = std::clamp(inner, -1.0f, 1.0f);
    return std::abs(std::acos(inner));
}
