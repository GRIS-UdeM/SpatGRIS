/**
 * Aften: A/52 audio encoder
 *
 * Copyright (c) 2007, David Conrad
 * Copyright (c) 2006, Ryan C. Gordon
 * Copyright (c) 2002, Xiph.org Foundation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "common.h"
#include <altivec.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "a52.h"
#include "mdct.h"
#include "altivec_common.h"
#include "mem.h"

// sign change constants
static const vec_u32_t vPNNP = (vec_u32_t)
    (0x00000000, 0x80000000, 0x80000000, 0x00000000);
static const vec_u32_t vPNPN = (vec_u32_t)
    (0x00000000, 0x80000000, 0x00000000, 0x80000000);
static const vec_u32_t vNNNN = (vec_u32_t)
    (0x80000000, 0x80000000, 0x80000000, 0x80000000);


static inline
void mdct_butterfly_8_altivec(FLOAT *x){
    vector float x0to3, x4to7, vPlus, vMinus, v1, v2, v3, v4;
    vec_u8_t perm0101 = VPERMUTE4(0, 1, 0, 1);
    vec_u32_t vNNPP = vec_sld(vPNNP, vPNNP, 4);

    x0to3 = vec_ld(0x00, x);
    x4to7 = vec_ld(0x10, x);

    vPlus = vec_add(x4to7, x0to3);
    vMinus = vec_sub(x4to7, x0to3);

    v2 = vec_perm(vMinus, vMinus, perm0101);
    v2 = vec_sld(v2, v2, 4);
    v4 = vec_perm(vPlus, vPlus, perm0101);

    vMinus = vec_sld(vMinus, vMinus, 8);
    vPlus = vec_sld(vPlus, vPlus, 8);
    v1 = vec_perm(vMinus, vMinus, perm0101);
    v3 = vec_perm(vPlus, vPlus, perm0101);

    v2 = vec_xor(v2, (vector float)vPNNP);
    v4 = vec_xor(v4, (vector float)vNNPP);

    x0to3 = vec_add(v1, v2);
    x4to7 = vec_add(v3, v4);

    vec_st(x0to3, 0x00, x);
    vec_st(x4to7, 0x10, x);
}

static inline void
mdct_butterfly_16_altivec(FLOAT *x)
{
    vec_u8_t perm1036 = VPERMUTE4(1, 0, 3, 6);
    vec_u8_t perm5472 = VPERMUTE4(5, 4, 7, 2);
    vector float zero = (vector float) vec_splat_u32(0);
    vector float pi2_8 = (vector float)(AFT_PI2_8, AFT_PI2_8, AFT_PI2_8, AFT_PI2_8);
    vector float x0to3, x4to7, x8to11, x12to15;
    vector float v1, v2, v3, v4, v5;

    x0to3   = vec_ld(0x00, x);
    x4to7   = vec_ld(0x10, x);
    x8to11  = vec_ld(0x20, x);
    x12to15 = vec_ld(0x30, x);

    v1 = vec_perm(x0to3, x8to11, perm1036);
    v2 = vec_perm(x0to3, x8to11, perm5472);

    v1 = vec_sub(v1, v2);
    v2 = vec_sub(x12to15, x4to7);

    v3 = vec_mergeh(v1, v1);
    v4 = vec_mergeh(v2, v2);
    v5 = vec_sld(v3, v3, 8);
    v5 = vec_sld(v5, v4, 8);

    v4 = vec_sld(v4, v4, 8);
    v3 = vec_sld(v3, v4, 8);
    v3 = vec_xor(v3, (vector float)vPNNP);

    v3 = vec_add(v5, v3);
    v3 = vec_madd(v3, pi2_8, zero);

    v1 = vec_sld(v1, v1, 8);
    v2 = vec_sld(v2, v2, 8);
    v4 = vec_sld(v3, v3, 8);
    v4 = vec_sld(v4, v1, 8);
    v5 = vec_sld(v3, v2, 8);

    v1 = vec_add(x8to11,  x0to3);
    v2 = vec_add(x12to15, x4to7);

    vec_st(v4, 0x00, x);
    vec_st(v5, 0x10, x);
    vec_st(v1, 0x20, x);
    vec_st(v2, 0x30, x);

    mdct_butterfly_8_altivec(x);
    mdct_butterfly_8_altivec(x+8);
}

static inline void
mdct_butterfly_32_altivec(FLOAT *x)
{
    vec_u8_t perm0022 = VPERMUTE4(0, 0, 2, 2);
    vec_u8_t perm1405 = VPERMUTE4(1, 4, 0, 5);
    vector float zero = (vector float) vec_splat_u32(0);
    vector float cpi = (vector float) (AFT_PI2_8, AFT_PI2_8, AFT_PI1_8, AFT_PI3_8);
    vec_u32_t vNPNP = vec_sld(vPNPN, vPNPN, 4);
    vector float x0to3, x4to7, x8to11, x12to15, x16to19, x20to23, x24to27, x28to31;
    vector float pi3122, pi1322, pi1313, pi3131;
    vector float v1, v2, v3, v4;

    pi1322 = vec_sld(cpi, cpi, 8);
    pi1313 = vec_sld(cpi, pi1322, 8);
    pi3131 = vec_sld(pi1313, pi1313, 4);
    pi3122 = vec_sld(pi3131, cpi, 8);

    x0to3   = vec_ld(0x00, x);
    x4to7   = vec_ld(0x10, x);
    x8to11  = vec_ld(0x20, x);
    x12to15 = vec_ld(0x30, x);
    x16to19 = vec_ld(0x40, x);
    x20to23 = vec_ld(0x50, x);
    x24to27 = vec_ld(0x60, x);
    x28to31 = vec_ld(0x70, x);

    v1 = vec_sub(x0to3, x16to19);
    v2 = vec_perm(v1, v1, perm0022);
    v1 = vec_sld(v1, v1, 4);
    v1 = vec_perm(v1, v1, perm0022);

    v2 = vec_madd(v2, pi1322, zero);
    v2 = vec_xor(v2, (vector float)vPNPN);
    v1 = vec_madd(v1, pi3122, v2);

    vec_st(v1, 0x00, x);

    v1 = vec_sub(x24to27, x8to11);
    v2 = vec_perm(v1, v1, perm0022);
    v1 = vec_sld(v1, v1, 4);
    v1 = vec_perm(v1, v1, perm0022);

    v2 = vec_madd(v2, pi3122, zero);
    v1 = vec_xor(v1, (vector float)vNPNP);
    v1 = vec_madd(v1, pi1322, v2);

    vec_st(v1, 0x20, x);

    v1 = vec_sub(x4to7, x20to23);
    v2 = vec_sub(x28to31, x12to15);

    v3 = vec_perm(v1, v2, perm1405);
    v4 = vec_mergel(v3, v3);
    v3 = vec_mergeh(v3, v3);
    v4 = vec_madd(v4, pi3131, zero);
    v3 = vec_madd(v3, pi1313, zero);
    v4 = vec_xor(v4, (vector float)vPNNP);
    v3 = vec_add(v3, v4);

    v2 = vec_sld(v2, v2, 8);
    v4 = vec_sld(v3, v2, 8);

    vec_st(v4, 0x30, x);

    v1 = vec_xor(v1, (vector float)vPNNP);
    v2 = vec_sld(v1, v1, 8);
    v1 = vec_sld(v1, v2, 12);
    v3 = vec_sld(v3, v3, 8);
    v1 = vec_sld(v3, v1, 8);

    vec_st(v1, 0x10, x);

    x16to19 = vec_add(x16to19, x0to3);
    x20to23 = vec_add(x20to23, x4to7);
    x24to27 = vec_add(x24to27, x8to11);
    x28to31 = vec_add(x28to31, x12to15);

    vec_st(x16to19, 0x40, x);
    vec_st(x20to23, 0x50, x);
    vec_st(x24to27, 0x60, x);
    vec_st(x28to31, 0x70, x);

    mdct_butterfly_16_altivec(x);
    mdct_butterfly_16_altivec(x+16);
}

/* in place N point first stage butterfly */
/* XXX: equivalent to mdct_butterfly_generic w/ trigint = 4 */
static inline void
mdct_butterfly_first_altivec(FLOAT *trig, FLOAT *x, int points)
{
    vec_u8_t perm5410 = VPERMUTE4(5, 4, 1, 0);
    vec_u8_t perm4501 = VPERMUTE4(4, 5, 0, 1);
    vec_u8_t perm0022 = VPERMUTE4(0, 0, 2, 2);
    vec_u8_t perm1133 = vec_add(perm0022, vec_splat_u8(4));
    vector float zero = (vector float) vec_splat_u32(0);
    vector float vTa, vTb, vTc, vTd;
    vector float x1a, x1b, x2a, x2b;
    vector float v1, v2, v3, v4, v5, v6, v7, v8;

    FLOAT *x1 = x + points - 8;
    FLOAT *x2 = x + (points>>1) - 8;

    do {
        x1a = vec_ld(0x00, x1);
        x1b = vec_ld(0x10, x1);
        x2a = vec_ld(0x00, x2);
        x2b = vec_ld(0x10, x2);

        vTa = vec_ld(0x00, trig);
        vTb = vec_ld(0x10, trig);
        vTc = vec_ld(0x20, trig);
        vTd = vec_ld(0x30, trig);

        v5 = vec_sub(x1a, x2a);
        v6 = vec_sub(x1b, x2b);

        v1 = vec_perm(v5, v5, perm1133);
        v2 = vec_perm(v6, v6, perm1133);
        v3 = vec_perm(v5, v5, perm0022);
        v4 = vec_perm(v6, v6, perm0022);

        v5 = vec_perm(vTc, vTd, perm5410);
        v6 = vec_perm(vTa, vTb, perm5410);
        v7 = vec_perm(vTc, vTd, perm4501);
        v8 = vec_perm(vTa, vTb, perm4501);

        v3 = vec_xor(v3, (vector float)vPNPN);
        v4 = vec_xor(v4, (vector float)vPNPN);

        v1 = vec_madd(v1, v5, zero);
        v2 = vec_madd(v2, v6, zero);
        v3 = vec_madd(v3, v7, v1);
        v4 = vec_madd(v4, v8, v2);

        v1 = vec_add(x1a, x2a);
        v2 = vec_add(x1b, x2b);

        vec_st(v1, 0x00, x1);
        vec_st(v2, 0x10, x1);
        vec_st(v3, 0x00, x2);
        vec_st(v4, 0x10, x2);

        x1 -= 8;
        x2 -= 8;
        trig += 16;
    } while (x2 >= x);
}

