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

/*
 * Code copied from Jonathan Boccara's blog.
 *
 * https://www.fluentcpp.com/2019/08/30/how-to-disable-a-warning-in-cpp/
 *
 */

/*
 * Usage:
 *
 * DISABLE_WARNING_PUSH
 *
 * DISABLE_WARNING_UNREFERENCED_FORMAL_PARAMETER
 * DISABLE_WARNING_UNREFERENCED_FUNCTION
 *
 * code where you want
 * to disable the warnings
 *
 * DISABLE_WARNING_POP
 */

#pragma once

#if defined(_MSC_VER)
    #define DISABLE_WARNING_PUSH __pragma(warning(push))
    #define DISABLE_WARNING_POP __pragma(warning(pop))
    #define DISABLE_WARNING(warningNumber) __pragma(warning(disable : warningNumber))

    #define DISABLE_WARNING_NAMELESS_STRUCT DISABLE_WARNING(4201)
    #define DISABLE_WARNING_UNREFERENCED_FUNCTION DISABLE_WARNING(4505)
// other warnings you want to deactivate...

#elif defined(__GNUC__) || defined(__clang__)
    #define DO_PRAGMA(X) _Pragma(#X)
    #define DISABLE_WARNING_PUSH DO_PRAGMA(GCC diagnostic push)
    #define DISABLE_WARNING_POP DO_PRAGMA(GCC diagnostic pop)
    #define DISABLE_WARNING(warningName) DO_PRAGMA(GCC diagnostic ignored #warningName)

    #define DISABLE_WARNING_NAMELESS_STRUCT
// clang-format off
    #define DISABLE_WARNING_UNREFERENCED_FUNCTION DISABLE_WARNING(-Wunused-function)
// clang-format on
// other warnings you want to deactivate...

#else
    #define DISABLE_WARNING_PUSH
    #define DISABLE_WARNING_POP
    #define DISABLE_WARNING_NAMELESS_STRUCT
    #define DISABLE_WARNING_UNREFERENCED_FUNCTION
// other warnings you want to deactivate...

#endif