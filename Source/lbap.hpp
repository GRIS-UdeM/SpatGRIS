/**
 *  Layer-Based Amplitude Panning framework.
 *
 * LBAP (Layer-Based Amplitude Panning) is a framework written in C
 * to do 2-D or 3-D sound spatialization. It uses a pre-computed
 * gain matrices to perform the spatialization of the sources very
 * efficiently.
 *
 * author : Olivier Belanger, 2018
 *
 * Modified by Samuel Béland, 2021
 */

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

#include "CartesianVector.hpp"
#include "LogicStrucs.hpp"
#include "OwnedMap.hpp"
#include "PolarVector.hpp"
#include "StrongTypes.hpp"

struct SpeakerData;

static auto constexpr LBAP_MATRIX_SIZE = 64;
using matrix_t = std::array<std::array<float, LBAP_MATRIX_SIZE + 1>, LBAP_MATRIX_SIZE + 1>;

/** \brief A structure containing coordinates of a speaker in the field.
 *
 * This structure is used to hold informations about a given speaker in
 * the field. It is the responsibility of the user to provide angular
 * coordinates (`azi`, `ele` and `rad`) values. parameters `azi` (-pi .. pi)
 * and `ele` (0 .. pi/2) must be given in radians and parameter `rad`
 * (the length of the vector) is in the range 0 to 1.
 *
 * The order of the physical outputs may not match the order in which the
 * field will process the speakers. The `spkid` is an integer (starting
 * at 0) used by the field to properly order the output signals.
 */
struct LbapSpeaker {
    CartesianVector position{};
    output_patch_t outputPatch{};
};

//==============================================================================
using matrix_t = std::array<std::array<float, LBAP_MATRIX_SIZE + 1>, LBAP_MATRIX_SIZE + 1>;

//==============================================================================
struct LbapLayer {
    int id;                                        /**< Layer id. */
    float height;                                  /**< Elevation of the layer in the range 0 .. 1. */
    float gainExponent;                            /**< Speaker gain exponent for 4+ speakers. */
    std::vector<matrix_t> amplitudeMatrix;         /**< Arrays of amplitude values [spk][x][y]. */
    std::vector<CartesianVector> speakerPositions; /**< Array of speakers. */
};

//==============================================================================
struct LbapField {
    std::vector<output_patch_t> outputOrder; /**< Physical output order. */
    std::vector<LbapLayer> layers;           /**< Array of layers. */
    //==============================================================================
    [[nodiscard]] size_t getNumSpeakers() const;
    void reset();
};

/** \brief Creates the field's layers according to the position of speakers.
 *
 * This function creates the field's layer according to the position of
 * speakers given as `speakers` argument. The argument `num` is the number of
 * speakers passed to the function.
 */
LbapField lbapInit(SpeakersData const & speakers);

/** \brief Calculates the gain of the outputs for a source's position.
 *
 * This function uses the position `pos` to retrieve the gain for every
 * output from the field's layers and fill the array of float `gains`.
 * The user must provide the array of float and is responsible of its
 * memory. This array can be passed to the audio processing function
 * to control the gain of the signal outputs.
 */
void lbap(SourceData const & source, SpeakersSpatGains & gains, LbapField const & field);
