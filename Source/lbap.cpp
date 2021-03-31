#include "lbap.hpp"

#include "macros.h"
DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

#include "AudioStructs.hpp"
#include "constants.hpp"
#include "narrow.hpp"

/* =================================================================================
Utility functions.
================================================================================= */

/* Fill x and y attributes of an lbap_pos according to azimuth and radius values. */
static void fillCartesianFromPolar(lbap_pos & pos)
{
    pos.x = pos.radius * std::cos(pos.azimuth.get());
    pos.y = pos.radius * std::sin(pos.azimuth.get());
}

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

/* Checks if an elevation is less distant than +/- 5 degrees of a base elevation. */
static bool isPracticallySameElevation(radians_t const baseElevation, radians_t const elevation)
{
    static constexpr radians_t TOLERANCE{ degrees_t{ 5.0f } };
    return elevation > (baseElevation - TOLERANCE) && elevation < (baseElevation + TOLERANCE);
}

/* Returns the average elevation from a list of speakers. */
static radians_t averageSpeakerElevation(lbap_speaker const * speakers, size_t const num)
{
    static auto const ACCUMULATE_ELEVATION
        = [](radians_t const total, lbap_speaker const & speaker) { return total + speaker.elevation; };

    auto const sum{ std::reduce(speakers, speakers + num, radians_t{}, ACCUMULATE_ELEVATION) };
    return sum / narrow<float>(num);
}

/* =================================================================================
lbap_pos utility functions.
================================================================================= */

/* Returns a vector lbap_pos created from an array of lbap_speaker.
 */
static std::vector<lbap_pos> lbapPositionsFromSpeakers(lbap_speaker const * speakers, size_t const num)
{
    std::vector<lbap_pos> positions{};
    positions.reserve(num);
    std::transform(speakers,
                   speakers + num,
                   std::back_inserter(positions),
                   [](lbap_speaker const & speaker) -> lbap_pos {
                       lbap_pos result{};
                       result.azimuth = speaker.azimuth;
                       result.elevation = speaker.elevation;
                       result.radius = speaker.radius;
                       return result;
                   });
    return positions;
}

/* =================================================================================
lbap_layer utility functions.
================================================================================= */

/* Initialize a newly created layer for `num` speakers. */
static lbap_layer initLayers(int const layerId, radians_t const elevation, std::vector<lbap_pos> const & speakers)
{
    lbap_layer result{};

    result.id = layerId;
    result.elevation = elevation;
    result.gainExponent = std::max(narrow<float>(speakers.size()) / 4.0f, 1.0f);

    result.speakers.reserve(speakers.size());
    std::transform(speakers.cbegin(),
                   speakers.cend(),
                   std::back_inserter(result.speakers),
                   [](lbap_pos const & speaker) {
                       lbap_pos result{};
                       result.azimuth = speaker.azimuth;
                       result.radius = speaker.radius;
                       fillCartesianFromPolar(result);
                       return result;
                   });

    result.amplitudeMatrix.reserve(speakers.size());
    static constexpr matrix_t EMPTY_MATRIX{};
    std::fill_n(std::back_inserter(result.amplitudeMatrix), speakers.size(), EMPTY_MATRIX);

    return result;
}

