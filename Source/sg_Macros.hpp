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

#define SG_DELETE_COPY(x)                                                                                              \
    x(x const &) = delete;                                                                                             \
    x & operator=(x const &) = delete;

#define SG_DELETE_MOVE(x)                                                                                              \
    x(x &&) = delete;                                                                                                  \
    x & operator=(x &&) = delete;

#define SG_DELETE_COPY_AND_MOVE(x) SG_DELETE_COPY(x) SG_DELETE_MOVE(x)

#define SG_DEFAULT_COPY(x)                                                                                             \
    x(x const &) = default;                                                                                            \
    x & operator=(x const &) = default;

#define SG_DEFAULT_MOVE(x)                                                                                             \
    x(x &&) = default;                                                                                                 \
    x & operator=(x &&) = default;

#define SG_DEFAULT_COPY_AND_MOVE(x) SG_DEFAULT_COPY(x) SG_DEFAULT_MOVE(x)
