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

#include <JuceHeader.h>

#include <type_traits>

namespace gris
{
/** jassert() seems to prevent narrow() from being constexpr on Apple Clang. */
#if defined(NDEBUG) || defined(__APPLE__)
//==============================================================================
/** Does a static_cast.
 *
 * On Windows and Linux debug builds, it also verifies that the original value is preserved. */
template<typename To, typename From>
constexpr To narrow(From const & value) noexcept
{
    return static_cast<To>(value);
}
#else
//==============================================================================
/** Does a static_cast.
 *
 * On Windows and Linux debug builds, it also verifies that the original value is preserved. */
template<typename To, typename From>
[[nodiscard]] constexpr To narrow(From const & value)
{
    static_assert(std::is_scalar_v<From> && std::is_scalar_v<To>, "narrow() can only be used with scalar types.");

    auto const result{ static_cast<To>(value) };

    if constexpr (std::is_signed_v<From> != std::is_signed_v<To>) {
        // If you hit this assertion, it means that you tried casting a negative value into an unsigned type.
        jassert(value >= 0);
        // If you hit this assertion, it means that you tried casting a positive value into a signed type that was to
        // narrow for it.
        jassert(result >= 0);
    }

    auto const sanity_check{ static_cast<From>(result) };

    // If you hit this assertion, it either means that you tried casting a value into a type that was too narrow for it
    // or that you loss precision when going to of from a floating point type.
    jassert(sanity_check == value);

    return result;
}
#endif

} // namespace gris
