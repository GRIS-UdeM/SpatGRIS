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

#include <cassert>
#include <type_traits>

#ifdef NDEBUG
template<typename To, typename From>
constexpr To narrow(From const value)
{
    return static_cast<To>(value);
}
#else
template<typename To, typename From>
constexpr To narrow(From const value)
{
    static_assert(std::is_scalar_v<To> && std::is_scalar_v<From>);

    // If you hit this assertion, it means that you tried to cast a negative value into an unsigned type.
    assert(std::is_signed_v<To> == std::is_signed_v<From> || value >= From{ 0 });

    auto const expanded_value{ static_cast<To>(value) };
    auto const sanity_check{ static_cast<From>(expanded_value) };

    // If you hit this assertion, it means that you tried to
    assert(sanity_check == value);

    return expanded_value;
}
#endif
