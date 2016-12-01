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
 * @file bitio.h
 * Bitwise file writer header
 */

#ifndef BITIO_H
#define BITIO_H

#include "common.h"

typedef struct BitWriter {
    uint32_t bit_buf;
    int bit_left;
    uint8_t *buffer, *buf_ptr, *buf_end;
    int eof;
} BitWriter;

extern void bitwriter_init(BitWriter *bw, void *buf, int len);

extern void bitwriter_flushbits(BitWriter *bw);

#define bitwriter_writebit(bw, val) bitwriter_writebits(bw, 1, val)

extern void bitwriter_writebits(BitWriter *bw, int bits, uint32_t val);

extern uint32_t bitwriter_bitcount(BitWriter *bw);

#endif /* BITIO_H */
