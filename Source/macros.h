/*
 This file is part of SpatGRIS2.

 Developers: Samuel Béland, Olivier Bélanger, Nicolas Masson

 SpatGRIS2 is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 SpatGRIS2 is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with SpatGRIS2.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

/* Code inspired by Jonathan Boccara's blog entry.
 *
 * https://www.fluentcpp.com/2019/08/30/how-to-disable-a-warning-in-cpp/
 */

#if defined(_MSC_VER)
    #define DISABLE_WARNINGS __pragma(warning(push, 0))
    #define ENABLE_WARNINGS __pragma(warning(pop))
#elif defined(__clang__)
    #define DISABLE_WARNINGS
    #define ENABLE_WARNINGS
#elif defined(__GNUC__)
    #define DO_PRAGMA(X) _Pragma(#X)
    #define DISABLE_WARNINGS                                                                                           \
        DO_PRAGMA(GCC diagnostic push)                                                                                 \
        DO_PRAGMA(GCC diagnostic "-w")
    #define ENABLE_WARNINGS DO_PRAGMA(GCC diagnostic pop)

#else
static_assert(false, "What compiler are you using?");
#endif