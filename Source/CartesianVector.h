#pragma once

#include "lib/tl/optional.hpp"

#include <cmath>

#include <JuceHeader.h>

struct CartesianVector {
    struct XmlTags {
        static juce::String const MAIN_TAG;
        static juce::String const X;
        static juce::String const Y;
        static juce::String const Z;
    };

    float x;
    float y;
    float z;

    [[nodiscard]] constexpr bool operator==(CartesianVector const & other) const noexcept
    {
        return x == other.x && y == other.y && z == other.z;
    }

    [[nodiscard]] constexpr CartesianVector operator-(CartesianVector const & other) const noexcept
    {
        CartesianVector const result{ x - other.x, y - other.y, z - other.z };
        return result;
    }

    [[nodiscard]] constexpr CartesianVector operator/(float const scalar) const noexcept
    {
        CartesianVector const result{ x / scalar, y / scalar, z / scalar };
        return result;
    }

    /* Returns the vector length without the sqrt. */
    [[nodiscard]] constexpr float length2() const noexcept { return x * x + y * y + z * z; }

    [[nodiscard]] float length() const noexcept { return std::sqrt(length2()); }

    [[nodiscard]] CartesianVector crossProduct(CartesianVector const & other) const noexcept;

    [[nodiscard]] constexpr CartesianVector operator-() const noexcept { return CartesianVector{ -x, -y, -z }; }

    [[nodiscard]] constexpr float dotProduct(CartesianVector const & other) const noexcept
    {
        return x * other.x + y * other.y + z * other.z;
    }

    [[nodiscard]] constexpr CartesianVector mean(CartesianVector const & other) const noexcept
    {
        auto const newX{ (x + other.x) * 0.5f };
        auto const newY{ (y + other.y) * 0.5f };
        auto const newZ{ (z + other.z) * 0.5f };

        CartesianVector const result{ newX, newY, newZ };
        return result;
    }

    [[nodiscard]] float angleWith(CartesianVector const & other) const noexcept;

    [[nodiscard]] juce::XmlElement * toXml() const noexcept
    {
        auto result{ std::make_unique<juce::XmlElement>(XmlTags::MAIN_TAG) };
        result->setAttribute(XmlTags::X, x);
        result->setAttribute(XmlTags::Y, y);
        result->setAttribute(XmlTags::Z, z);
        return result.release();
    }

    [[nodiscard]] static tl::optional<CartesianVector> fromXml(juce::XmlElement const & xml)
    {
        juce::StringArray const requiredTags{ XmlTags::X, XmlTags::Y, XmlTags::Z };

        if (!std::all_of(requiredTags.begin(), requiredTags.end(), [&](juce::String const & tag) {
                return xml.hasAttribute(tag);
            })) {
            return tl::nullopt;
        }

        CartesianVector result;
        result.x = xml.getDoubleAttribute(XmlTags::X);
        result.y = xml.getDoubleAttribute(XmlTags::Y);
        result.z = xml.getDoubleAttribute(XmlTags::Z);

        return result;
    }
};