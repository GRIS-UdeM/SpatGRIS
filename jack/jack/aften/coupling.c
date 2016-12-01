/**
 * Aften: A/52 audio encoder
 * Copyright (c) 2007 Justin Ruggles
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
 * @file coupling.c
 * A/52 channel coupling
 */

#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "coupling.h"
#include "a52.h"
#include "aften.h"

/*
typedef struct A52CouplingStrategy {
    int cplinu;
    int chincpl[A52_MAX_CHANNELS];
    int phsflginu;
    int cplbegf;
    int cplendf;
    int cplbndstrc[18];
} A52CouplingStrategy;
*/

A52CouplingStrategy cplstr_stereo = {
    6, 12
    .cplbndstrc = { 0, 0, 1, 0, 1, 1, 0, 1, 1 }
};

A52CouplingStrategy cplstr_multi = {
    .cplinu = 1,
    10, 12
    .chincpl = { 0, 1, 0, 1, 1 }
}
