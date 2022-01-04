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

#include "sg_lbap.hpp"

#include "sg_AudioStructs.hpp"
#include "sg_Narrow.hpp"
#include "sg_constants.hpp"

namespace gris
{
//==============================================================================
/* Bilinear interpolation to retrieve the value at position (x, y) in a 2D matrix. */
static float bilinearInterpolation(matrix_t const & matrix, float const x, float const y)
{
    jassert(x >= 0.0f && y >= 0.0f);
    auto const xi = static_cast<std::size_t>(x);
    auto const yi = static_cast<std::size_t>(y);
    auto const xf = narrow<float>(x) - narrow<float>(xi);
    auto const yf = narrow<float>(y) - narrow<float>(yi);
    auto const v1 = matrix[xi][yi];
    auto const v2 = matrix[xi + 1][yi];
    auto const v3 = matrix[xi][yi + 1];
    auto const v4 = matrix[xi + 1][yi + 1];
    auto const xv1 = v1 + (v2 - v1) * xf;
    auto const xv2 = v3 + (v4 - v3) * xf;
    return xv1 + (xv2 - xv1) * yf;
}

//==============================================================================
/* Returns a vector lbap_pos created from an array of lbap_speaker.
 */
static std::vector<Position> lbapPositionsFromSpeakers(LbapSpeaker const * speakers, size_t const num)
{
    std::vector<Position> positions{};
    positions.reserve(num);
    std::transform(speakers, speakers + num, std::back_inserter(positions), [](LbapSpeaker const & speaker) {
        return speaker.position;
    });
    return positions;
}

//==============================================================================
/* Initialize a newly created layer for `num` speakers. */
static LbapLayer initLayers(int const layerId, float const height, std::vector<Position> speakers)
{
    LbapLayer layer{};

    layer.id = layerId;
    layer.height = height;
    layer.gainExponent = std::max(narrow<float>(speakers.size()) / 4.0f, 1.0f);
    layer.amplitudeMatrix.reserve(speakers.size());
    static constexpr matrix_t EMPTY_MATRIX{};
    std::fill_n(std::back_inserter(layer.amplitudeMatrix), speakers.size(), EMPTY_MATRIX);
    layer.speakerPositions = std::move(speakers);

    return layer;
}

//==============================================================================
/* Pre-compute the matrices of amplitude for the layer's speakers. */
static void computeMatrix(LbapLayer & layer)
{
    static auto constexpr H_SIZE = LBAP_MATRIX_SIZE / 2;

    for (size_t i{}; i < layer.speakerPositions.size(); ++i) {
        auto const px = layer.speakerPositions[i].getCartesian().x * H_SIZE + H_SIZE;
        auto const py = layer.speakerPositions[i].getCartesian().y * H_SIZE + H_SIZE;
        for (size_t x{}; x < LBAP_MATRIX_SIZE; ++x) {
            for (size_t y{}; y < LBAP_MATRIX_SIZE; ++y) {
                auto dist = std::sqrt(std::pow(narrow<float>(x) - px, 2.0f) + std::pow(narrow<float>(y) - py, 2.0f));
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
static LbapLayer createLayer(LbapField const & field, float const height, std::vector<Position> speakers)
{
    auto result{ initLayers(narrow<int>(field.layers.size()), height, std::move(speakers)) };
    computeMatrix(result);
    return result;
}

//==============================================================================
/* Compute the gain of each layer's speakers, for the given position, and store
 * the result in the `gains` array.
 */
static void computeGains(LbapLayer const & layer, SourceData const & source, float * gains)
{
    static constexpr auto H_SIZE = LBAP_MATRIX_SIZE / 2.0f;
    static constexpr auto SIZE_MINUS_ONE = LBAP_MATRIX_SIZE - 1.0f;

    jassert(source.position);

    auto const exponent = layer.gainExponent * (1.0f - source.azimuthSpan) * 2.0f;
    auto x = source.position->getCartesian().x * (H_SIZE - 1.0f) + H_SIZE;
    auto y = source.position->getCartesian().y * (H_SIZE - 1.0f) + H_SIZE;
    x = std::clamp(x, 0.0f, SIZE_MINUS_ONE);
    y = std::clamp(y, 0.0f, SIZE_MINUS_ONE);

    jassert(layer.speakerPositions.size() == layer.amplitudeMatrix.size());
    std::transform(
        layer.amplitudeMatrix.cbegin(),
        layer.amplitudeMatrix.cend(),
        gains,
        [x, y, exponent](matrix_t const & matrix) { return std::pow(bilinearInterpolation(matrix, x, y), exponent); });
    auto const sum{ std::reduce(gains, gains + layer.speakerPositions.size(), 0.0f, std::plus()) };

    if (sum > 0.0f) {
        // (pow(3.0, (1.0 - rad))) for energy spreading when moving toward the center.
        auto const flatRadius{ source.position->getCartesian().discardZ().getDistanceFromOrigin() };
        auto const comp = flatRadius < 1.0f ? std::pow(3.0f, 1.0f - flatRadius) : 1.0f;
        // normalization (1.0 / sum) and compensation
        auto const norm = 1.0f / sum * comp;
        std::transform(gains, gains + layer.speakerPositions.size(), gains, [norm](float & gain) {
            return gain * norm;
        });
    }
}

//==============================================================================
size_t LbapField::getNumSpeakers() const
{
    return std::accumulate(layers.cbegin(), layers.cend(), size_t{}, [](size_t const sum, LbapLayer const & layer) {
        return sum + layer.speakerPositions.size();
    });
}

//==============================================================================
void LbapField::reset()
{
    outputOrder.clear();
    layers.clear();
}

//==============================================================================
LbapField lbapInit(SpeakersData const & speakers)
{
    std::vector<LbapSpeaker> lbapSpeakers{};
    lbapSpeakers.reserve(narrow<std::size_t>(speakers.size()));

    for (auto const & speaker : speakers) {
        if (speaker.value->isDirectOutOnly) {
            continue;
        }

        LbapSpeaker const newSpeaker{ speaker.value->position, speaker.key };
        lbapSpeakers.push_back(newSpeaker);
    }

    std::sort(lbapSpeakers.begin(), lbapSpeakers.end(), [](LbapSpeaker const & a, LbapSpeaker const & b) -> bool {
        return a.position.getCartesian().z < b.position.getCartesian().z;
    });

    LbapField field{};
    std::transform(lbapSpeakers.cbegin(),
                   lbapSpeakers.cend(),
                   std::back_inserter(field.outputOrder),
                   [](LbapSpeaker const & speaker) { return speaker.outputPatch; });

    size_t count{};
    while (count < lbapSpeakers.size()) {
        auto const start{ count };
        auto const height{ lbapSpeakers[count++].position.getCartesian().z };
        static constexpr auto TOLERANCE{ 0.055f };
        while (count < lbapSpeakers.size()
               && std::abs(height - lbapSpeakers[count].position.getCartesian().z) < TOLERANCE) {
            ++count;
        }
        auto const howMany{ count - start };
        auto const spk{ lbapPositionsFromSpeakers(&lbapSpeakers[start], howMany) };

        auto const heightSum{ std::accumulate(
            spk.cbegin(),
            spk.cend(),
            0.0f,
            [](float const total, Position const & position) { return total + position.getCartesian().z; }) };

        auto const mean{ heightSum / narrow<float>(howMany) };
        field.layers.emplace_back(createLayer(field, mean, spk));
    }

    return field;
}

//==============================================================================
void lbap(SourceData const & source, SpeakersSpatGains & gains, LbapField const & field)
{
    jassert(source.position);
    auto const & position{ *source.position };

    auto const * firstLayer{ &field.layers.front() };
    auto const * secondLayer{ &field.layers.back() };
    for (auto const & layer : field.layers) {
        if (layer.height >= position.getCartesian().z) {
            secondLayer = &layer;
            break;
        }
        firstLayer = &layer;
    }

    auto const getRatio = [&]() {
        if (firstLayer == secondLayer) {
            return 0.0f;
        }

        jassert(secondLayer->height - firstLayer->height != 0.0f);
        return (position.getCartesian().z - firstLayer->height) / (secondLayer->height - firstLayer->height);
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
        } else if (i == secondLayer->id && firstLayer->id == secondLayer->id) {
            gain = elevationSpan;
        } else if (i == secondLayer->id && firstLayer->id != secondLayer->id) {
            gain = ratio + elevationSpan;
        } else {
            gain = elevationSpan / narrow<float>((i - secondLayer->id) * 2);
        }
        auto const i_size{ narrow<std::size_t>(i) };
        gain = std::min(gain, 1.0f);
        computeGains(field.layers[i_size], source, tempGains.data() + c);
        std::transform(tempGains.cbegin() + c,
                       tempGains.cbegin() + c + field.layers[i_size].speakerPositions.size(),
                       tempGains.begin() + c,
                       [gain](float const x) { return x * gain; });
        c += field.layers[i_size].speakerPositions.size();
    }

    for (size_t i{}; i < field.getNumSpeakers(); ++i) {
        auto const & outputPatch{ field.outputOrder[i] };
        gains[outputPatch] = tempGains[i];
    }
}

} // namespace gris