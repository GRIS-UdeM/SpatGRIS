#pragma once

#include <cmath>

#include <JuceHeader.h>

struct CartesianVector {
private:
    static juce::String const XML_TAG;

public:
    float x;
    float y;
    float z;

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
        auto result{ std::make_unique<juce::XmlElement>(XML_TAG) };
        result->setAttribute("x", x);
        result->setAttribute("y", y);
        result->setAttribute("z", z);
        return result.release();
    }
};