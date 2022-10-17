/*
 This file is part of SpatGRIS.

 Developers: Gaël Lane Lépine

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

namespace gris
{
//==============================================================================
/** Maps a numeric range to another. */
template<typename Val>
constexpr Val
    remap(Val const & value, Val const & inMin, Val const & inMax, Val const & outMin, Val const & outMax) noexcept
{
    return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

} // namespace gris