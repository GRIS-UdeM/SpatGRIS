/**
 * Aften: A/52 audio encoder
 * Copyright (c) 2007, David Conrad
 * Copyright (c) 2006, Ryan C. Gordon
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

#ifndef ALTIVEC_COMMON_H
#define ALTIVEC_COMMON_H

typedef vector unsigned char  vec_u8_t;
typedef vector signed char    vec_s8_t;
typedef vector unsigned short vec_u16_t;
typedef vector signed short   vec_s16_t;
typedef vector unsigned int   vec_u32_t;
typedef vector signed int     vec_s32_t;

#define VPERMUTE4(a,b,c,d) (vec_u8_t) \
                             ( (a*4)+0, (a*4)+1, (a*4)+2, (a*4)+3, \
                               (b*4)+0, (b*4)+1, (b*4)+2, (b*4)+3, \
                               (c*4)+0, (c*4)+1, (c*4)+2, (c*4)+3, \
                               (d*4)+0, (d*4)+1, (d*4)+2, (d*4)+3 )

static inline vector float vec_ld_float(const float *a)
{
    switch((int)a & 0xc) {
        case 4:  return vec_splat(vec_lde(0, a), 1);
        case 8:  return vec_splat(vec_lde(0, a), 2);
        case 12: return vec_splat(vec_lde(0, a), 3);
    }
    return vec_splat(vec_lde(0, a), 0);
}

#endif