static inline void
mdct_butterfly_generic_altivec(float *trig, float *x, int points, int trigint)
{
    vec_u8_t perm5410 = VPERMUTE4(5, 4, 1, 0);
    vec_u8_t perm4501 = VPERMUTE4(4, 5, 0, 1);
    vec_u8_t perm0022 = VPERMUTE4(0, 0, 2, 2);
    vec_u8_t perm1133 = vec_add(perm0022, vec_splat_u8(4));
    vector float zero = (vector float) vec_splat_u32(0);
    vector float vTa, vTb, vTc, vTd;
    vector float x1a, x1b, x2a, x2b;
    vector float v1, v2, v3, v4, v5, v6, v7, v8;

    FLOAT *x1 = x + points - 8;
    FLOAT *x2 = x + (points>>1) - 8;

    do {
        x1a = vec_ld(0x00, x1);
        x1b = vec_ld(0x10, x1);
        x2a = vec_ld(0x00, x2);
        x2b = vec_ld(0x10, x2);

        vTa = vec_ld(0, trig); trig += trigint;
        vTb = vec_ld(0, trig); trig += trigint;
        vTc = vec_ld(0, trig); trig += trigint;
        vTd = vec_ld(0, trig); trig += trigint;

        v5 = vec_sub(x1a, x2a);
        v6 = vec_sub(x1b, x2b);

        v1 = vec_perm(v5, v5, perm1133);
        v2 = vec_perm(v6, v6, perm1133);
        v3 = vec_perm(v5, v5, perm0022);
        v4 = vec_perm(v6, v6, perm0022);

        v5 = vec_perm(vTc, vTd, perm5410);
        v6 = vec_perm(vTa, vTb, perm5410);
        v7 = vec_perm(vTc, vTd, perm4501);
        v8 = vec_perm(vTa, vTb, perm4501);

        v3 = vec_xor(v3, (vector float)vPNPN);
        v4 = vec_xor(v4, (vector float)vPNPN);

        v1 = vec_madd(v1, v5, zero);
        v2 = vec_madd(v2, v6, zero);
        v3 = vec_madd(v3, v7, v1);
        v4 = vec_madd(v4, v8, v2);

        v1 = vec_add(x1a, x2a);
        v2 = vec_add(x1b, x2b);

        vec_st(v1, 0x00, x1);
        vec_st(v2, 0x10, x1);
        vec_st(v3, 0x00, x2);
        vec_st(v4, 0x10, x2);

        x1 -= 8;
        x2 -= 8;
    } while(x2>=x);
}

