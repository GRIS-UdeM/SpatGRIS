/**
 * Aften: A/52 audio encoder
 * Copyright (c) 2007 Justin Ruggles
 * Copyright (c) 2007 Prakash Punnoor <prakash@punnoor.de>
 *
 * Uses a modified version of memalign hack from FFmpeg
 * Copyright (c) 2002 Fabrice Bellard.
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
 * @file mem.h
 * Memory-related functions
 */

#ifndef MEM_H
#define MEM_H

#include "common.h"

#ifdef HAVE_MM_MALLOC

#define aligned_malloc(X) _mm_malloc(X,16)

#define aligned_free(X) _mm_free(X)

#else
#ifdef HAVE_POSIX_MEMALIGN

#define _XOPEN_SOURCE 600
#include <stdlib.h>

static inline void *
aligned_malloc(size_t size)
{
    void *mem;
    if (posix_memalign(&mem, 16, size))
        return NULL;
    return mem;
}

#define aligned_free(X) free(X)

#else

static inline void*
aligned_malloc(long size)
{
    void *mem;
    long diff;
    mem = malloc(size+32);
    if(!mem)
        return mem;
    diff = ((long)mem & 15) - 32;
    mem -= diff;
    ((int *)mem)[-1] = (int)diff;
    return mem;
}

static inline void
aligned_free(void *ptr)
{
    if(ptr)
        free(ptr + ((int*)ptr)[-1]);
}

#endif /* HAVE_POSIX_MEMALIGN */

#endif /* HAVE_MM_MALLOC */

#endif /* MEM_H */
