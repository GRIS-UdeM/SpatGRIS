/**
 * Aften: A/52 audio encoder
 * Copyright (c) 2006 Justin Ruggles
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file filter.c
 * Audio filters
 */

#include "common.h"

#include <stdlib.h>
#include <string.h>

#include "filter.h"

typedef struct Filter {
    const char *name;
    enum FilterID id;
    int private_size;
    int (*init)(FilterContext *f);
    void (*filter)(FilterContext *f, FLOAT *out, FLOAT *in, int n);
} Filter;


typedef struct {
    FLOAT coefs[5];
    FLOAT state[2][5];
} BiquadContext;

static void
biquad_generate_lowpass(BiquadContext *f, FLOAT fc)
{
    FLOAT omega, alpha, cs;
    FLOAT a[3], b[3];

    omega = FCONST(2.0) * AFT_PI * fc;
    alpha = AFT_SIN(omega) / FCONST(2.0);
    cs = AFT_COS(omega);

    a[0] =  FCONST(1.0) + alpha;
    a[1] = -FCONST(2.0) * cs;
    a[2] =  FCONST(1.0) - alpha;
    b[0] = (FCONST(1.0) - cs) / FCONST(2.0);
    b[1] = (FCONST(1.0) - cs);
    b[2] = (FCONST(1.0) - cs) / FCONST(2.0);

    f->coefs[0] = b[0] / a[0];
    f->coefs[1] = b[1] / a[0];
    f->coefs[2] = b[2] / a[0];
    f->coefs[3] = a[1] / a[0];
    f->coefs[4] = a[2] / a[0];
}

static void
biquad_generate_highpass(BiquadContext *f, FLOAT fc)
{
    FLOAT omega, alpha, cs;
    FLOAT a[3], b[3];

    omega = FCONST(2.0) * AFT_PI * fc;
    alpha = AFT_SIN(omega) / FCONST(2.0);
    cs = AFT_COS(omega);

    a[0] =   FCONST(1.0) + alpha;
    a[1] =  -FCONST(2.0) * cs;
    a[2] =   FCONST(1.0) - alpha;
    b[0] =  (FCONST(1.0) + cs) / FCONST(2.0);
    b[1] = -(FCONST(1.0) + cs);
    b[2] =  (FCONST(1.0) + cs) / FCONST(2.0);

    f->coefs[0] = b[0] / a[0];
    f->coefs[1] = b[1] / a[0];
    f->coefs[2] = b[2] / a[0];
    f->coefs[3] = a[1] / a[0];
    f->coefs[4] = a[2] / a[0];
}

static int
biquad_init(FilterContext *f)
{
    int i, j;
    BiquadContext *b = f->private_context;
    FLOAT fc;

    if(f->samplerate <= 0) {
        return -1;
    }
    if(f->cutoff < 0 || f->cutoff > (f->samplerate/FCONST(2.0))) {
        return -1;
    }
    fc = f->cutoff / f->samplerate;

    if(f->type == FILTER_TYPE_LOWPASS) {
        biquad_generate_lowpass(b, fc);
    } else if(f->type == FILTER_TYPE_HIGHPASS) {
        biquad_generate_highpass(b, fc);
    } else {
        return -1;
    }

    for(j=0; j<2; j++) {
        for(i=0; i<5; i++) {
            b->state[j][i] = 0.0;
        }
    }

    return 0;
}

static void
biquad_i_run_filter(FilterContext *f, FLOAT *out, FLOAT *in, int n)
{
    BiquadContext *b = f->private_context;
    FLOAT *coefs = b->coefs;
    FLOAT *tmp = in;
    int i, j;

    for(j=0; j<1+f->cascaded; j++) {
        FLOAT *state_j = b->state[j];
        for(i=0; i<n; i++) {
            FLOAT v = 0;
            state_j[0] = tmp[i];

            v += coefs[0] * state_j[0];
            v += coefs[1] * state_j[1];
            v += coefs[2] * state_j[2];
            v -= coefs[3] * state_j[3];
            v -= coefs[4] * state_j[4];

            state_j[2] = state_j[1];
            state_j[4] = state_j[3];
            state_j[1] = state_j[0];
            state_j[3] = v;

            out[i] = CLIP(v, -FCONST(1.0), FCONST(1.0));
        }
        tmp = out;
    }
}