static inline void
mdct_butterflies_altivec(MDCTContext *mdct, FLOAT *x, int points)
{
    FLOAT *trig = mdct->trig;
    int stages = mdct->log2n-5;
    int i, j;

    if(--stages > 0){
        mdct_butterfly_first_altivec(trig,x,points);
        //mdct_butterfly_generic_altivec(trig,x,points,4);
    }

    for(i=1; --stages>0; i++){
        for(j=0; j<(1<<i); j++)
            mdct_butterfly_generic_altivec(trig, x+(points>>i)*j, points>>i, 4<<i);
    }

    for(j=0; j<points; j+=32)
        mdct_butterfly_32_altivec(x+j);
}

static inline void
mdct_bitreverse_altivec(MDCTContext *mdct, FLOAT *x)
{
    int    n    = mdct->n;
    int   *bit0 = mdct->bitrev;
    int   *bit1 = mdct->bitrev + mdct->n/4;
    FLOAT *w0   = x;
    FLOAT *w1   = x = w0+(n>>1);
    FLOAT *w2   =     w0+(n>>2);
    FLOAT *w3   = w2;
    FLOAT *trig0 = mdct->trig+n;
    FLOAT *trig1 = trig0+(n>>2);

    vector float vx0, vx1, vx2, vx3, v0, v1, v2, v3, v4, v5;
    vector float vPlus0, vPlus1, vMinus0, vMinus1;
    vector float vtrig0, vtrig0swap, vtrig1, vtrig1swap;

    vec_u8_t perm0145 = VPERMUTE4(0, 1, 4, 5);
    vec_u8_t perm2301 = VPERMUTE4(2, 3, 0, 1);
    vec_u8_t perm1032 = VPERMUTE4(1, 0, 3, 2);
    vec_u8_t perm4400 = VPERMUTE4(4, 4, 0, 0);
    vec_u8_t perm1414 = VPERMUTE4(1, 4, 1, 4);

    vec_u8_t perm2367 = vec_add(perm0145, vec_splat_u8(8));
    vec_u8_t perm5511 = vec_add(perm4400, vec_splat_u8(4));
    vec_u8_t perm3636 = vec_add(perm1414, vec_splat_u8(8));
    vec_u8_t perm2266, perm3377;

    vector float point5 = (vector float) (0.5f, 0.5f, 0.5f, 0.5f);
    vector float zero = (vector float) vec_splat_u32(0);
    vec_u32_t vNPNP = vec_sld(vPNPN, vPNPN, 4);

    perm2266 = vec_add(perm5511, vec_splat_u8(4));
    perm2266 = vec_sld(perm2266, perm2266, 8);
    perm3377 = vec_add(perm2266, vec_splat_u8(4));

    // unrolled by 2 from C MDCT, so that we can operate on all 4 elements
    // loaded at once rather than 2 (and working on the other 2 later),
    // and so we don't have to worry about unaligned loads
    do {
        w1 -= 4;
        w2 -= 4;
        trig1 -= 4;
        bit1 -= 4;

        vx0 = vec_ld(0, x+bit0[1]);
        vx1 = vec_ld(0, x+bit0[3]);
        vx2 = vec_ld(0, x+bit1[1]);
        vx3 = vec_ld(0, x+bit1[3]);

        vtrig0 = vec_ld(0, trig0);
        vtrig1 = vec_ld(0, trig1);

        vtrig0swap = vec_perm(vtrig0, vtrig0, perm1032);
        vtrig1swap = vec_perm(vtrig1, vtrig1, perm1032);

        v0 = vec_perm(vx0, vx3, perm2367);
        v1 = vec_perm(vx3, vx0, perm0145);
        v2 = vec_perm(vx1, vx2, perm2367);
        v3 = vec_perm(vx2, vx1, perm0145);

        vPlus0  = vec_add(v0, v1);
        vMinus0 = vec_sub(v0, v1);
        vPlus1  = vec_add(v2, v3);
        vMinus1 = vec_sub(v2, v3);

        v0 = vec_perm(vPlus0, vPlus1, perm2266);
        v1 = vec_perm(vMinus0, vMinus1, perm3377);

        v1 = vec_xor(v1, (vector float) vPNPN);
        v1 = vec_madd(v1, vtrig0swap, zero);
        v0 = vec_madd(v0, vtrig0, v1);
        v1 = vec_perm(v0, v0, perm2301);
        v1 = vec_xor(v1, (vector float) vNPNP);

        v4 = vec_perm(vPlus0, vMinus0, perm3636);
        v5 = vec_perm(vPlus1, vMinus1, perm3636);
        v2 = vec_perm(v4, v5, perm0145);
        v3 = vec_perm(v5, v4, perm0145);

        v2 = vec_madd(v2, point5, v0);
        v3 = vec_xor(v3, (vector float) vPNPN);
        v3 = vec_madd(v3, point5, v1);

        vec_st(v2, 0, w0);
        vec_st(v3, 0, w1);

        v0 = vec_perm(vPlus0, vPlus1, perm4400);
        v1 = vec_perm(vMinus0, vMinus1, perm5511);

        v1 = vec_xor(v1, (vector float) vPNPN);
        v1 = vec_madd(v1, vtrig1swap, zero);
        v0 = vec_madd(v0, vtrig1, v1);
        v1 = vec_perm(v0, v0, perm2301);
        v1 = vec_xor(v1, (vector float) vNPNP);

        v4 = vec_perm(vPlus1, vMinus1, perm1414);
        v5 = vec_perm(vPlus0, vMinus0, perm1414);
        v2 = vec_perm(v4, v5, perm0145);
        v3 = vec_perm(v5, v4, perm0145);

        v2 = vec_madd(v2, point5, v0);
        v3 = vec_xor(v3, (vector float) vPNPN);
        v3 = vec_madd(v3, point5, v1);

        vec_st(v2, 0, w2);
        vec_st(v3, 0, w3);

        bit0 += 4;
        trig0 += 4;
        w0 += 4;
        w3 += 4;
    } while (w2 > w0);
}

