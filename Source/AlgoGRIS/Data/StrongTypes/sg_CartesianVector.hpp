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

#include "../sg_Macros.hpp"
#include "../sg_constants.hpp"

namespace gris
{
struct PolarVector;

//==============================================================================
/** A straightforward 3D [x,y,z] cartesian vector. */
struct CartesianVector {
    //==============================================================================
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const X;
        static juce::String const Y;
        static juce::String const Z;
    };
    //==============================================================================
    float x{};
    float y{};
    float z{};
    //==============================================================================
    CartesianVector() = default;
    constexpr CartesianVector(float newX, float newY, float newZ) noexcept;
    explicit CartesianVector(PolarVector const & polarVector) noexcept;
    ~CartesianVector() = default;
    SG_DEFAULT_COPY_AND_MOVE(CartesianVector)
    //==============================================================================
    [[nodiscard]] constexpr bool operator==(CartesianVector const & other) const noexcept;
    [[nodiscard]] constexpr bool operator!=(CartesianVector const & other) const noexcept;
    [[nodiscard]] constexpr CartesianVector operator-(CartesianVector const & other) const noexcept;
    [[nodiscard]] constexpr CartesianVector operator/(float scalar) const noexcept;
    [[nodiscard]] constexpr CartesianVector operator-() const noexcept;
    //==============================================================================
    /** @return a vector identical to this one but with its x value replaced by newX. */
    [[nodiscard]] constexpr CartesianVector withX(float newX) const noexcept;
    /** @return a vector identical to this one but with its y value replaced by newY. */
    [[nodiscard]] constexpr CartesianVector withY(float newY) const noexcept;
    /** @return a vector identical to this one but with its z value replaced by newZ. */
    [[nodiscard]] constexpr CartesianVector withZ(float newZ) const noexcept;
    /** @return a vector identical to this one but with its x value moved by delta. */
    [[nodiscard]] constexpr CartesianVector translatedX(float delta) const noexcept;
    /** @return a vector identical to this one but with its y value moved by delta. */
    [[nodiscard]] constexpr CartesianVector translatedY(float delta) const noexcept;
    /** @return a vector identical to this one but with its z value moved by delta. */
    [[nodiscard]] constexpr CartesianVector translatedZ(float delta) const noexcept;
    //==============================================================================
    /** @return this vector clipped to the MBAP extended field. */
    [[nodiscard]] constexpr CartesianVector clampedToFarField() const noexcept;
    /** @return the midpoint between this vector and another one. */
    [[nodiscard]] constexpr CartesianVector midPoint(CartesianVector const & other) const noexcept;
    /** @return a flat projection on the x-y plane of this vector, in the form of a standard JUCE point. */
    [[nodiscard]] constexpr juce::Point<float> discardZ() const noexcept;
    /** @return the vector's squared euclidean length. */
    [[nodiscard]] constexpr float length2() const noexcept;
    /** @return the vector's squared euclidean length. This function is MUCH slower than length() but can be called at
     * constexpr-time. */
    [[nodiscard]] constexpr float constexprLength() const noexcept;
    /** @return the dot product between this vector and another one. */
    [[nodiscard]] constexpr float dotProduct(CartesianVector const & other) const noexcept;
    [[nodiscard]] float angleWith(CartesianVector const & other) const noexcept;
    /** @return the vector's euclidean length. */
    [[nodiscard]] float length() const noexcept;
    [[nodiscard]] CartesianVector crossProduct(CartesianVector const & other) const noexcept;
    /** @return an XML representation of this vector. */
    [[nodiscard]] std::unique_ptr<juce::XmlElement> toXml() const noexcept;
    //==============================================================================
    /** @return the CartesianVector encoded in an XML element. tl::nullopt if parsing fails. */
    [[nodiscard]] static tl::optional<CartesianVector> fromXml(juce::XmlElement const & xml);
};

//==============================================================================
constexpr CartesianVector::CartesianVector(float const newX, float const newY, float const newZ) noexcept
    : x(newX)
    , y(newY)
    , z(newZ)
{
}

