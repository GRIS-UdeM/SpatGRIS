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

#include "../sg_Macros.hpp"
#include "sg_StrongFloat.hpp"

#include <JuceHeader.h>

namespace gris
{
//==============================================================================
/** Strongly-typed decibels full-scale. */
class dbfs_t final : public StrongFloat<float, dbfs_t, struct VolumeT>
{
public:
    dbfs_t() = default;
    explicit constexpr dbfs_t(type const & value) : StrongFloat(value) {}
    SG_DEFAULT_COPY_AND_MOVE(dbfs_t)
    //==============================================================================
    [[nodiscard]] type toGain() const { return juce::Decibels::decibelsToGain(mValue); }
    static dbfs_t fromGain(type const gain) { return dbfs_t{ juce::Decibels::gainToDecibels(gain) }; }
};

} // namespace gris
