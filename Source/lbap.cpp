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

#include "lbap.hpp"

#include <JuceHeader.h>

#include "AudioStructs.hpp"
#include "constants.hpp"
#include "narrow.hpp"

/* =================================================================================
Utility functions.
================================================================================= */

//==============================================================================
/* Bilinear interpolation to retrieve the value at position (x, y) in a 2D matrix. */
static float bilinearInterpolation(matrix_t const & matrix, float const x, float const y)
{
    auto const xi = static_cast<int>(x);
    auto const yi = static_cast<int>(y);
    auto const xf = narrow<float>(x) - xi;
    auto const yf = narrow<float>(y) - yi;
    auto const v1 = matrix[xi][yi];
    auto const v2 = matrix[xi + 1][yi];
    auto const v3 = matrix[xi][yi + 1];
    auto const v4 = matrix[xi + 1][yi + 1];
    auto const xv1 = v1 + (v2 - v1) * xf;
    auto const xv2 = v3 + (v4 - v3) * xf;
    return xv1 + (xv2 - xv1) * yf;
}

//==============================================================================
/* Checks if an elevation is less distant than +/- 5 degrees of a base elevation. */
static bool isPracticallySameElevation(radians_t const baseElevation, radians_t const elevation)
{
    static constexpr radians_t TOLERANCE{ degrees_t{ 5.0f } };
    return elevation > (baseElevation - TOLERANCE) && elevation < (baseElevation + TOLERANCE);
}

//==============================================================================
/* Returns the average elevation from a list of speakers. */
static radians_t averageSpeakerElevation(LbapSpeaker const * speakers, size_t const num)
{
    static auto const ACCUMULATE_ELEVATION
        = [](radians_t const total, LbapSpeaker const & speaker) { return total + speaker.vector.elevation; };

    auto const sum{ std::reduce(speakers, speakers + num, radians_t{}, ACCUMULATE_ELEVATION) };
    return sum / narrow<float>(num);
}

/* =================================================================================
lbap_pos utility functions.
================================================================================= */

//==============================================================================
/* Returns a vector lbap_pos created from an array of lbap_speaker.
 */
static std::vector<LbapPosition> lbapPositionsFromSpeakers(LbapSpeaker const * speakers, size_t const num)
{
    std::vector<LbapPosition> positions{};
    positions.reserve(num);
    std::transform(speakers,
                   speakers + num,
                   std::back_inserter(positions),
                   [](LbapSpeaker const & speaker) -> LbapPosition {
                       LbapPosition result{};
                       result.vector = speaker.vector;
                       result.position = speaker.vector.toCartesian();
                       return result;
                   });
    return positions;
}

/* =================================================================================
lbap_layer utility functions.
================================================================================= */

//==============================================================================
/* Initialize a newly created layer for `num` speakers. */
static lbap_layer initLayers(int const layerId, radians_t const elevation, std::vector<LbapPosition> const & speakers)
{
    lbap_layer result{};

    result.id = layerId;
    result.elevation = elevation;
    result.gainExponent = std::max(narrow<float>(speakers.size()) / 4.0f, 1.0f);

    result.speakers.reserve(speakers.size());
    std::transform(speakers.cbegin(),
                   speakers.cend(),
                   std::back_inserter(result.speakers),
                   [](LbapPosition const & speaker) { return speaker; });

    result.amplitudeMatrix.reserve(speakers.size());
    static constexpr matrix_t EMPTY_MATRIX{};
    std::fill_n(std::back_inserter(result.amplitudeMatrix), speakers.size(), EMPTY_MATRIX);

    return result;
}

//==============================================================================
/* Pre-compute the matrices of amplitude for the layer's speakers. */
static void computeMatrix(lbap_layer & layer)
{
    static auto constexpr H_SIZE = LBAP_MATRIX_SIZE / 2;

    for (size_t i{}; i < layer.speakers.size(); ++i) {
        auto const px = layer.speakers[i].position.x * H_SIZE + H_SIZE;
        auto const py = layer.speakers[i].position.y * H_SIZE + H_SIZE;
        for (size_t x{}; x < LBAP_MATRIX_SIZE; ++x) {
            for (size_t y{}; y < LBAP_MATRIX_SIZE; ++y) {
                auto dist = std::sqrt(std::pow(x - px, 2.0f) + std::pow(y - py, 2.0f));
                dist /= LBAP_MATRIX_SIZE;
                dist = std::clamp(dist, 0.0f, 1.0f);
                layer.amplitudeMatrix[i][x][y] = 1.0f - dist;
            }
            layer.amplitudeMatrix[i][x][LBAP_MATRIX_SIZE] = layer.amplitudeMatrix[i][x][0];
        }
        layer.amplitudeMatrix[i][LBAP_MATRIX_SIZE] = layer.amplitudeMatrix[i][0];
    }
}

//==============================================================================
/* Create a new layer, based on a lbap_pos array, and add it the to field.*/
static lbap_layer
    createLayer(LbapField const & field, radians_t const elevation, std::vector<LbapPosition> const & speakers)
{
    auto result{ initLayers(narrow<int>(field.layers.size()), elevation, speakers) };
    computeMatrix(result);
    return result;
}

//==============================================================================
/* Compute the gain of each layer's speakers, for the given position, and store
 * the result in the `gains` array.
 */