//==============================================================================
constexpr bool CartesianVector::operator==(CartesianVector const & other) const noexcept
{
    return x == other.x && y == other.y && z == other.z;
}

//==============================================================================
constexpr bool CartesianVector::operator!=(CartesianVector const & other) const noexcept
{
    return !(*this == other);
}

//==============================================================================
constexpr CartesianVector CartesianVector::operator-(CartesianVector const & other) const noexcept
{
    CartesianVector const result{ x - other.x, y - other.y, z - other.z };
    return result;
}

//==============================================================================
constexpr CartesianVector CartesianVector::operator/(float const scalar) const noexcept
{
    CartesianVector const result{ x / scalar, y / scalar, z / scalar };
    return result;
}

//==============================================================================
constexpr CartesianVector CartesianVector::operator-() const noexcept
{
    return CartesianVector{ -x, -y, -z };
}

//==============================================================================
constexpr CartesianVector CartesianVector::withX(float const newX) const noexcept
{
    auto result{ *this };
    result.x = newX;
    return result;
}

//==============================================================================
constexpr CartesianVector CartesianVector::withY(float const newY) const noexcept
{
    auto result{ *this };
    result.y = newY;
    return result;
}

//==============================================================================
constexpr CartesianVector CartesianVector::withZ(float const newZ) const noexcept
{
    auto result{ *this };
    result.z = newZ;
    return result;
}

//==============================================================================
constexpr CartesianVector CartesianVector::translatedX(float const delta) const noexcept
{
    return withX(x + delta);
}

//==============================================================================
constexpr CartesianVector CartesianVector::translatedY(float const delta) const noexcept
{
    return withY(y + delta);
}

//==============================================================================
constexpr CartesianVector CartesianVector::translatedZ(float const delta) const noexcept
{
    return withZ(z + delta);
}

//==============================================================================
constexpr float CartesianVector::dotProduct(CartesianVector const & other) const noexcept
{
    return x * other.x + y * other.y + z * other.z;
}

//==============================================================================
constexpr CartesianVector CartesianVector::clampedToFarField() const noexcept
{
    return CartesianVector{ std::clamp(x, -MBAP_EXTENDED_RADIUS, MBAP_EXTENDED_RADIUS),
                            std::clamp(y, -MBAP_EXTENDED_RADIUS, MBAP_EXTENDED_RADIUS),
                            std::clamp(z, -MBAP_EXTENDED_RADIUS, MBAP_EXTENDED_RADIUS) };
}

//==============================================================================
constexpr CartesianVector CartesianVector::midPoint(CartesianVector const & other) const noexcept
{
    auto const newX{ (x + other.x) * 0.5f };
    auto const newY{ (y + other.y) * 0.5f };
    auto const newZ{ (z + other.z) * 0.5f };

    CartesianVector const result{ newX, newY, newZ };
    return result;
}

//==============================================================================
constexpr juce::Point<float> CartesianVector::discardZ() const noexcept
{
    return juce::Point<float>{ x, y };
}

//==============================================================================
constexpr float CartesianVector::length2() const noexcept
{
    return x * x + y * y + z * z;
}

//==============================================================================
template<typename T>
static constexpr T sqrtNewtonRaphson(T const x, T const current, T const previous)
{
    static_assert(std::is_floating_point_v<T>, "only works with floating point values");
    return current == previous ? current : sqrtNewtonRaphson(x, static_cast<T>(0.5) * (current + x / current), current);
}

//==============================================================================
constexpr float CartesianVector::constexprLength() const noexcept
{
    auto const value{ length2() };
    return value >= 0.0f && value < std::numeric_limits<float>::infinity() ? sqrtNewtonRaphson(value, value, 0.0f)
                                                                           : std::numeric_limits<float>::quiet_NaN();
}

//==============================================================================
static_assert(std::is_trivially_destructible_v<CartesianVector>);

} // namespace gris