static void
mdct_altivec(MDCTThreadContext *tmdct, FLOAT *out, FLOAT *in)
{
    MDCTContext *mdct = tmdct->mdct;
    int n = mdct->n;
    int n2 = n>>1;
    int n4 = n>>2;
    int n8 = n>>3;
    FLOAT *w = tmdct->buffer;
    FLOAT *w2 = w+n2;
    FLOAT *x0 = in+n2+n4;
    FLOAT *x1 = x0;
    FLOAT *trig = mdct->trig + n2;
    int i;

    vec_u8_t perm3210 = VPERMUTE4(3, 2, 1, 0);
    vec_u8_t perm2301 = VPERMUTE4(2, 3, 0, 1);
    vec_u8_t perm0022 = VPERMUTE4(0, 0, 2, 2);
    vec_u8_t perm0246 = VPERMUTE4(0, 2, 4, 6);
    vec_u8_t perm6420 = VPERMUTE4(6, 4, 2, 0);
    vec_u8_t perm1357 = vec_add(perm0246, vec_splat_u8(4));
    vec_u8_t perm7531 = vec_add(perm6420, vec_splat_u8(4));
    vector unsigned char perm1133 = vec_add(perm0022, vec_splat_u8(4));
    vec_u8_t perm3175 = vec_sld(perm7531, perm7531, 8);
    vec_u8_t perm4602;

    vector float zero = (vector float) vec_splat_u32(0);
    vector float x0_0to3, x0_4to7, x1_0to3, x1_4to7, w0to3, w4to7;
    vector float trig0to3, trig4to7;
    vector float v0, v1, v2, v3, v4;

    vector float vScale = vec_ld_float(&mdct->scale);

    perm4602 = vec_perm(perm6420, perm6420, perm3210);
    perm4602 = vec_perm(perm4602, perm4602, perm2301);

    for(i=0; i<n8; i+=4) {
        x0 -= 8;
        trig -= 4;

        x0_0to3  = vec_ld(0x00, x0);
        x0_4to7  = vec_ld(0x10, x0);
        x1_0to3  = vec_ld(0x00, x1);
        x1_4to7  = vec_ld(0x10, x1);
        trig0to3 = vec_ld(0, trig);

        v0 = vec_perm(x0_0to3, x0_4to7, perm4602);
        v1 = vec_perm(x1_0to3, x1_4to7, perm3175);
        v0 = vec_add(v0, v1);

        v1 = vec_perm(trig0to3, trig0to3, perm3210);
        v2 = vec_perm(trig0to3, trig0to3, perm2301);
        v2 = vec_xor(v2, (vector float) vPNPN);

        v3 = vec_perm(v0, v0, perm0022);
        v3 = vec_madd(v3, v1, zero);
        v4 = vec_perm(v0, v0, perm1133);

        v0 = vec_madd(v4, v2, v3);

        vec_st(v0, 0, &w2[i]);

        x1 += 8;
    }

    x1 = in;
    for(; i<n2-n8; i+=4) {
        x0 -= 8;
        trig -= 4;

        x0_0to3  = vec_ld(0x00, x0);
        x0_4to7  = vec_ld(0x10, x0);
        x1_0to3  = vec_ld(0x00, x1);
        x1_4to7  = vec_ld(0x10, x1);
        trig0to3 = vec_ld(0, trig);

        v0 = vec_perm(x0_0to3, x0_4to7, perm4602);
        v1 = vec_perm(x1_0to3, x1_4to7, perm3175);
        v0 = vec_sub(v0, v1);

        v1 = vec_perm(trig0to3, trig0to3, perm3210);
        v2 = vec_perm(trig0to3, trig0to3, perm2301);
        v2 = vec_xor(v2, (vector float) vPNPN);

        v3 = vec_perm(v0, v0, perm0022);
        v3 = vec_madd(v3, v1, zero);
        v4 = vec_perm(v0, v0, perm1133);

        v0 = vec_madd(v4, v2, v3);

        vec_st(v0, 0, &w2[i]);

        x1 += 8;
    }

    x0 = in+n;
    for(; i<n2; i+=4) {
        trig -= 4;
        x0 -= 8;

        x0_0to3  = vec_ld(0x00, x0);
        x0_4to7  = vec_ld(0x10, x0);
        x1_0to3  = vec_ld(0x00, x1);
        x1_4to7  = vec_ld(0x10, x1);
        trig0to3 = vec_ld(0, trig);

        v0 = vec_perm(x0_0to3, x0_4to7, perm4602);
        v1 = vec_perm(x1_0to3, x1_4to7, perm3175);
        v0 = vec_xor(v0, (vector float) vNNNN);
        v0 = vec_sub(v0, v1);

        v1 = vec_perm(trig0to3, trig0to3, perm3210);
        v2 = vec_perm(trig0to3, trig0to3, perm2301);
        v2 = vec_xor(v2, (vector float) vPNPN);

        v3 = vec_perm(v0, v0, perm0022);
        v3 = vec_madd(v3, v1, zero);
        v4 = vec_perm(v0, v0, perm1133);

        v0 = vec_madd(v4, v2, v3);

        vec_st(v0, 0, &w2[i]);

        x1 += 8;
    }

    mdct_butterflies_altivec(mdct, w2, n2);
    mdct_bitreverse_altivec(mdct, w);

    trig = mdct->trig+n2;
    x0 = out+n2;
    for(i=0; i<n4; i+=4) {
        x0-=4;

        w0to3 = vec_ld(0x00, w);
        w4to7 = vec_ld(0x10, w);

        trig0to3 = vec_ld(0x00, trig);
        trig4to7 = vec_ld(0x10, trig);

        v0 = vec_madd(w0to3, trig0to3, zero);
        v1 = vec_madd(w4to7, trig4to7, zero);
        v2 = vec_perm(v0, v1, perm0246);
        v3 = vec_perm(v0, v1, perm1357);
        v0 = vec_add(v2, v3);
        v0 = vec_madd(v0, vScale, zero);

        v1 = vec_perm(w0to3, w4to7, perm6420);
        v2 = vec_perm(w0to3, w4to7, perm7531);
        v3 = vec_perm(trig0to3, trig4to7, perm7531);
        v4 = vec_perm(trig0to3, trig4to7, perm6420);

        v1 = vec_madd(v1, v3, zero);
        v2 = vec_madd(v2, v4, zero);
        v1 = vec_sub(v1, v2);
        v1 = vec_madd(v1, vScale, zero);

        vec_st(v0, 0, &out[i]);
        vec_st(v1, 0, x0);

        w += 8;
        trig += 8;
    }
}

