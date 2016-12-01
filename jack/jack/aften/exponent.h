/**
 * Aften: A/52 audio encoder
 * Copyright (c) 2006 Justin Ruggles
 *
 * Based on "The simplest AC3 encoder" from FFmpeg
 * Copyright (c) 2000 Fabrice Bellard.
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
 * @file exponent.h
 * A/52 exponent header
 */

#ifndef EXPONENT_H
#define EXPONENT_H

#include "a52.h"

extern uint16_t expstr_set_bits[6][256];

extern void exponent_init(A52Context *ctx);

#ifdef HAVE_SSE2
extern void sse2_process_exponents(A52ThreadContext *tctx);
#endif /* HAVE_SSE2 */
#ifdef HAVE_MMX
extern void mmx_process_exponents(A52ThreadContext *tctx);
#endif /* HAVE_MMX */

#endif /* EXPONENT_H */
