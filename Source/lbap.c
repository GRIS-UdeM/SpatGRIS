#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "lbap.h"

/* =================================================================================
Opaque data type declarations.
================================================================================= */

struct lbap_layer {
    int id;                 /**< Layer id. */
    int num_of_speakers;    /**< Number of speakers into the layer. */
    float ele;              /**< Elevation of the layer in the range 0 .. pi/2. */
    float expon;            /**< Speaker gain exponent for 4+ speakers. */ 
    float ***matrix;        /**< Arrays of amplitude values [spk][x][y]. */
    lbap_pos *speakers;     /**< Array of speakers. */
};

struct lbap_field {
    int num_of_speakers;    /**< Total number of speakers in the field. */
    int num_of_layers;      /**< Number of layers into the field. */
    int *out_order;         /**< Physical output order as a list of int. */
    lbap_layer **layers;    /**< Array of layers. */
};

/* =================================================================================
Utility functions.
================================================================================= */

/* Fill x and y attributes of an lbap_pos according to azimuth and radius values. */
static void 
lbap_poltocar(lbap_pos *pos) {
    pos->x = pos->rad * cosf(pos->azi);
    pos->y = pos->rad * sinf(pos->azi);
}

/* Bilinear interpolation to retrieve the value at position (x, y) in a 2D matrix. */
static float
lbap_lookup(float **matrix, float x, float y) {
    int xi = (int)x;
    int yi = (int)y;
    float xf = x - xi;
    float x1 = 1.0f - xf;
    float yf = y - yi;
    float y1 = 1.0f - yf;
    float v1 = matrix[xi][yi] * x1 * y1;
    float v2 = matrix[xi+1][yi] * xf * y1;
    float v3 = matrix[xi][yi+1] * x1 * yf;
    float v4 = matrix[xi+1][yi+1] * xf * yf;
    return (v1 + v2 + v3 + v4);
}

/* Compare two speaker positions based on elevation. */
static int
lbap_speaker_compare(const void *pa, const void *pb) {
    const lbap_speaker *a = (lbap_speaker *)pa;
    const lbap_speaker *b = (lbap_speaker *)pb;
    if (a->ele > b->ele)
        return 1;
    else
        return -1;
}

/* Checks if an elevation is less distant than +/- 5 degrees of a base elevation. */
static int
lbap_is_same_ele(float base_ele, float ele) {
    float deg5rad = 5.0 / 360.0 * M_PI * 2;
    if (ele > (base_ele - deg5rad) && ele < (base_ele + deg5rad))
        return 1;
    else
        return 0;
}

/* Returns the average elevation from a list of speakers. */
static float
lbap_mean_ele_from_speakers(lbap_speaker *speakers, int num) {
    int i;
    float sum = 0.0;
    for (i=0; i<num; i++) {
        sum += speakers[i].ele;
    }
    return sum / num;
}

/* =================================================================================
lbap_pos utility functions.
================================================================================= */

/* Returns a pointer to an array of lbap_pos created from an array of lbap_speaker.
 * The pointer must be freed when done with it.
 */
static lbap_pos * 
lbap_pos_from_speakers(lbap_speaker *speakers, int num) {
    int i;
    lbap_pos *positions = (lbap_pos *)malloc(sizeof(lbap_pos) * num);
    for (i=0; i<num; i++) {
        positions[i].azi = speakers[i].azi;
        positions[i].ele = speakers[i].ele;
        positions[i].rad = speakers[i].rad;
    }
    return positions;
}

/* =================================================================================
lbap_layer utility functions.
================================================================================= */