static void
mdct_512_altivec(A52ThreadContext *tctx, FLOAT *out, FLOAT *in)
{
    mdct_altivec(&tctx->mdct_tctx_512, out, in);
}

static void
mdct_256_altivec(A52ThreadContext *tctx, FLOAT *out, FLOAT *in)
{
    FLOAT *coef_a = in;
    FLOAT *coef_b = in+128;
    FLOAT *xx = tctx->mdct_tctx_256.buffer1;
    int i;
    vector float v0, v1, v_coef_a, v_coef_b;

    memcpy(xx, in+64, 192 * sizeof(FLOAT));
    for(i=0; i<64; i+=4) {
        v0 = vec_ld(0, in+i);
        v0 = vec_xor(v0, (vector float) vNNNN);
        vec_st(v0, 0, xx+i+192);
    }

    mdct_altivec(&tctx->mdct_tctx_256, coef_a, xx);

    for(i=0; i<64; i+=4) {
        v0 = vec_ld(0, in+i+256+192);
        v0 = vec_xor(v0, (vector float) vNNNN);
        vec_st(v0, 0, xx+i);
    }

    memcpy(xx+64, in+256, 128 * sizeof(FLOAT));
    for(i=0; i<64; i+=4) {
        v0 = vec_ld(0, in+i+256+128);
        v0 = vec_xor(v0, (vector float) vNNNN);
        vec_st(v0, 0, xx+i+192);
    }

    mdct_altivec(&tctx->mdct_tctx_256, coef_b, xx);

    for(i=0; i<128; i+=4) {
        v_coef_a = vec_ld(0, coef_a+i);
        v_coef_b = vec_ld(0, coef_b+i);
        v0 = vec_mergeh(v_coef_a, v_coef_b);
        v1 = vec_mergel(v_coef_a, v_coef_b);
        vec_st(v0, 0, out+2*i);
        vec_st(v1, 0, out+2*i+4);
    }
}