static void
biquad_ii_run_filter(FilterContext *f, FLOAT *out, FLOAT *in, int n)
{
    BiquadContext *b = f->private_context;
    FLOAT *coefs = b->coefs;
    FLOAT *tmp = in;
    int i, j;
    FLOAT v;

    for(j=0; j<1+f->cascaded; j++) {
        FLOAT *state_j = b->state[j];
        for(i=0; i<n; i++) {
            state_j[0] = tmp[i];

            v = coefs[0] * state_j[0] + state_j[1];
            state_j[1] = coefs[1] * state_j[0] - coefs[3] * v + state_j[2];
            state_j[2] = coefs[2] * state_j[0] - coefs[4] * v;

            out[i] = CLIP(v, -FCONST(1.0), FCONST(1.0));
        }
        tmp = out;
    }
}

static const Filter biquad_i_filter = {
    "Biquad Direct Form I",
    FILTER_ID_BIQUAD_I,
    sizeof(BiquadContext),
    biquad_init,
    biquad_i_run_filter,
};

static const Filter biquad_ii_filter = {
    "Biquad Direct Form II",
    FILTER_ID_BIQUAD_II,
    sizeof(BiquadContext),
    biquad_init,
    biquad_ii_run_filter,
};

static void
butterworth_generate_lowpass(BiquadContext *f, FLOAT fc)
{
    FLOAT c = FCONST(1.0) / AFT_TAN(AFT_PI * fc);
    FLOAT c2 = (c * c);

    f->coefs[0] = FCONST(1.0) / (c2 + AFT_SQRT2 * c + FCONST(1.0));
    f->coefs[1] = FCONST(2.0) * f->coefs[0];
    f->coefs[2] = f->coefs[0];
    f->coefs[3] = FCONST(2.0) * (FCONST(1.0) - c2) * f->coefs[0];
    f->coefs[4] = (c2 - AFT_SQRT2 * c + FCONST(1.0)) * f->coefs[0];
}

static void
butterworth_generate_highpass(BiquadContext *f, FLOAT fc)
{
    FLOAT c = AFT_TAN(AFT_PI * fc);
    FLOAT c2 = (c * c);

    f->coefs[0] = FCONST(1.0) / (c2 + AFT_SQRT2 * c + FCONST(1.0));
    f->coefs[1] = -FCONST(2.0) * f->coefs[0];
    f->coefs[2] = f->coefs[0];
    f->coefs[3] = FCONST(2.0) * (c2 - FCONST(1.0)) * f->coefs[0];
    f->coefs[4] = (c2 - AFT_SQRT2 * c + FCONST(1.0)) * f->coefs[0];
}

static int
butterworth_init(FilterContext *f)
{
    int i, j;
    BiquadContext *b = f->private_context;
    FLOAT fc;

    if(f->samplerate <= 0) {
        return -1;
    }
    if(f->cutoff < 0 || f->cutoff > (f->samplerate/FCONST(2.0))) {
        return -1;
    }
    fc = f->cutoff / f->samplerate;

    if(f->type == FILTER_TYPE_LOWPASS) {
        butterworth_generate_lowpass(b, fc);
    } else if(f->type == FILTER_TYPE_HIGHPASS) {
        butterworth_generate_highpass(b, fc);
    } else {
        return -1;
    }

    for(j=0; j<2; j++) {
        for(i=0; i<5; i++) {
            b->state[j][i] = 0.0;
        }
    }

    return 0;
}

