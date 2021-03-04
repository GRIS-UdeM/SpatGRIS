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
};

class dbfs_t final : public StrongFloat<float, dbfs_t, struct VolumeT>
{
public:
    dbfs_t() = default;
    explicit constexpr dbfs_t(type const & value) : StrongFloat(value) {}

    [[nodiscard]] type toGain() const { return juce::Decibels::decibelsToGain(mValue); }
    static dbfs_t fromGain(type const gain) { return dbfs_t{ juce::Decibels::gainToDecibels(gain) }; }
};