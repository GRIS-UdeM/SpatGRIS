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

#include "../sg_Narrow.hpp"

namespace gris
{
//==============================================================================
/** Used for validating template parameters using std::is_base_of_v<>. */
class StrongIndexBase
{
};

//==============================================================================
/** CRTP base for an integer-based strong type. */
template<typename T, typename Dummy, T StartsAt>
class StrongIndex : public StrongIndexBase
{
    T mValue;

    static_assert(std::is_integral_v<T>, "Underlying types should be integrals.");

public:
    //==============================================================================
    using type = T;
    static constexpr auto OFFSET = StartsAt;
    //==============================================================================
    StrongIndex() = default;
    explicit constexpr StrongIndex(type const & value) : mValue(value) {}
    //==============================================================================
    [[nodiscard]] constexpr bool operator==(StrongIndex const & other) const { return mValue == other.mValue; }
    [[nodiscard]] constexpr bool operator!=(StrongIndex const & other) const { return mValue != other.mValue; }
    [[nodiscard]] constexpr bool operator<(StrongIndex const & other) const { return mValue < other.mValue; }
    [[nodiscard]] constexpr bool operator>(StrongIndex const & other) const { return mValue > other.mValue; }
    [[nodiscard]] constexpr bool operator<=(StrongIndex const & other) const { return mValue <= other.mValue; }
    [[nodiscard]] constexpr bool operator>=(StrongIndex const & other) const { return mValue >= other.mValue; }
    //==============================================================================
    [[nodiscard]] constexpr type const & get() const { return mValue; }
    //==============================================================================
    StrongIndex & operator++();
    StrongIndex operator++(int);
    StrongIndex & operator--();
    //==============================================================================
    template<typename TargetType>
    [[nodiscard]] TargetType removeOffset() const;
};

//==============================================================================
template<typename T, typename Dummy, T StartsAt>
StrongIndex<T, Dummy, StartsAt> & StrongIndex<T, Dummy, StartsAt>::operator++()
{
    ++mValue;
    return *this;
}

//==============================================================================
template<typename T, typename Dummy, T StartsAt>
StrongIndex<T, Dummy, StartsAt> StrongIndex<T, Dummy, StartsAt>::operator++(int)
{
    return StrongIndex{ mValue++ };
}

//==============================================================================
template<typename T, typename Dummy, T StartsAt>
StrongIndex<T, Dummy, StartsAt> & StrongIndex<T, Dummy, StartsAt>::operator--()
{
    --mValue;
    return *this;
}

//==============================================================================
template<typename T, typename Dummy, T StartsAt>
template<typename TargetType>
TargetType StrongIndex<T, Dummy, StartsAt>::removeOffset() const
{
    return narrow<TargetType>(mValue - OFFSET);
}

} // namespace gris