static void computeGains(lbap_layer const & layer, SourceData const & source, float * gains)
{
    static constexpr auto H_SIZE = LBAP_MATRIX_SIZE / 2.0f;
    static constexpr auto SIZE_MINUS_ONE = LBAP_MATRIX_SIZE - 1.0f;

    jassert(source.position && source.vector);

    auto const exponent = layer.gainExponent * (1.0f - source.azimuthSpan) * 2.0f;
    auto x = source.position->x * (H_SIZE - 1.0f) + H_SIZE;
    auto y = source.position->y * (H_SIZE - 1.0f) + H_SIZE;
    x = std::clamp(x, 0.0f, SIZE_MINUS_ONE);
    y = std::clamp(y, 0.0f, SIZE_MINUS_ONE);

    jassert(layer.speakers.size() == layer.amplitudeMatrix.size());
    std::transform(
        layer.amplitudeMatrix.cbegin(),
        layer.amplitudeMatrix.cend(),
        gains,
        [x, y, exponent](matrix_t const & matrix) { return std::pow(bilinearInterpolation(matrix, x, y), exponent); });
    auto const sum{ std::reduce(gains, gains + layer.speakers.size(), 0.0f, std::plus()) };

    if (sum > 0.0f) {
        // (pow(3.0, (1.0 - rad))) for energy spreading when moving toward the center.
        auto const comp = source.vector->length < 1.0f ? std::pow(3.0f, (1.0f - source.vector->length)) : 1.0f;
        // normalization (1.0 / sum) and compensation
        auto const norm = 1.0f / sum * comp;
        std::transform(gains, gains + layer.speakers.size(), gains, [norm](float & gain) { return gain * norm; });
        // TODO : should this be done again?
        /*for (size_t i{}; i < layer.speakers.size(); ++i) {
            gains.data()[i] *= norm;
        }*/
    }
}

/* =================================================================================
====================================================================================
Layer-Based Amplitude Panning interface implementation.
====================================================================================
================================================================================= */

//==============================================================================
LbapField lbapInit(SpeakersData const & speakers)
{
    std::vector<LbapSpeaker> lbapSpeakers{};
    lbapSpeakers.reserve(speakers.size());

    for (auto const & speaker : speakers) {
        if (speaker.value->isDirectOutOnly) {
            continue;
        }

        LbapSpeaker const newSpeaker{ speaker.value->vector, speaker.key };
        lbapSpeakers.push_back(newSpeaker);
    }

    std::sort(lbapSpeakers.begin(), lbapSpeakers.end(), [](LbapSpeaker const & a, LbapSpeaker const & b) -> bool {
        return a.vector.elevation < b.vector.elevation;
    });

    LbapField field{};
    std::transform(lbapSpeakers.cbegin(),
                   lbapSpeakers.cend(),
                   std::back_inserter(field.outputOrder),
                   [](LbapSpeaker const & speaker) { return speaker.outputPatch; });

    size_t count{};
    while (count < lbapSpeakers.size()) {
        auto const start = count;
        auto const elevation = lbapSpeakers[count++].vector.elevation;
        while (count < lbapSpeakers.size()
               && isPracticallySameElevation(elevation, lbapSpeakers[count].vector.elevation)) {
            ++count;
        }
        auto const howMany{ count - start };
        auto const spk{ lbapPositionsFromSpeakers(&lbapSpeakers[start], howMany) };
        auto const mean{ averageSpeakerElevation(&lbapSpeakers[start], howMany) };
        field.layers.emplace_back(createLayer(field, mean, spk));
    }

    return field;
}

//==============================================================================
void lbap(SourceData const & source, SpeakersSpatGains & gains, LbapField const & field)
{
    jassert(source.vector);
    auto const & position{ *source.vector };

    auto const * firstLayer = &field.layers.front();
    auto const * secondLayer = &field.layers.back();
    for (size_t i{}; i < field.layers.size(); ++i) {
        if (field.layers[i].elevation > position.elevation) {
            secondLayer = &field.layers[i];
            break;
        }
        firstLayer = &field.layers[i];
    }

    auto const getRatio = [&]() {
        if (firstLayer->id != narrow<int>(field.layers.size() - 1)) {
            return (position.elevation - firstLayer->elevation) / (secondLayer->elevation - firstLayer->elevation);
        }
        return 0.0f;
    };

    auto const ratio{ getRatio() };

    auto const elevationSpan{ source.zenithSpan == 0.0f ? 0.0f : std::pow(source.zenithSpan, 4.0f) * 6.0f };

    size_t c{};
    float gain;
    std::array<float, MAX_NUM_SPEAKERS> tempGains{};
    for (int i{}; i < narrow<int>(field.layers.size()); ++i) {
        if (i < firstLayer->id) {
            gain = elevationSpan / narrow<float>((firstLayer->id - i) * 2);
        } else if (i == firstLayer->id) {
            gain = 1.0f - ratio + elevationSpan;
        } else if (i == secondLayer->id && firstLayer->id != secondLayer->id) {
            gain = ratio + elevationSpan;
        } else if (i == secondLayer->id && firstLayer->id == secondLayer->id) {
            gain = elevationSpan;
        } else {
            gain = elevationSpan / narrow<float>((i - secondLayer->id) * 2);
        }
        gain = std::min(gain, 1.0f);
        computeGains(field.layers[i], source, tempGains.data() + c);
        std::transform(tempGains.cbegin() + c,
                       tempGains.cbegin() + c + field.layers[i].speakers.size(),
                       tempGains.begin() + c,
                       [gain](float const x) { return x * gain; });
        c += field.layers[i].speakers.size();
    }

    for (size_t i{}; i < field.getNumSpeakers(); ++i) {
        auto const outputPatch{ field.outputOrder[i] };
        gains[outputPatch] = tempGains[i];
    }
}
