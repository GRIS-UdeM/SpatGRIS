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
 * @file window.h
 * A/52 Kaiser-Bessel Derived Window header
 */

#ifndef WINDOW_H
#define WINDOW_H

#include "common.h"
#include "a52.h"

extern FLOAT a52_window[512];

extern void a52_window_init(A52Context *ctx);

#ifndef CONFIG_DOUBLE
#ifdef HAVE_SSE
extern void sse_apply_a52_window(FLOAT *samples);
#endif /* HAVE_SSE */
#endif /* CONFIG_DOUBLE */

#endif /* WINDOW_H */