/* Pre-compute the matrices of amplitude for the layer's speakers. */
static void computeMatrix(lbap_layer & layer)
{
    static auto constexpr H_SIZE = LBAP_MATRIX_SIZE / 2;

    for (size_t i{}; i < layer.speakers.size(); ++i) {
        auto const px = layer.speakers[i].x * H_SIZE + H_SIZE;
        auto const py = layer.speakers[i].y * H_SIZE + H_SIZE;
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

/* Create a new layer, based on a lbap_pos array, and add it the to field.*/
static lbap_layer
    createLayer(lbap_field const & field, radians_t const elevation, std::vector<lbap_pos> const & speakers)
{
    auto result{ initLayers(field.layers.size(), elevation, speakers) };
    computeMatrix(result);
    return result;
}

/* Compute the gain of each layer's speakers, for the given position, and store
 * the result in the `gains` array.
 */
static void computeGains(lbap_layer const & layer,
                         radians_t const azimuth,
                         float const radius,
                         float const radiusSpan,
                         float * gains)
{
    static constexpr auto H_SIZE = LBAP_MATRIX_SIZE / 2.0f;
    static constexpr auto SIZE_MINUS_ONE = LBAP_MATRIX_SIZE - 1.0f;

    auto const exponent = layer.gainExponent * (1.0f - radiusSpan) * 2.0f;
    lbap_pos pos;
    pos.azimuth = azimuth;
    pos.radius = radius;
    fillCartesianFromPolar(pos);
    auto x = pos.x * (H_SIZE - 1.0f) + H_SIZE;
    auto y = pos.y * (H_SIZE - 1.0f) + H_SIZE;
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
        auto const comp = radius < 1.0f ? std::pow(3.0f, (1.0f - radius)) : 1.0f;
        // normalization (1.0 / sum) and compensation
        auto const norm = 1.0f / sum * comp;
        std::transform(gains, gains + layer.speakers.size(), gains, [norm](float & gain) { return gain * norm; });
        for (size_t i{}; i < layer.speakers.size(); ++i) {
            gains[i] *= norm;
        }
    }
}

/* =================================================================================
====================================================================================
Layer-Based Amplitude Panning interface implementation.
====================================================================================
================================================================================= */

void lbap_field_setup(lbap_field & field, std::vector<lbap_speaker> & speakers)
{
    std::sort(speakers.begin(), speakers.end(), [](lbap_speaker const & a, lbap_speaker const & b) -> bool {
        return a.elevation < b.elevation;
    });

    std::transform(speakers.cbegin(),
                   speakers.cend(),
                   std::back_inserter(field.outputOrder),
                   [](lbap_speaker const & speaker) { return speaker.outputPatch; });

    size_t count{};
    while (count < speakers.size()) {
        auto const start = count;
        auto const elevation = speakers[count++].elevation;
        while (count < speakers.size() && isPracticallySameElevation(elevation, speakers[count].elevation)) {
            ++count;
        }
        auto const howMany{ count - start };
        auto const spk{ lbapPositionsFromSpeakers(&speakers[start], howMany) };
        auto const mean{ averageSpeakerElevation(&speakers[start], howMany) };
        field.layers.emplace_back(createLayer(field, mean, spk));
    }
}

void lbap_field_compute(lbap_field const & field, lbap_pos const & position, float * gains)
{
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

    auto const elevationSpan{ position.elevationSpan == 0.0f ? 0.0f : std::pow(position.elevationSpan, 4.0f) * 6.0f };

    size_t c{};
    float gain;
    std::array<float, MAX_OUTPUTS> tempGains{};
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
        computeGains(field.layers[i], position.azimuth, position.radius, position.radiusSpan, tempGains.data() + c);
        for (size_t j{}; j < field.layers[i].speakers.size(); ++j) {
            tempGains[c++] *= gain;
        }
    }

    // TODO : is it where shit hits the fan ?
    for (size_t i{}; i < field.getNumSpeakers(); ++i) {
        auto const outputPatch{ field.outputOrder[i].get() };
        gains[outputPatch] = tempGains[i];
    }
}

std::vector<lbap_speaker> lbap_speakers_from_positions(OwnedMap<SpeakerData, output_patch_t> const & speakers)
{
    std::vector<lbap_speaker> result{};
    result.reserve(speakers.size());
    std::transform(speakers.cbegin(), speakers.cend(), std::back_inserter(result), [](SpeakerData const * speaker) {
        auto const clampedRadius{ std::clamp(speaker->vector.length, 0.0f, 1.0f) };
        return lbap_speaker{ speaker->vector.azimuth, speaker->vector.elevation, clampedRadius, speaker->outputPatch };
    });
    return result;
}

lbap_pos lbap_pos_init_from_radians(radians_t const azimuth, radians_t const elevation, float const radius)
{
    lbap_pos result{};
    result.radiusSpan = 0.0f;
    result.elevationSpan = 0.0f;
    result.azimuth = azimuth.centered();
    result.elevation = std::clamp(elevation, radians_t{}, HALF_PI);
    result.radius = std::clamp(radius, 0.0f, 2.0f);
    return result;
}
