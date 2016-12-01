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
 * @file window.c
 * A/52 Kaiser-Bessel Derived Window
 */

#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "window.h"
#include "cpu_caps.h"


ALIGN16(FLOAT) a52_window[512];

static void
apply_a52_window(FLOAT *samples)
{
    int i;
    for(i=0; i<512; i+=2) {
        samples[i  ] *= a52_window[i  ];
        samples[i+1] *= a52_window[i+1];
    }
}

/**
 * Generate a Kaiser-Bessel Derived Window.
 * @param ctx           The context
 * @param alpha         Determines window shape
 * @param out_window    Array to fill with window values
 * @param n             Full window size
 * @param iter          Number of iterations to use in BesselI0
 */
static void
kbd_window_init(A52Context *ctx, FLOAT alpha, FLOAT *window, int n, int iter)
{
    int i, j, n2;
    FLOAT a, x, bessel, sum;

    n2 = n >> 1;
    a = alpha * AFT_PI / n2;
    a = a*a;
    sum = 0.0;
    for(i=0; i<n2; i++) {
        x = i * (n2 - i) * a;
        bessel = FCONST(1.0);
        for(j=iter; j>0; j--) {
            bessel = (bessel * x / (j*j)) + FCONST(1.0);
        }
        sum += bessel;
        window[i] = sum;
    }
    sum += FCONST(1.0);
    for(i=0; i<n2; i++) {
        window[i] = AFT_SQRT(window[i] / sum);
        window[n-1-i] = window[i];
    }
#ifndef CONFIG_DOUBLE
#ifdef HAVE_SSE
    if (cpu_caps_have_sse()) {
        ctx->apply_a52_window = sse_apply_a52_window;
        return;
    }
#endif /* HAVE_SSE */
#endif /* CONFIG_DOUBLE */
    ctx->apply_a52_window = apply_a52_window;
}

void
a52_window_init(A52Context *ctx)
{
    kbd_window_init(ctx, 5.0, a52_window, 512, 50);
}
