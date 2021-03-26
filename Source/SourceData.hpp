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

#include <array>

#include "lib/tl/optional.hpp"

#include "constants.hpp"
#include "lbap.hpp"
#include "vbap.hpp"

struct AttenuationData {
    float lastGain{};
    float lastCoefficient{};
    float lowpassY{};
    float lowpassZ{};
};

//==============================================================================
struct SourceData {
    unsigned int id{};

    degrees_t azimuth{};
    degrees_t zenith{};
    float radius{ 1.0f };

    radians_t radAzimuth{};
    radians_t radElevation{};

    float azimuthSpan{};
    float zenithSpan{};

    float x{};
    float y{};
    float z{};

    std::array<float, MAX_OUTPUTS> lbapGains{};
    std::array<float, MAX_OUTPUTS> lbapY{};
    lbap_pos lbapLastPos{ radians_t{ -1.0f }, radians_t{ -1.0f }, -1, 0.0f, 0.0f, 0.0f };

    bool isMuted = false;
    bool isSolo = false;
    float gain{}; // Not used yet.
    float magnitude{};

    tl::optional<output_patch_t> directOut{};

    VbapData * paramVBap{};
    bool shouldUpdateVbap{};
    AttenuationData attenuationData{};
};
