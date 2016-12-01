/*********************************************************************
 * Copyright (C) 2007 by Prakash Punnoor                             *
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
#include "window.h"

#include <xmmintrin.h>

void
sse_apply_a52_window(FLOAT *samples)
{
    int i;

    for(i=0; i<512; i+=4) {
        __m128 input = _mm_load_ps(samples+i);
        __m128 window = _mm_load_ps(a52_window+i);
        input = _mm_mul_ps(input, window);
        _mm_store_ps(samples+i, input);
    }
}
