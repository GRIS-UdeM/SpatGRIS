#include "lbap.hpp"

#include "macros.h"
DISABLE_WARNINGS
#include <JuceHeader.h>
ENABLE_WARNINGS

/* =================================================================================
Opaque data type declarations.
================================================================================= */

struct lbap_layer {
    int id;                    /**< Layer id. */
    int numSpeakers;           /**< Number of speakers into the layer. */
    float elevation;           /**< Elevation of the layer in the range 0 .. pi/2. */
    float gainExponent;        /**< Speaker gain exponent for 4+ speakers. */
    float *** amplitudeMatrix; /**< Arrays of amplitude values [spk][x][y]. */
    lbap_pos * speakers;       /**< Array of speakers. */
};

struct lbap_field {
    int numSpeakers;      /**< Total number of speakers in the field. */
    int numLayers;        /**< Number of layers into the field. */
    int * outputOrder;    /**< Physical output order as a list of int. */
    lbap_layer ** layers; /**< Array of layers. */
};

/* =================================================================================
Utility functions.
================================================================================= */

/* Fill x and y attributes of an lbap_pos according to azimuth and radius values. */
static void lbap_poltocar(lbap_pos * pos)
{
    pos->x = pos->radius * std::cos(pos->azimuth);
    pos->y = pos->radius * std::sin(pos->azimuth);
}

/* Bilinear interpolation to retrieve the value at position (x, y) in a 2D matrix. */
static float lbap_lookup(float const * const * matrix, float const x, float const y)
{
    auto const xi = static_cast<int>(x);
    auto const yi = static_cast<int>(y);
    auto const xf = x - xi;
    auto const yf = y - yi;
    auto const v1 = matrix[xi][yi];
    auto const v2 = matrix[xi + 1][yi];
    auto const v3 = matrix[xi][yi + 1];
    auto const v4 = matrix[xi + 1][yi + 1];
    auto const xv1 = v1 + (v2 - v1) * xf;
    auto const xv2 = v3 + (v4 - v3) * xf;
    return xv1 + (xv2 - xv1) * yf;
}

/* Compare two speaker positions based on elevation. */
static int lbap_speaker_compare(const void * pa, const void * pb)
{
    const lbap_speaker * a = (lbap_speaker *)pa;
    const lbap_speaker * b = (lbap_speaker *)pb;
    if (a->elevation > b->elevation) {
        return 1;
    }
    return -1;
}

/* Checks if an elevation is less distant than +/- 5 degrees of a base elevation. */
static bool lbap_is_same_ele(float const baseElevation, float const elevation)
{
    auto const deg5rad{ 5.0f / 360.0f * juce::MathConstants<float>::twoPi };
    return elevation > (baseElevation - deg5rad) && elevation < (baseElevation + deg5rad);
}

/* Returns the average elevation from a list of speakers. */
static float lbap_mean_ele_from_speakers(lbap_speaker const * speakers, int const num)
{
    return std::accumulate(speakers, speakers + num, 0.0f) / static_cast<float>(num);
}

/* =================================================================================
lbap_pos utility functions.
================================================================================= */

/* Returns a pointer to an array of lbap_pos created from an array of lbap_speaker.
 * The pointer must be freed when done with it.
 */
