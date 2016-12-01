/**
 * Aften: A/52 audio encoder
 * Copyright (c) 2006  David Conrad
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

#ifndef PPC_CPU_CAPS_H
#define PPC_CPU_CAPS_H

#include "common.h"
#include "cpu_caps.h"

extern int ppc_cpu_caps_altivec;

void cpu_caps_detect(void);
void apply_simd_restrictions(AftenSimdInstructions *simd_instructions);

static inline int cpu_caps_have_altivec(void)
{
    return ppc_cpu_caps_altivec;
}

#endif