/* Initialize a newly created layer for `num` speakers. */
static lbap_layer *
lbap_layer_init(int id, float ele, lbap_pos *speakers, int num) {
    int i, x, y, size1 = LBAP_MATRIX_SIZE + 1;

    lbap_layer *layer = (lbap_layer *)malloc(sizeof(lbap_layer));

    layer->id = id;
    layer->ele = ele;
    layer->num_of_speakers = num;

    if (num <= 4)
        layer->expon = 1.0;
    else
        layer->expon = num / 4.0;

    layer->speakers = (lbap_pos *)malloc(sizeof(lbap_pos) * num);
    for (i=0; i<num; i++) {
        layer->speakers[i].azi = speakers[i].azi;
        layer->speakers[i].rad = speakers[i].rad;
        lbap_poltocar(&layer->speakers[i]);
    }

    layer->matrix = (float ***)malloc(sizeof(float *) * num);
    for (i=0; i<num; i++) {
        layer->matrix[i] = (float **)malloc(sizeof(float *) * size1);
        for (x=0; x<size1; x++) {
            layer->matrix[i][x] = (float *)malloc(sizeof(float) * size1);
            for (y=0; y<size1; y++) {
                layer->matrix[i][x][y] = 0.0;
            }
        }
    }

    return layer;
}

/* Pre-compute the matrices of amplitude for the layer's speakers. */
static void
lbap_layer_compute_matrix(lbap_layer *layer) {
    int i, x, y, hsize = LBAP_MATRIX_SIZE / 2;
    float px, py, dist;

    for (i=0; i<layer->num_of_speakers; i++) {
        px = layer->speakers[i].x * hsize + hsize;
        py = layer->speakers[i].y * hsize + hsize;
        for (x=0; x<LBAP_MATRIX_SIZE; x++) {
            for (y=0; y<LBAP_MATRIX_SIZE; y++) {
                dist = sqrtf(powf(x - px, 2) + powf(y - py, 2));
                dist /= LBAP_MATRIX_SIZE;
                dist = dist < 0.0f ? 0.0f : dist > 1.0f ? 1.0f : dist;
                layer->matrix[i][x][y] = 1 - dist;
            }
            layer->matrix[i][x][LBAP_MATRIX_SIZE] = layer->matrix[i][x][0];
        }
        layer->matrix[i][LBAP_MATRIX_SIZE] = layer->matrix[i][0];
    }
}

/* Create a new layer, based on a lbap_pos array, and add it the to field.*/
static void
lbap_layer_create(lbap_field *field, float ele, lbap_pos *speakers, int num) {
    lbap_layer *layer;

    field->num_of_speakers += num;
    field->num_of_layers++;

    if (field->num_of_layers == 1)
        field->layers = (lbap_layer **)malloc(sizeof(lbap_layer *));
    else
        field->layers = (lbap_layer **)realloc(field->layers,
                                               sizeof(lbap_layer *) * field->num_of_layers);

    layer = lbap_layer_init(field->num_of_layers-1, ele, speakers, num);

    lbap_layer_compute_matrix(layer);

    field->layers[field->num_of_layers-1] = layer;
}

/* Cleanup the memory used by a layer. */
static void
lbap_layer_free(lbap_layer *layer) {
    int i, x;
    if (layer->matrix) {
        for (i=0; i<layer->num_of_speakers; i++) {
            for (x=0; x<LBAP_MATRIX_SIZE; x++) {
                free(layer->matrix[i][x]);
            }
            free(layer->matrix[i]);
        }
        free(layer->matrix);
    }
    if (layer->speakers) {
        free(layer->speakers);
    }
    free(layer);
}

/* Compute the gain of each layer's speakers, for the given position, and store 
 * the result in the `gains` array.
 */
static void
lbap_layer_compute_gains(lbap_layer *layer, float azi, float rad, float azispan, float *gains) {
    int i, hsize = LBAP_MATRIX_SIZE / 2;
    float x, y, norm, sum = 0.0;
    lbap_pos pos;
    pos.azi = azi;
    pos.rad = rad;
    lbap_poltocar(&pos);
    x = pos.x * (hsize - 1) + hsize;
    y = pos.y * (hsize - 1) + hsize;
    for (i=0; i<layer->num_of_speakers; i++) {
        gains[i] = powf(lbap_lookup(layer->matrix[i], x, y), layer->expon * (1.0 - azispan) * 2.0);
        sum += gains[i];
    }
    if (sum > 0.0) {
        norm = 1.0 / sum;
        for (i=0; i<layer->num_of_speakers; i++) {
            gains[i] *= norm;
        }
    }
}