static const Filter butterworth_i_filter = {
    "Butterworth Direct Form I",
    FILTER_ID_BUTTERWORTH_I,
    sizeof(BiquadContext),
    butterworth_init,
    biquad_i_run_filter,
};

static const Filter butterworth_ii_filter = {
    "Butterworth Direct Form II",
    FILTER_ID_BUTTERWORTH_II,
    sizeof(BiquadContext),
    butterworth_init,
    biquad_ii_run_filter,
};


/* One-pole, One-zero filter */

typedef struct {
    FLOAT p;
    FLOAT last;
} OnePoleContext;

static void
onepole_generate_lowpass(OnePoleContext *o, FLOAT fc)
{
    FLOAT omega, cs;

    omega = FCONST(2.0) * AFT_PI * fc;
    cs = FCONST(2.0) - AFT_COS(omega);
    o->p = cs - AFT_SQRT((cs*cs)-FCONST(1.0));
    o->last = 0.0;
}

static void
onepole_generate_highpass(OnePoleContext *o, FLOAT fc)
{
    FLOAT omega, cs;

    omega = FCONST(2.0) * AFT_PI * fc;
    cs = FCONST(2.0) + AFT_COS(omega);
    o->p = cs - AFT_SQRT((cs*cs)-FCONST(1.0));
    o->last = 0.0;
}

static int
onepole_init(FilterContext *f)
{
    OnePoleContext *o = f->private_context;
    FLOAT fc;

    if(f->cascaded) {
        return -1;
    }

    if(f->samplerate <= 0) {
        return -1;
    }
    if(f->cutoff < 0 || f->cutoff > (f->samplerate/FCONST(2.0))) {
        return -1;
    }
    fc = f->cutoff / f->samplerate;

    if(f->type == FILTER_TYPE_LOWPASS) {
        onepole_generate_lowpass(o, fc);
    } else if(f->type == FILTER_TYPE_HIGHPASS) {
        onepole_generate_highpass(o, fc);
    } else {
        return -1;
    }

    return 0;
}

static void
onepole_run_filter(FilterContext *f, FLOAT *out, FLOAT *in, int n)
{
    int i;
    FLOAT v;
    FLOAT p1 = 0;
    OnePoleContext *o = f->private_context;

    if(f->type == FILTER_TYPE_LOWPASS) {
        p1 = FCONST(1.0) - o->p;
    } else if(f->type == FILTER_TYPE_HIGHPASS) {
        p1 = o->p - FCONST(1.0);
    }

    for(i=0; i<n; i++) {
        v = (p1 * in[i]) + (o->p * o->last);
        o->last = out[i] = CLIP(v, -FCONST(1.0), FCONST(1.0));
    }
}

static const Filter onepole_filter = {
    "One-Pole Filter",
    FILTER_ID_ONEPOLE,
    sizeof(OnePoleContext),
    onepole_init,
    onepole_run_filter,
};


int
filter_init(FilterContext *f, enum FilterID id)
{
    if(f == NULL) return -1;

    switch(id) {
        case FILTER_ID_BIQUAD_I:        f->filter = &biquad_i_filter;
                                        break;
        case FILTER_ID_BIQUAD_II:       f->filter = &biquad_ii_filter;
                                        break;
        case FILTER_ID_BUTTERWORTH_I:   f->filter = &butterworth_i_filter;
                                        break;
        case FILTER_ID_BUTTERWORTH_II:  f->filter = &butterworth_ii_filter;
                                        break;
        case FILTER_ID_ONEPOLE:         f->filter = &onepole_filter;
                                        break;
        default:                        return -1;
    }

    f->private_context = calloc(f->filter->private_size, 1);

    return f->filter->init(f);
}

void
filter_run(FilterContext *f, FLOAT *out, FLOAT *in, int n)
{
    f->filter->filter(f, out, in, n);
}

void
filter_close(FilterContext *f)
{
    if(!f) return;
    if(f->private_context) {
        free(f->private_context);
        f->private_context = NULL;
    }
    f->filter = NULL;
}
