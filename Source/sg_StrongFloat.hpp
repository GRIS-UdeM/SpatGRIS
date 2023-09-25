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

#include <cmath>
#include <type_traits>

namespace gris
{
//==============================================================================
/** Used for validating template parameters using std::is_base_of_v<>. */
class StrongFloatBase
{
};

//==============================================================================
/** CRPT base for a float-based strong type. */
template<typename T, typename Derived, typename Dummy>
class StrongFloat : public StrongFloatBase
{
    static_assert(std::is_floating_point_v<T>, "Underlying types should be floating points.");
    static_assert(std::is_floating_point_v<T>);

protected:
    //==============================================================================
    T mValue;

public:
    //==============================================================================
    using type = T;
    //==============================================================================
    StrongFloat() = default;
    explicit constexpr StrongFloat(T const & value) : mValue(value) {}
    //==============================================================================
    [[nodiscard]] constexpr bool operator==(Derived const & other) const { return mValue == other.mValue; }
    [[nodiscard]] constexpr bool operator!=(Derived const & other) const { return mValue != other.mValue; }
    [[nodiscard]] constexpr bool operator<(Derived const & other) const { return mValue < other.mValue; }
    [[nodiscard]] constexpr bool operator>(Derived const & other) const { return mValue > other.mValue; }
    [[nodiscard]] constexpr bool operator<=(Derived const & other) const { return mValue <= other.mValue; }
    [[nodiscard]] constexpr bool operator>=(Derived const & other) const { return mValue >= other.mValue; }
    //==============================================================================
    [[nodiscard]] constexpr type const & get() const { return mValue; }
    //==============================================================================
    [[nodiscard]] constexpr Derived operator-() const { return Derived{ -mValue }; }
    [[nodiscard]] constexpr Derived operator+(Derived const & other) const { return Derived{ mValue + other.mValue }; }
    [[nodiscard]] constexpr Derived operator-(Derived const & other) const { return Derived{ mValue - other.mValue }; }
    [[nodiscard]] constexpr Derived operator*(type const mod) const { return Derived{ mValue * mod }; }
    [[nodiscard]] constexpr Derived operator/(type const mod) const { return Derived{ mValue / mod }; }
    [[nodiscard]] constexpr type operator/(Derived const & other) const { return mValue / other.mValue; }
    //==============================================================================
    Derived & operator+=(Derived const & other) noexcept;
    Derived & operator*=(type const & mod) noexcept;
    Derived & operator/=(type const & mod) noexcept;
    //==============================================================================
    [[nodiscard]] constexpr Derived abs() const noexcept;

protected:
    //==============================================================================
    [[nodiscard]] constexpr Derived centeredAroundZero(type const amplitude) const noexcept;
};

//==============================================================================
template<typename T, typename Derived, typename Dummy>
constexpr Derived StrongFloat<T, Derived, Dummy>::abs() const noexcept
{
    return Derived{ std::abs(mValue) };
}

//==============================================================================
template<typename T, typename Derived, typename Dummy>
constexpr Derived StrongFloat<T, Derived, Dummy>::centeredAroundZero(type const amplitude) const noexcept
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

//==============================================================================
template<typename T, typename Derived, typename Dummy>
Derived & StrongFloat<T, Derived, Dummy>::operator+=(Derived const & other) noexcept
{
    mValue += other.mValue;
    return *static_cast<Derived *>(this);
}

//==============================================================================
template<typename T, typename Derived, typename Dummy>
Derived & StrongFloat<T, Derived, Dummy>::operator*=(type const & mod) noexcept
{
    mValue *= mod;
    return *static_cast<Derived *>(this);
}

//==============================================================================
template<typename T, typename Derived, typename Dummy>
Derived & StrongFloat<T, Derived, Dummy>::operator/=(type const & mod) noexcept
{
    jassert(mod != 0);
    mValue /= mod;
    return *static_cast<Derived *>(this);
}

} // namespace gris