/* =================================================================================
====================================================================================
Layer-Based Amplitude Panning interface implementation.
====================================================================================
================================================================================= */

lbap_field * 
lbap_field_init() {
    lbap_field *field = (lbap_field *)malloc(sizeof(lbap_field));
    field->num_of_speakers = 0;
    field->num_of_layers = 0;
    field->layers = NULL;
    field->out_order = NULL;
    return field;
}

void
lbap_field_free(lbap_field *field) {
    int i;
    if (field->layers) {
        for (i=0; i<field->num_of_layers; i++) {
            lbap_layer_free(field->layers[i]);
        }
        free(field->layers);
        field->layers = NULL;
    }
    if (field->out_order) {
        free(field->out_order);
    }
    free(field);
}

void
lbap_field_reset(lbap_field *field) {
    int i;
    if (field->layers) {
        for (i=0; i<field->num_of_layers; i++) {
            lbap_layer_free(field->layers[i]);
        }
        free(field->layers);
    }
    if (field->out_order) {
        free(field->out_order);
    }
    field->num_of_speakers = 0;
    field->num_of_layers = 0;
    field->layers = NULL;
    field->out_order = NULL;
}

void
lbap_field_setup(lbap_field *field, lbap_speaker *speakers, int num) {
    int i, start, howmany, count = 0;
    float ele, mean;
    lbap_pos *spk;

    field->out_order = (int *)malloc(sizeof(int) * num);

    qsort(speakers, num, sizeof(lbap_speaker), lbap_speaker_compare);

    for (i=0; i<num; i++) {
        field->out_order[i] = speakers[i].spkid;
    }

    while (count < num) {
        start = count;
        ele = speakers[count++].ele;
        while (count < num && lbap_is_same_ele(ele, speakers[count].ele)) {
            count++;
        }
        howmany = count - start;
        spk = lbap_pos_from_speakers(&speakers[start], howmany);
        mean = lbap_mean_ele_from_speakers(&speakers[start], howmany);
        lbap_layer_create(field, mean, spk, howmany);
        free(spk);
    }
}

void
lbap_field_compute(lbap_field *field, lbap_pos *pos, float *gains) {
    int i, j, c;
    float frac = 0.0, gain = 0.0, elespan = 0.0;
    float gns[field->num_of_speakers];

    lbap_layer *first = field->layers[0];
    lbap_layer *second = field->layers[field->num_of_layers-1];
    for (i=0; i<field->num_of_layers; i++) {
        if (field->layers[i]->ele > pos->ele) {
            second = field->layers[i];
            break;
        }
        first = field->layers[i];
    }

    if (first->id != (field->num_of_layers-1))
        frac = (pos->ele - first->ele) / (second->ele - first->ele);

    if (pos->elespan != 0.0)
        elespan = powf(pos->elespan, 2) * 6;

    c = 0;
    for (i=0; i<field->num_of_layers; i++) {
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
        gain = gain > 1.0 ? 1.0 : gain;
        lbap_layer_compute_gains(field->layers[i], pos->azi, pos->rad, pos->azispan, &gns[c]);
        for (j=0; j<field->layers[i]->num_of_speakers; j++) {
            gns[c++] *= gain;
        }
    }

    for (i=0; i<field->num_of_speakers; i++) {
        gains[field->out_order[i]] = gns[i];
    }
}

