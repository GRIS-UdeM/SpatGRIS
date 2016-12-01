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
#ifndef X86_CPU_CAPS_H
#define X86_CPU_CAPS_H

#include "aften-types.h"
#include "common.h"

struct x86cpu_caps_s {
    int mmx;
    int sse;
    int sse2;
    int sse3;
    int ssse3;
    int amd_3dnow;
    int amd_3dnowext;
    int amd_sse_mmx;
    int cyrix_mmxext;
};

extern struct x86cpu_caps_s x86cpu_caps_use;

void cpu_caps_detect(void);
void apply_simd_restrictions(AftenSimdInstructions *simd_instructions);

static inline int cpu_caps_have_mmx(void);
static inline int cpu_caps_have_sse(void);
static inline int cpu_caps_have_sse2(void);
static inline int cpu_caps_have_sse3(void);
static inline int cpu_caps_have_ssse3(void);
static inline int cpu_caps_have_3dnow(void);
static inline int cpu_caps_have_3dnowext(void);
static inline int cpu_caps_have_ssemmx(void);


static inline int cpu_caps_have_mmx(void)
{
    return x86cpu_caps_use.mmx;
}

static inline int cpu_caps_have_sse(void)
{
    return x86cpu_caps_use.sse;
}

static inline int cpu_caps_have_sse2(void)
{
    return x86cpu_caps_use.sse2;
}

static inline int cpu_caps_have_sse3(void)
{
    return x86cpu_caps_use.sse3;
}

static inline int cpu_caps_have_ssse3(void)
{
    return x86cpu_caps_use.ssse3;
}

static inline int cpu_caps_have_3dnow(void)
{
    return x86cpu_caps_use.amd_3dnow;
}

static inline int cpu_caps_have_3dnowext(void)
{
    return x86cpu_caps_use.amd_3dnowext;
}

static inline int cpu_caps_have_ssemmx(void)
{
    return x86cpu_caps_use.amd_sse_mmx;
}

#endif /* not X86_CPU_CAPS_H */
