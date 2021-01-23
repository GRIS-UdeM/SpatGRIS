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

#include <array>

#include "macros.h"

DISABLE_WARNINGS
#include "spat/lbap.h"
#include "spat/vbap.h"
ENABLE_WARNINGS

#include "constants.hpp"

//==============================================================================
struct SourceData {
    unsigned int id{};
    float x{};
    float y{};
    float z{};

    float radAzimuth{};
    float radElevation{};
    float azimuth{};
    float zenith{};
    float radius{ 1.0f };
    float azimuthSpan{};
    float zenithSpan{};

    std::array<float, MAX_OUTPUTS> lbapGains{};
    std::array<float, MAX_OUTPUTS> lbapY{};
    lbap_pos lbapLastPos{ -1, -1, -1, 0.0f, 0.0f, 0.0f };

    bool isMuted = false;
    bool isSolo = false;
    float gain{}; // Not used yet.

    int directOut{};

    VBAP_DATA * paramVBap{};
};
