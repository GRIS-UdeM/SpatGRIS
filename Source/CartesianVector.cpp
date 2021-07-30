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

#include "CartesianVector.hpp"

#include "PolarVector.hpp"

#include <algorithm>

juce::String const CartesianVector::XmlTags::MAIN_TAG = "POSITION";
juce::String const CartesianVector::XmlTags::X = "X";
juce::String const CartesianVector::XmlTags::Y = "Y";
juce::String const CartesianVector::XmlTags::Z = "Z";

//==============================================================================
CartesianVector::CartesianVector(PolarVector const & polarVector) noexcept
{
    // Mathematically, the azimuth angle should start from the pole and be equal to 90 degrees at the equator.
    // We have to accomodate for a slightly different coordinate system where the azimuth angle starts at the equator
    // and is equal to 90 degrees at the north pole.

    // This is quite dangerous because any trigonometry done outside of this class might get things wrong.

    auto const inverseElevation{ HALF_PI - polarVector.elevation };

    x = polarVector.length * std::sin(inverseElevation.get()) * std::cos(polarVector.azimuth.get());
    y = polarVector.length * std::sin(inverseElevation.get()) * std::sin(polarVector.azimuth.get());
    z = polarVector.length * std::cos(inverseElevation.get());
}

//==============================================================================
CartesianVector CartesianVector::crossProduct(CartesianVector const & other) const noexcept
{
    auto const newX{ y * other.z - z * other.y };
    auto const newY{ z * other.x - x * other.z };
    auto const newZ{ x * other.y - y * other.x };
    CartesianVector const unscaledResult{ newX, newY, newZ };

    auto const length = unscaledResult.length();
    auto const result{ unscaledResult / length };

    return result;
}

//==============================================================================
juce::XmlElement * CartesianVector::toXml() const noexcept
{
    auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG) };
    result->setAttribute(XmlTags::X, x);
    result->setAttribute(XmlTags::Y, y);
    result->setAttribute(XmlTags::Z, z);
    return result.release();
}

//==============================================================================
tl::optional<CartesianVector> CartesianVector::fromXml(juce::XmlElement const & xml)
{
    juce::StringArray const requiredTags{ XmlTags::X, XmlTags::Y, XmlTags::Z };

    if (!std::all_of(requiredTags.begin(), requiredTags.end(), [&](juce::String const & tag) {
            return xml.hasAttribute(tag);
        })) {
        return tl::nullopt;
    }

    CartesianVector result;
    result.x = static_cast<float>(xml.getDoubleAttribute(XmlTags::X));
    result.y = static_cast<float>(xml.getDoubleAttribute(XmlTags::Y));
    result.z = static_cast<float>(xml.getDoubleAttribute(XmlTags::Z));

    return result;
}

//==============================================================================
float CartesianVector::angleWith(CartesianVector const & other) const noexcept
{
    auto inner = dotProduct(other) / std::sqrt(length2() * other.length2());
    inner = std::clamp(inner, -1.0f, 1.0f);
    return std::abs(std::acos(inner));
}

//==============================================================================
float CartesianVector::length() const noexcept
{
    return std::sqrt(length2());
}
