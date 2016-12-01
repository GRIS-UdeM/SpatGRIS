/*********************************************************************
 * Copyright (C) 2006 by Prakash Punnoor                             *
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
#ifndef CPU_CAPS_H
#define CPU_CAPS_H

#if defined(HAVE_MMX)
#include "x86_cpu_caps.h"
#elif defined(HAVE_ALTIVEC)
#include "ppc_cpu_caps.h"
#else
static inline void cpu_caps_detect(void){}
#endif

#endif /* CPU_CAPS_H */
