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
 * @file crc.c
 * CRC-16 calculation
 */

#include "common.h"

#include <stdlib.h>
#include <assert.h>

#include "crc.h"

#define CRC16_POLY  0x18005

static void
crc_init_table(uint16_t *table, int bits, int poly)
{
    int i, j, crc;

    poly = (poly + (1<<bits));
    for(i=0; i<256; i++) {
        crc = i;
        for(j=0; j<bits; j++) {
            if(crc & (1<<(bits-1))) {
                crc = (crc << 1) ^ poly;
            } else {
                crc <<= 1;
            }
        }
        table[i] = (crc & ((1<<bits)-1));
    }
}

static uint16_t crc16tab[256];

void
crc_init()
{
    crc_init_table(crc16tab, 16, CRC16_POLY);
}

static uint16_t
calc_crc(const uint16_t *table, int bits, const uint8_t *data, uint32_t len)
{
	uint16_t crc, v1, v2;

    crc = 0;
    while(len--) {
        v1 = (crc << 8) & ((1 << bits) - 1);
        v2 = (crc >> (bits - 8)) ^ *data++;
        crc = v1 ^ table[v2];
    }
    return crc;
}

uint16_t
calc_crc16(const uint8_t *data, uint32_t len)
{
	uint16_t crc;

    assert(data != NULL);

    crc = calc_crc(crc16tab, 16, data, len);
    return crc;
}

static uint16_t
mul_poly(uint32_t a, uint32_t b)
{
    uint32_t c = 0;
    while(a) {
        if(a & 1)
            c ^= b;
        a = a >> 1;
        b = b << 1;
        if(b & (1 << 16))
            b ^= CRC16_POLY;
    }
    return c;
}

static uint32_t
pow_poly(uint32_t n)
{
    uint32_t a = (CRC16_POLY >> 1);
    uint32_t r = 1;
    while(n) {
        if(n & 1)
            r = mul_poly(r, a);
        a = mul_poly(a, a);
        n >>= 1;
    }
    return r;
}

/**
 * calculates crc value which will result in zero crc
 * where the crc is the first 2 bytes of the data
 * @param crc   crc16 of all data except 1st 2 crc bytes
 * @param size  total bytes of data, including the crc
 */
uint16_t
crc16_zero(uint16_t crc, int size)
{
    int crc_inv;
    crc_inv = pow_poly(size*8);
    crc = mul_poly(crc_inv, crc);
    return crc;
}