/*
void
lbap_field_compute(lbap_field *field, lbap_pos *pos, float *gains) {
    int i, j, c;
    float frac, gns[field->num_of_speakers];

    lbap_layer *first = field->layers[0];
    lbap_layer *second = field->layers[field->num_of_layers-1];
    for (i=0; i<field->num_of_layers; i++) {
        if (field->layers[i]->ele > pos->ele) {
            second = field->layers[i];
            break;
        }
        first = field->layers[i];
    }

    c = 0;
    if (first->id == second->id) {
        for (i=0; i<field->num_of_layers; i++) {
            if (i != first->id)
                memset(&gns[c], 0, sizeof(float) * field->layers[i]->num_of_speakers);
            else
                lbap_layer_compute_gains(first, pos->azi, pos->rad, pos->azispan, &gns[c]);
            c += field->layers[i]->num_of_speakers;
        }
    } else {
        frac = (pos->ele - first->ele) / (second->ele - first->ele);
        for (i=0; i<field->num_of_layers; i++) {
            if (i == first->id) {
                 lbap_layer_compute_gains(first, pos->azi, pos->rad, pos->azispan, &gns[c]);
                 for (j=0; j<first->num_of_speakers; j++) {
                    gns[c++] *= (1 - frac);
                 }
            } else if (i == second->id) {
                 lbap_layer_compute_gains(second, pos->azi, pos->rad, pos->azispan, &gns[c]);
                 for (j=0; j<second->num_of_speakers; j++) {
                    gns[c++] *= frac;
                 }
            } else {
                memset(&gns[c], 0, sizeof(float) * field->layers[i]->num_of_speakers);
                c += field->layers[i]->num_of_speakers;
            }
        }
    }

    for (i=0; i<field->num_of_speakers; i++) {
        gains[field->out_order[i]] = gns[i];
    }
}
*/

lbap_speaker *
lbap_speakers_from_positions(float *azi, float *ele, float *rad, int *spkid, int num) {
    int i;
    lbap_speaker *speakers = (lbap_speaker *)malloc(sizeof(lbap_speaker) * num);
    for (i=0; i<num; i++) {
        speakers[i].azi = azi[i] / 360.0f * M_PI * 2;
        speakers[i].ele = ele[i] / 360.0f * M_PI * 2;
        speakers[i].rad = rad[i] < 0.0f ? 0.0f : rad[i] > 1.0f ? 1.0f : rad[i];
        speakers[i].spkid = spkid[i];
    }
    return speakers;
}

void lbap_pos_init_from_radians(lbap_pos *pos, float azi, float ele, float rad) {
    while (azi < -M_PI) {
        azi += M_PI * 2;
    }
    while (azi > M_PI) {
        azi -= M_PI * 2;
    }
    if (ele < 0) {
        ele = 0.0;
    } else if (ele > (M_PI / 2)) {
        ele = M_PI / 2;
    }

    pos->azi = azi;
    pos->ele = ele;
    pos->rad = rad < 0.0f ? 0.0f : rad > 1.0f ? 1.0f : rad;
}

void lbap_pos_init_from_degrees(lbap_pos *pos, float azi, float ele, float rad) {
    float deg2rad = 1.0f / 360.0f * M_PI * 2.0f;

    while (azi < -180) {
        azi += 360.0;
    }
    while (azi > 180) {
        azi -= 360.0;
    }
    if (ele < 0) {
        ele = 0.0;
    } else if (ele > 90) {
        ele = 90.0;
    }

    pos->azi = azi * deg2rad;
    pos->ele = ele * deg2rad;
    pos->rad = rad < 0.0f ? 0.0f : rad > 1.0f ? 1.0f : rad;
}

int lbap_pos_compare(lbap_pos *p1, lbap_pos *p2) {
    if (p1->azi == p2->azi && p1->ele == p2->ele && p1->rad == p2->rad && 
       p1->azispan == p2->azispan && p1->elespan == p2->elespan)
        return 1;
    else
        return 0;
}

void lbap_pos_copy(lbap_pos *dest, lbap_pos *src) {
    dest->azi = src->azi;
    dest->ele = src->ele;
    dest->rad = src->rad;
}
