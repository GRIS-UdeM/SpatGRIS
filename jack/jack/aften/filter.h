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
 * @file filter.h
 * Audio filters header
 */

#ifndef FILTER_H
#define FILTER_H

#include "common.h"

enum FilterType {
    FILTER_TYPE_LOWPASS,
    FILTER_TYPE_HIGHPASS,
    FILTER_TYPE_BANDPASS,
    FILTER_TYPE_BANDSTOP,
    FILTER_TYPE_ALLPASS,
};

enum FilterID {
    FILTER_ID_BIQUAD_I,
    FILTER_ID_BIQUAD_II,
    FILTER_ID_BUTTERWORTH_I,
    FILTER_ID_BUTTERWORTH_II,
    FILTER_ID_ONEPOLE,
};

struct Filter;

typedef struct {
    const struct Filter *filter;
    void *private_context;
    enum FilterType type;
    int cascaded;
    FLOAT cutoff;
    FLOAT cutoff2;
    FLOAT samplerate;
    int taps;
} FilterContext;

extern int filter_init(FilterContext *f, enum FilterID id);

extern void filter_run(FilterContext *f, FLOAT *out, FLOAT *in, int n);

extern void filter_close(FilterContext *f);

#endif /* FILTER_H */
