/*********************************************************************
 * Copyright (C) 2005-2007 by Prakash Punnoor                        *
 * prakash@punnoor.de                                                *
 *                                                                   *
 * This library is free software; you can redistribute it and/or     *
 * modify it under the terms of the GNU Library General Public       *
 * License as published by the Free Software Foundation;             *
 * version 2 of the License                                          *
 *                                                                   *
 * This library is distributed in the hope that it will be useful,   *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of    *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU *
 * Library General Public License for more details.                  *
 *                                                                   *
 * You should have received a copy of the GNU Library General Public *
 * License along with this library; if not, write to the             *
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,      *
 * Boston, MA  02110-1301, USA.                                      *
 *********************************************************************/
#ifndef X86_SIMD_SUPPORT_H
#define X86_SIMD_SUPPORT_H

#include "mem.h"

#ifdef USE_MMX
#include <mmintrin.h>

union __m64ui {
    unsigned int ui[4];
    __m64 v;
};

#ifdef USE_SSE
#include <xmmintrin.h>

union __m128ui {
    unsigned int ui[4];
    __m128 v;
};

union __m128f {
    float f[4];
    __m128 v;
};

#define _mm_lddqu_ps(x) _mm_loadu_ps(x)

#ifdef USE_SSE2
#include <emmintrin.h>

union __m128iui {
    unsigned int ui[4];
    __m128i v;
};

#ifdef USE_SSE3
#include <pmmintrin.h>

#ifdef EMU_CASTSI128
#define _mm_castsi128_ps(X) ((__m128)(X))
#endif /* EMU_CASTSI128 */

#undef _mm_lddqu_ps
#define _mm_lddqu_ps(x) _mm_castsi128_ps(_mm_lddqu_si128((__m128i*)(x)))
#endif /* USE_SSE3 */
#endif /* USE_SSE2 */

#ifndef _MM_ALIGN16
#define _MM_ALIGN16  __attribute__((aligned(16)))
#endif

#define PM128(x) (*(__m128*)(x))
#endif /* USE_SSE */
#endif /* USE_MMX */
#endif /* X86_SIMD_SUPPORT_H */