void
mdct_init_altivec(A52Context *ctx)
{
    mdct_init(ctx);

    ctx->mdct_ctx_512.mdct = mdct_512_altivec;
    ctx->mdct_ctx_256.mdct = mdct_256_altivec;
}

static void
mdct_tctx_init_altivec(MDCTThreadContext *tmdct, int n)
{
    // internal mdct buffers
    tmdct->buffer = aligned_malloc(n * sizeof(FLOAT));
    tmdct->buffer1 = aligned_malloc(n * sizeof(FLOAT));
}

static void
mdct_tctx_close_altivec(MDCTThreadContext *tmdct)
{
    if(tmdct) {
        if(tmdct->buffer) aligned_free(tmdct->buffer);
        if(tmdct->buffer1) aligned_free(tmdct->buffer1);
    }
}

static void
mdct_thread_close_altivec(A52ThreadContext *tctx)
{
    mdct_tctx_close_altivec(&tctx->mdct_tctx_512);
    mdct_tctx_close_altivec(&tctx->mdct_tctx_256);

    aligned_free(tctx->frame.blocks[0].input_samples[0]);
}

void
mdct_thread_init_altivec(A52ThreadContext *tctx)
{
    mdct_tctx_init_altivec(&tctx->mdct_tctx_512, 512);
    mdct_tctx_init_altivec(&tctx->mdct_tctx_256, 256);

    tctx->mdct_tctx_512.mdct_thread_close = mdct_thread_close_altivec;
    tctx->mdct_tctx_256.mdct_thread_close = mdct_thread_close_altivec;

    tctx->mdct_tctx_512.mdct = &tctx->ctx->mdct_ctx_512;
    tctx->mdct_tctx_256.mdct = &tctx->ctx->mdct_ctx_256;

    tctx->frame.blocks[0].input_samples[0] =
        aligned_malloc(A52_NUM_BLOCKS * A52_MAX_CHANNELS * (256 + 512) * sizeof(FLOAT));
    alloc_block_buffers(tctx);
}