static lbap_pos * lbap_pos_from_speakers(lbap_speaker const * speakers, int const num)
{
    lbap_pos * positions = static_cast<lbap_pos*>(malloc(sizeof(lbap_pos) * num));
    std::transform(speakers, speakers + num, positions, [](lbap_speaker const & speaker) -> lbap_pos {
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
static lbap_layer * lbap_layer_init(int id, float ele, lbap_pos * speakers, int num)
{
    int i, x, y, size1 = LBAP_MATRIX_SIZE + 1;

    lbap_layer * layer = (lbap_layer *)malloc(sizeof(lbap_layer));

    layer->id = id;
    layer->elevation = ele;
    layer->numSpeakers = num;

    if (num <= 4)
        layer->gainExponent = 1.0f;
    else
        layer->gainExponent = num / 4.0f;

    layer->speakers = (lbap_pos *)malloc(sizeof(lbap_pos) * num);
    for (i = 0; i < num; i++) {
        layer->speakers[i].azimuth = speakers[i].azimuth;
        layer->speakers[i].radius = speakers[i].radius;
        lbap_poltocar(&layer->speakers[i]);
    }

    layer->amplitudeMatrix = (float ***)malloc(sizeof(float *) * num);
    for (i = 0; i < num; i++) {
        layer->amplitudeMatrix[i] = (float **)malloc(sizeof(float *) * size1);
        for (x = 0; x < size1; x++) {
            layer->amplitudeMatrix[i][x] = (float *)malloc(sizeof(float) * size1);
            for (y = 0; y < size1; y++) {
                layer->amplitudeMatrix[i][x][y] = 0.0;
            }
        }
    }

    return layer;
}

/* Pre-compute the matrices of amplitude for the layer's speakers. */
static void lbap_layer_compute_matrix(lbap_layer * layer)
{
    int i, x, y, hsize = LBAP_MATRIX_SIZE / 2;
    float px, py, dist;

    for (i = 0; i < layer->numSpeakers; i++) {
        px = layer->speakers[i].x * hsize + hsize;
        py = layer->speakers[i].y * hsize + hsize;
        for (x = 0; x < LBAP_MATRIX_SIZE; x++) {
            for (y = 0; y < LBAP_MATRIX_SIZE; y++) {
                dist = sqrtf(powf(x - px, 2) + powf(y - py, 2));
                dist /= LBAP_MATRIX_SIZE;
                dist = dist < 0.0f ? 0.0f : dist > 1.0f ? 1.0f : dist;
                layer->amplitudeMatrix[i][x][y] = 1 - dist;
            }
            layer->amplitudeMatrix[i][x][LBAP_MATRIX_SIZE] = layer->amplitudeMatrix[i][x][0];
        }
        layer->amplitudeMatrix[i][LBAP_MATRIX_SIZE] = layer->amplitudeMatrix[i][0];
    }
}

/* Create a new layer, based on a lbap_pos array, and add it the to field.*/
static void lbap_layer_create(lbap_field * field, float ele, lbap_pos * speakers, int num)
{
    lbap_layer * layer;

    field->numSpeakers += num;
    field->numLayers++;

    if (field->numLayers == 1)
        field->layers = (lbap_layer **)malloc(sizeof(lbap_layer *));
    else
        field->layers = (lbap_layer **)realloc(field->layers, sizeof(lbap_layer *) * field->numLayers);

    layer = lbap_layer_init(field->numLayers - 1, ele, speakers, num);

    lbap_layer_compute_matrix(layer);

    field->layers[field->numLayers - 1] = layer;
}

/* Cleanup the memory used by a layer. */
static void lbap_layer_free(lbap_layer * layer)
{
    int i, x;
    if (layer->amplitudeMatrix) {
        for (i = 0; i < layer->numSpeakers; i++) {
            for (x = 0; x < LBAP_MATRIX_SIZE; x++) {
                free(layer->amplitudeMatrix[i][x]);
            }
            free(layer->amplitudeMatrix[i]);
        }
        free(layer->amplitudeMatrix);
    }
    if (layer->speakers) {
        free(layer->speakers);
    }
    free(layer);
}

/* Compute the gain of each layer's speakers, for the given position, and store
 * the result in the `gains` array.
 */
static void lbap_layer_compute_gains(lbap_layer * layer, float azi, float rad, float radspan, float * gains)
{
    int i, hsize = LBAP_MATRIX_SIZE / 2, sizeMinusOne = LBAP_MATRIX_SIZE - 1;
    float x, y, norm, comp, sum = 0.0f;
    float exponent = layer->gainExponent * (1.0f - radspan) * 2.0f;
    lbap_pos pos;
    pos.azimuth = azi;
    pos.radius = rad;
    lbap_poltocar(&pos);
    x = pos.x * (hsize - 1) + hsize;
    y = pos.y * (hsize - 1) + hsize;
    x = x < 0 ? 0 : x > sizeMinusOne ? sizeMinusOne : x;
    y = y < 0 ? 0 : y > sizeMinusOne ? sizeMinusOne : y;
    for (i = 0; i < layer->numSpeakers; i++) {
        gains[i] = powf(lbap_lookup(layer->amplitudeMatrix[i], x, y), exponent);
        sum += gains[i];
    }
    if (sum > 0.0f) {
        comp = rad < 1.0f ? powf(3.0f, (1.0f - rad)) : 1.0f;
        norm = 1.0f / sum * comp;                  // normalization (1.0 / sum) and compensation
        for (i = 0; i < layer->numSpeakers; i++) { // (powf(3.0, (1.0 - rad))) for energy spreading
            gains[i] *= norm;                      // when moving toward the center.
        }
    }
}

/* =================================================================================
====================================================================================
Layer-Based Amplitude Panning interface implementation.
====================================================================================
================================================================================= */

lbap_field * lbap_field_init(void)
{
    lbap_field * field = (lbap_field *)malloc(sizeof(lbap_field));
    field->numSpeakers = 0;
    field->numLayers = 0;
    field->layers = NULL;
    field->outputOrder = NULL;
    return field;
}

void lbap_field_free(lbap_field * field)
{
    int i;
    if (field->layers) {
        for (i = 0; i < field->numLayers; i++) {
            lbap_layer_free(field->layers[i]);
        }
        free(field->layers);
        field->layers = NULL;
    }
    if (field->outputOrder) {
        free(field->outputOrder);
    }
    free(field);
}

void lbap_field_reset(lbap_field * field)
{
    int i;
    if (field->layers) {
        for (i = 0; i < field->numLayers; i++) {
            lbap_layer_free(field->layers[i]);
        }
        free(field->layers);
    }
    if (field->outputOrder) {
        free(field->outputOrder);
    }
    field->numSpeakers = 0;
    field->numLayers = 0;
    field->layers = NULL;
    field->outputOrder = NULL;
}

void lbap_field_setup(lbap_field * field, lbap_speaker * speakers, int num)
{
    int i, start, howmany, count = 0;
    float ele, mean;
    lbap_pos * spk;

    field->outputOrder = (int *)malloc(sizeof(int) * num);

    qsort(speakers, num, sizeof(lbap_speaker), lbap_speaker_compare);

    for (i = 0; i < num; i++) {
        field->outputOrder[i] = speakers[i].outputPatch.get();
    }

    while (count < num) {
        start = count;
        ele = speakers[count++].elevation;
        while (count < num && lbap_is_same_ele(ele, speakers[count].elevation)) {
            count++;
        }
        howmany = count - start;
        spk = lbap_pos_from_speakers(&speakers[start], howmany);
        mean = lbap_mean_ele_from_speakers(&speakers[start], howmany);
        lbap_layer_create(field, mean, spk, howmany);
        free(spk);
    }
}

void lbap_field_compute(lbap_field * field, lbap_pos * position, float * gains)
{
    int i, j, c;
    float frac = 0.0, gain = 0.0, elespan = 0.0;
    float gns[LBAP_MAX_NUMBER_OF_SPEAKERS];

    if (field->layers == NULL) {
        return;
    }

    lbap_layer * first = field->layers[0];
    lbap_layer * second = field->layers[field->numLayers - 1];
    for (i = 0; i < field->numLayers; i++) {
        if (field->layers[i]->elevation > position->elevation) {
            second = field->layers[i];
            break;
        }
        first = field->layers[i];
    }

    if (first->id != (field->numLayers - 1))
        frac = (position->elevation - first->elevation) / (second->elevation - first->elevation);

    if (position->elevationSpan != 0.0)
        elespan = powf(position->elevationSpan, 4) * 6;

    c = 0;
    for (i = 0; i < field->numLayers; i++) {
        if (i < first->id) {
            gain = elespan / ((first->id - i) * 2);
        } else if (i == first->id) {
            gain = (1 - frac) + elespan;
        } else if (i == second->id && first->id != second->id) {
            gain = frac + elespan;
        } else if (i == second->id && first->id == second->id) {
            gain = elespan;
        } else {
            gain = elespan / ((i - second->id) * 2);
        }
        gain = gain > 1.0f ? 1.0f : gain;
        lbap_layer_compute_gains(field->layers[i], position->azimuth, position->radius, position->radiusSpan, &gns[c]);
        for (j = 0; j < field->layers[i]->numSpeakers; j++) {
            gns[c++] *= gain;
        }
    }

    for (i = 0; i < field->numSpeakers; i++) {
        gains[field->outputOrder[i]] = gns[i];
    }
}

lbap_speaker *
    lbap_speakers_from_positions(float * azimuth, float * elevation, float * radius, output_patch_t * spkid, int num)
{
    int i;
    lbap_speaker * speakers = (lbap_speaker *)malloc(sizeof(lbap_speaker) * num);
    for (i = 0; i < num; i++) {
        speakers[i].azimuth = azimuth[i] / 360.0f * (float)juce::MathConstants<float>::pi * 2.0f;
        speakers[i].elevation = elevation[i] / 360.0f * (float)juce::MathConstants<float>::pi * 2.0f;
        speakers[i].radius = radius[i] < 0.0f ? 0.0f : radius[i] > 1.0f ? 1.0f : radius[i];
        speakers[i].outputPatch = spkid[i];
    }
    return speakers;
}

void lbap_pos_init_from_radians(lbap_pos * position, float azimuth, float elevation, float radius)
{
    position->radiusSpan = 0.0f;
    position->elevationSpan = 0.0f;
    while (azimuth < -juce::MathConstants<float>::pi) {
        azimuth += (float)juce::MathConstants<float>::pi * 2.0f;
    }
    while (azimuth > juce::MathConstants<float>::pi) {
        azimuth -= (float)juce::MathConstants<float>::pi * 2.0f;
    }
    if (elevation < 0) {
        elevation = 0.0f;
    } else if (elevation > (juce::MathConstants<float>::pi / 2)) {
        elevation = (float)juce::MathConstants<float>::pi / 2.0f;
    }

    position->azimuth = azimuth;
    position->elevation = elevation;
    position->radius = radius < 0.0f ? 0.0f : radius > 2.0f ? 2.0f : radius;
}

void lbap_pos_init_from_degrees(lbap_pos * position, float azimuth, float elevation, float radius)
{
    position->radiusSpan = 0.0f;
    position->elevationSpan = 0.0f;

    float deg2rad = 1.0f / 360.0f * juce::MathConstants<float>::pi * 2.0f;

    while (azimuth < -180) {
        azimuth += 360.0;
    }
    while (azimuth > 180) {
        azimuth -= 360.0;
    }
    if (elevation < 0) {
        elevation = 0.0;
    } else if (elevation > 90) {
        elevation = 90.0;
    }

    position->azimuth = azimuth * deg2rad;
    position->elevation = elevation * deg2rad;
    position->radius = radius < 0.0f ? 0.0f : radius > 2.0f ? 2.0f : radius;
}

int lbap_pos_compare(lbap_pos * p1, lbap_pos * p2)
{
    if (p1->azimuth == p2->azimuth && p1->elevation == p2->elevation && p1->radius == p2->radius
        && p1->radiusSpan == p2->radiusSpan && p1->elevationSpan == p2->elevationSpan)
        return 1;
    else
        return 0;
}

void lbap_pos_copy(lbap_pos * dest, lbap_pos * src)
{
    dest->azimuth = src->azimuth;
    dest->elevation = src->elevation;
    dest->radius = src->radius;
    dest->radiusSpan = src->radiusSpan;
    dest->elevationSpan = src->elevationSpan;
}
