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

#pragma once

#include "macros.h"
DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

#include <cstdint>

template<typename T, typename Dummy>
class StrongIndex
{
    T mValue;

    static_assert(std::is_integral_v<T>, "Underlying types should be integrals.");

public:
    using type = T;

    StrongIndex() = default;
    explicit constexpr StrongIndex(type const & value) : mValue(value) {}

    [[nodiscard]] constexpr bool operator==(StrongIndex const & other) const { return mValue == other.mValue; }
    [[nodiscard]] constexpr bool operator!=(StrongIndex const & other) const { return mValue != other.mValue; }
    [[nodiscard]] constexpr bool operator<(StrongIndex const & other) const { return mValue < other.mValue; }
    [[nodiscard]] constexpr bool operator>(StrongIndex const & other) const { return mValue > other.mValue; }
    [[nodiscard]] constexpr bool operator<=(StrongIndex const & other) const { return mValue <= other.mValue; }
    [[nodiscard]] constexpr bool operator>=(StrongIndex const & other) const { return mValue >= other.mValue; }

    [[nodiscard]] constexpr type const & get() const { return mValue; }
    StrongIndex & operator++()
    {
        ++mValue;
        return *this;
    }
    StrongIndex operator++(int) { return StrongIndex{ mValue++ }; }
    StrongIndex & operator--()
    {
        --mValue;
        return *this;
    }
};
using speaker_id_t = StrongIndex<int, struct SpeakerIdT>;
using port_id_t = StrongIndex<uint32_t, struct PortIdT>;
using output_patch_t = StrongIndex<int, struct OutputPatchT>;

template<typename T, typename Derived, typename Dummy>
class StrongFloat
{
protected:
    T mValue;

    static_assert(std::is_floating_point_v<T>, "Underlying types should be floating points.");
    // static_assert(std::is_base_of_v<StrongFloat, Derived>, "This is a CRTP type.");
public:
    using type = T;

    StrongFloat() = default;
    explicit constexpr StrongFloat(T const & value) : mValue(value) {}

    [[nodiscard]] constexpr bool operator==(Derived const & other) const { return mValue == other.mValue; }
    [[nodiscard]] constexpr bool operator!=(Derived const & other) const { return mValue != other.mValue; }
    [[nodiscard]] constexpr bool operator<(Derived const & other) const { return mValue < other.mValue; }
    [[nodiscard]] constexpr bool operator>(Derived const & other) const { return mValue > other.mValue; }
    [[nodiscard]] constexpr bool operator<=(Derived const & other) const { return mValue <= other.mValue; }
    [[nodiscard]] constexpr bool operator>=(Derived const & other) const { return mValue >= other.mValue; }

    [[nodiscard]] constexpr type const & get() const { return mValue; }

    [[nodiscard]] constexpr Derived operator+(Derived const & other) const { return Derived{ mValue + other.mValue }; }
    [[nodiscard]] constexpr Derived operator-(Derived const & other) const { return Derived{ mValue - other.mValue }; }
    [[nodiscard]] constexpr Derived operator*(type const mod) const { return Derived{ mValue * mod }; }
    [[nodiscard]] constexpr Derived operator/(type const mod) const { return Derived{ mValue / mod }; }
    [[nodiscard]] constexpr type operator/(Derived const & other) const { return mValue / other.mValue; }

protected:
    [[nodiscard]] constexpr Derived centeredAroundZero(type const amplitude) const noexcept
    {
        auto const halfAmplitude{ amplitude / static_cast<type>(2) };
        auto const isTooSmall = [min = -halfAmplitude](type const value) -> bool { return value < min; };

        if (isTooSmall(mValue)) {
            auto newValue{ mValue + amplitude };
            while (isTooSmall(newValue)) {
                newValue += amplitude;
            }
            return Derived{ newValue };
        }

        auto const isTooHigh = [max = halfAmplitude](type const value) -> bool { return value >= max; };

        if (isTooHigh(mValue)) {
            auto newValue{ mValue - amplitude };
            while (isTooHigh(newValue)) {
                newValue -= amplitude;
            }
            return Derived{ newValue };
        }

        return *static_cast<Derived const *>(this);
    }
};

class dbfs_t final : public StrongFloat<float, dbfs_t, struct VolumeT>
{
public:
    dbfs_t() = default;
    explicit constexpr dbfs_t(type const & value) : StrongFloat(value) {}

    [[nodiscard]] type toGain() const { return juce::Decibels::decibelsToGain(mValue); }
    static dbfs_t fromGain(type const gain) { return dbfs_t{ juce::Decibels::gainToDecibels(gain) }; }
};

class degrees_t final : public StrongFloat<float, degrees_t, struct DegreesT>
{
public:
    static constexpr type DEGREE_PER_RADIAN{ static_cast<type>(360) / juce::MathConstants<type>::twoPi };

    degrees_t() = default;
    explicit constexpr degrees_t(type const & value) : StrongFloat(value) {}

    [[nodiscard]] constexpr degrees_t centered() const noexcept { return centeredAroundZero(static_cast<type>(360)); }
};

class radians_t final : public StrongFloat<float, radians_t, struct RadiansT>
{
public:
    static constexpr type RADIAN_PER_DEGREE{ juce::MathConstants<type>::twoPi / static_cast<type>(360) };

    radians_t() = default;
    explicit constexpr radians_t(type const & value) : StrongFloat(value) {}
    constexpr radians_t(degrees_t const & degrees) : StrongFloat(degrees.get() * RADIAN_PER_DEGREE) {}

    [[nodiscard]] constexpr radians_t centered() const noexcept
    {
        return centeredAroundZero(juce::MathConstants<type>::twoPi);
    }
    [[nodiscard]] constexpr degrees_t toDegrees() const noexcept
    {
        return degrees_t{ mValue * degrees_t::DEGREE_PER_RADIAN };
    }
};