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
