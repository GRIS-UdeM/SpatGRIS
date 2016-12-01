/**
 * Aften: A/52 audio encoder
 *
 * This file is derived from libvorbis lancer patch
 * Copyright (c) 2006-2007 prakash@punnoor.de
 * Copyright (c) 2006, blacksword8192@hotmail.com
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

/**
 * @file mdct.c
 * Modified Discrete Cosine Transform
 */

#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "a52.h"
#include "mdct.h"

#include "x86_simd_support.h"


static const union __m128ui PCS_NNRR = {{0x80000000, 0x80000000, 0x00000000, 0x00000000}};
static const union __m128ui PCS_NRRN = {{0x00000000, 0x80000000, 0x80000000, 0x00000000}};
static const union __m128ui PCS_RNRN = {{0x00000000, 0x80000000, 0x00000000, 0x80000000}};
static const union __m128ui PCS_RRRR = {{0x80000000, 0x80000000, 0x80000000, 0x80000000}};
static const union __m128f PFV_0P5 = {{0.5f, 0.5f, 0.5f, 0.5f}};

/* 8 point butterfly (in place, 4 register) */
static inline void
mdct_butterfly_8(FLOAT *x) {
    __m128  XMM0, XMM1, XMM2, XMM3;
    XMM0     = _mm_load_ps(x+4);
    XMM1     = _mm_load_ps(x  );
    XMM2     = XMM0;
    XMM0     = _mm_sub_ps(XMM0, XMM1);
    XMM2     = _mm_add_ps(XMM2, XMM1);

    XMM1     = XMM0;
    XMM3     = XMM2;

    XMM0     = _mm_shuffle_ps(XMM0, XMM0, _MM_SHUFFLE(3,2,3,2));
    XMM1     = _mm_shuffle_ps(XMM1, XMM1, _MM_SHUFFLE(0,1,0,1));
    XMM2     = _mm_shuffle_ps(XMM2, XMM2, _MM_SHUFFLE(3,2,3,2));
    XMM3     = _mm_shuffle_ps(XMM3, XMM3, _MM_SHUFFLE(1,0,1,0));

    XMM1     = _mm_xor_ps(XMM1, PCS_NRRN.v);
    XMM3     = _mm_xor_ps(XMM3, PCS_NNRR.v);

    XMM0     = _mm_add_ps(XMM0, XMM1);
    XMM2     = _mm_add_ps(XMM2, XMM3);

    _mm_store_ps(x  , XMM0);
    _mm_store_ps(x+4, XMM2);
}

/* 16 point butterfly (in place, 4 register) */
static inline void
mdct_butterfly_16(FLOAT *x) {
    static _MM_ALIGN16 const float PFV0[4] = { AFT_PI2_8,  AFT_PI2_8, 1.f, -1.f};
    static _MM_ALIGN16 const float PFV1[4] = { AFT_PI2_8, -AFT_PI2_8, 0.f,  0.f};
    static _MM_ALIGN16 const float PFV2[4] = { AFT_PI2_8,  AFT_PI2_8, 1.f,  1.f};
    static _MM_ALIGN16 const float PFV3[4] = {-AFT_PI2_8,  AFT_PI2_8, 0.f,  0.f};
    __m128  XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7;

    XMM3     = _mm_load_ps(x+12);
    XMM0     = _mm_load_ps(x   );
    XMM1     = _mm_load_ps(x+ 4);
    XMM2     = _mm_load_ps(x+ 8);
    XMM4     = XMM3;
    XMM5     = XMM0;
    XMM0     = _mm_sub_ps(XMM0, XMM2);
    XMM3     = _mm_sub_ps(XMM3, XMM1);
    XMM2     = _mm_add_ps(XMM2, XMM5);
    XMM4     = _mm_add_ps(XMM4, XMM1);
    XMM1     = XMM0;
    XMM5     = XMM3;
    _mm_store_ps(x+ 8, XMM2);
    _mm_store_ps(x+12, XMM4);
    XMM0     = _mm_shuffle_ps(XMM0, XMM0, _MM_SHUFFLE(2,3,1,1));
    XMM2     = _mm_load_ps(PFV0);
    XMM1     = _mm_shuffle_ps(XMM1, XMM1, _MM_SHUFFLE(2,3,0,0));
    XMM4     = _mm_load_ps(PFV1);
    XMM3     = _mm_shuffle_ps(XMM3, XMM3, _MM_SHUFFLE(3,2,0,0));
    XMM6     = _mm_load_ps(PFV2);
    XMM5     = _mm_shuffle_ps(XMM5, XMM5, _MM_SHUFFLE(3,2,1,1));
    XMM7     = _mm_load_ps(PFV3);
    XMM0     = _mm_mul_ps(XMM0, XMM2);
    XMM1     = _mm_mul_ps(XMM1, XMM4);
    XMM3     = _mm_mul_ps(XMM3, XMM6);
    XMM5     = _mm_mul_ps(XMM5, XMM7);
    XMM0     = _mm_add_ps(XMM0, XMM1);
    XMM3     = _mm_add_ps(XMM3, XMM5);
    _mm_store_ps(x   , XMM0);
    _mm_store_ps(x+ 4, XMM3);

    mdct_butterfly_8(x);
    mdct_butterfly_8(x+8);
}

/* 32 point butterfly (in place, 4 register) */
static inline void
mdct_butterfly_32(FLOAT *x) {
    static _MM_ALIGN16 const float PFV0[4] = { -AFT_PI3_8, -AFT_PI1_8,
                                               -AFT_PI2_8, -AFT_PI2_8 };
    static _MM_ALIGN16 const float PFV1[4] = { -AFT_PI1_8,  AFT_PI3_8,
                                               -AFT_PI2_8,  AFT_PI2_8 };
    static _MM_ALIGN16 const float PFV2[4] = { -AFT_PI1_8, -AFT_PI3_8,
                                                     -1.f,        1.f };
    static _MM_ALIGN16 const float PFV3[4] = { -AFT_PI3_8,  AFT_PI1_8,
                                                      0.f,        0.f };
    static _MM_ALIGN16 const float PFV4[4] = {  AFT_PI3_8,  AFT_PI3_8,
                                                AFT_PI2_8,  AFT_PI2_8 };
    static _MM_ALIGN16 const float PFV5[4] = { -AFT_PI1_8,  AFT_PI1_8,
                                               -AFT_PI2_8,  AFT_PI2_8 };
    static _MM_ALIGN16 const float PFV6[4] = {  AFT_PI1_8,  AFT_PI3_8,
                                                      1.f,        1.f };
    static _MM_ALIGN16 const float PFV7[4] = { -AFT_PI3_8,  AFT_PI1_8,
                                                      0.f,        0.f };
    __m128  XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7;

    XMM0     = _mm_load_ps(x+16);
    XMM1     = _mm_load_ps(x+20);
    XMM2     = _mm_load_ps(x+24);
    XMM3     = _mm_load_ps(x+28);
    XMM4     = XMM0;
    XMM5     = XMM1;
    XMM6     = XMM2;
    XMM7     = XMM3;

    XMM0     = _mm_sub_ps(XMM0, PM128(x   ));
    XMM1     = _mm_sub_ps(XMM1, PM128(x+ 4));
    XMM2     = _mm_sub_ps(XMM2, PM128(x+ 8));
    XMM3     = _mm_sub_ps(XMM3, PM128(x+12));
    XMM4     = _mm_add_ps(XMM4, PM128(x   ));
    XMM5     = _mm_add_ps(XMM5, PM128(x+ 4));
    XMM6     = _mm_add_ps(XMM6, PM128(x+ 8));
    XMM7     = _mm_add_ps(XMM7, PM128(x+12));
    _mm_store_ps(x+16, XMM4);
    _mm_store_ps(x+20, XMM5);
    _mm_store_ps(x+24, XMM6);
    _mm_store_ps(x+28, XMM7);

#ifdef USE_SSE3
    XMM4     = _mm_moveldup_ps(XMM0);
    XMM5     = XMM1;
    XMM0     = _mm_movehdup_ps(XMM0);
    XMM6     = XMM2;
    XMM7     = XMM3;
#else
    XMM4     = XMM0;
    XMM5     = XMM1;
    XMM6     = XMM2;
    XMM7     = XMM3;

    XMM0     = _mm_shuffle_ps(XMM0, XMM0, _MM_SHUFFLE(3,3,1,1));
    XMM4     = _mm_shuffle_ps(XMM4, XMM4, _MM_SHUFFLE(2,2,0,0));
#endif
    XMM1     = _mm_shuffle_ps(XMM1, XMM1, _MM_SHUFFLE(2,3,1,1));
    XMM5     = _mm_shuffle_ps(XMM5, XMM5, _MM_SHUFFLE(2,3,0,0));
    XMM2     = _mm_shuffle_ps(XMM2, XMM2, _MM_SHUFFLE(2,2,1,0));
    XMM6     = _mm_shuffle_ps(XMM6, XMM6, _MM_SHUFFLE(3,3,0,1));
    XMM3     = _mm_shuffle_ps(XMM3, XMM3, _MM_SHUFFLE(3,2,0,0));
    XMM7     = _mm_shuffle_ps(XMM7, XMM7, _MM_SHUFFLE(3,2,1,1));
    XMM0     = _mm_mul_ps(XMM0, PM128(PFV0));
    XMM4     = _mm_mul_ps(XMM4, PM128(PFV1));
    XMM1     = _mm_mul_ps(XMM1, PM128(PFV2));
    XMM5     = _mm_mul_ps(XMM5, PM128(PFV3));
    XMM2     = _mm_mul_ps(XMM2, PM128(PFV4));
    XMM6     = _mm_mul_ps(XMM6, PM128(PFV5));
    XMM3     = _mm_mul_ps(XMM3, PM128(PFV6));
    XMM7     = _mm_mul_ps(XMM7, PM128(PFV7));
    XMM0     = _mm_add_ps(XMM0, XMM4);
    XMM1     = _mm_add_ps(XMM1, XMM5);
    XMM2     = _mm_add_ps(XMM2, XMM6);
    XMM3     = _mm_add_ps(XMM3, XMM7);
    _mm_store_ps(x   , XMM0);
    _mm_store_ps(x+ 4, XMM1);
    _mm_store_ps(x+ 8, XMM2);
    _mm_store_ps(x+12, XMM3);

    mdct_butterfly_16(x);
    mdct_butterfly_16(x+16);
}

/* N point first stage butterfly (in place, 2 register) */
static inline void
mdct_butterfly_first(FLOAT *trig, FLOAT *x, int points)
{
    float   *X1  = x +  points - 8;
    float   *X2  = x + (points>>1) - 8;

    do{
        __m128  XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7;
        XMM0     = _mm_load_ps(X1+4);
        XMM1     = _mm_load_ps(X1  );
        XMM2     = _mm_load_ps(X2+4);
        XMM3     = _mm_load_ps(X2  );
        XMM4     = XMM0;
        XMM5     = XMM1;
        XMM0     = _mm_sub_ps(XMM0, XMM2);
        XMM1     = _mm_sub_ps(XMM1, XMM3);
        XMM4     = _mm_add_ps(XMM4, XMM2);
        XMM5     = _mm_add_ps(XMM5, XMM3);
#ifdef USE_SSE3
        XMM2     = _mm_moveldup_ps(XMM0);
        XMM3     = _mm_moveldup_ps(XMM1);
        _mm_store_ps(X1+4, XMM4);
        _mm_store_ps(X1  , XMM5);
        XMM0     = _mm_movehdup_ps(XMM0);
        XMM1     = _mm_movehdup_ps(XMM1);
#else
        XMM2     = XMM0;
        XMM3     = XMM1;
        _mm_store_ps(X1+4, XMM4);
        _mm_store_ps(X1  , XMM5);
        XMM0     = _mm_shuffle_ps(XMM0 , XMM0 , _MM_SHUFFLE(3,3,1,1));
        XMM1     = _mm_shuffle_ps(XMM1 , XMM1 , _MM_SHUFFLE(3,3,1,1));
        XMM2     = _mm_shuffle_ps(XMM2 , XMM2 , _MM_SHUFFLE(2,2,0,0));
        XMM3     = _mm_shuffle_ps(XMM3 , XMM3 , _MM_SHUFFLE(2,2,0,0));
#endif
        XMM4     = _mm_load_ps(trig   );
        XMM5     = _mm_load_ps(trig+ 4);
        XMM6     = _mm_load_ps(trig+ 8);
        XMM7     = _mm_load_ps(trig+12);
        XMM2     = _mm_mul_ps(XMM2, XMM4);
        XMM3     = _mm_mul_ps(XMM3, XMM5);
        XMM0     = _mm_mul_ps(XMM0, XMM6);
        XMM1     = _mm_mul_ps(XMM1, XMM7);
        XMM0     = _mm_add_ps(XMM0, XMM2);
        XMM1     = _mm_add_ps(XMM1, XMM3);
        _mm_store_ps(X2+4, XMM0);
        _mm_store_ps(X2  , XMM1);
        X1  -= 8;
        X2  -= 8;
        trig+= 16;
    }while(X2>=x);
}

/* N/stage point generic N stage butterfly (in place, 2 register) */
static inline void
mdct_butterfly_generic(MDCTContext *mdct, FLOAT *x, int points, int trigint)
{
    float *T;
    float *x1    = x +  points     - 8;
    float *x2    = x + (points>>1) - 8;
    switch(trigint)
    {
    default :
        T    = mdct->trig;
        do
        {
            __m128  XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6;
            XMM0     = _mm_load_ps(x1  );
            XMM1     = _mm_load_ps(x2  );
            XMM2     = _mm_load_ps(x1+4);
            XMM3     = _mm_load_ps(x2+4);
            XMM4     = XMM0;
            XMM5     = XMM2;
            XMM0     = _mm_sub_ps(XMM0, XMM1);
            XMM2     = _mm_sub_ps(XMM2, XMM3);
            XMM4     = _mm_add_ps(XMM4, XMM1);
            XMM5     = _mm_add_ps(XMM5, XMM3);
            XMM1     = XMM0;
            XMM3     = XMM2;
            _mm_store_ps(x1  , XMM4);
            _mm_store_ps(x1+4, XMM5);
#ifdef USE_SSE3
            XMM0     = _mm_movehdup_ps(XMM0);
            XMM1     = _mm_moveldup_ps(XMM1);
            XMM2     = _mm_movehdup_ps(XMM2);
            XMM3     = _mm_moveldup_ps(XMM3);
#else
            XMM0     = _mm_shuffle_ps(XMM0, XMM0, _MM_SHUFFLE(3,3,1,1));
            XMM1     = _mm_shuffle_ps(XMM1, XMM1, _MM_SHUFFLE(2,2,0,0));
            XMM2     = _mm_shuffle_ps(XMM2, XMM2, _MM_SHUFFLE(3,3,1,1));
            XMM3     = _mm_shuffle_ps(XMM3, XMM3, _MM_SHUFFLE(2,2,0,0));
#endif
            XMM4     = _mm_load_ps(T+trigint*3);
            XMM5     = _mm_load_ps(T+trigint*3);
            XMM6     = _mm_load_ps(T+trigint*2);
            XMM1     = _mm_xor_ps(XMM1, PCS_RNRN.v);
            XMM4     = _mm_shuffle_ps(XMM4, XMM6, _MM_SHUFFLE(0,1,0,1));
            XMM5     = _mm_shuffle_ps(XMM5, XMM6, _MM_SHUFFLE(1,0,1,0));
            XMM0     = _mm_mul_ps(XMM0, XMM4);
            XMM1     = _mm_mul_ps(XMM1, XMM5);
            XMM4     = _mm_load_ps(T+trigint  );
            XMM5     = _mm_load_ps(T+trigint  );
            XMM6     = _mm_load_ps(T          );
            XMM3     = _mm_xor_ps(XMM3, PCS_RNRN.v);
            XMM4     = _mm_shuffle_ps(XMM4, XMM6, _MM_SHUFFLE(0,1,0,1));
            XMM5     = _mm_shuffle_ps(XMM5, XMM6, _MM_SHUFFLE(1,0,1,0));
            XMM2     = _mm_mul_ps(XMM2, XMM4);
            XMM3     = _mm_mul_ps(XMM3, XMM5);
            XMM0     = _mm_add_ps(XMM0, XMM1);
            XMM2     = _mm_add_ps(XMM2, XMM3);
            _mm_store_ps(x2  , XMM0);
            _mm_store_ps(x2+4, XMM2);
            T   += trigint*4;
            x1  -= 8;
            x2  -= 8;
        }
        while(x2>=x);
        return;
    case  8:
        T    = mdct->trig_butterfly_generic8;
        break;
    case 16:
        T    = mdct->trig_butterfly_generic16;
        break;
    case 32:
        T    = mdct->trig_butterfly_generic32;
        break;
    case 64:
        T    = mdct->trig_butterfly_generic64;
        break;
    }
    _mm_prefetch((char*)T   , _MM_HINT_NTA);
    do
    {
        __m128  XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7;
        _mm_prefetch((char*)(T+16), _MM_HINT_NTA);
        XMM0     = _mm_load_ps(x1  );
        XMM1     = _mm_load_ps(x2  );
        XMM2     = _mm_load_ps(x1+4);
        XMM3     = _mm_load_ps(x2+4);
        XMM4     = XMM0;
        XMM5     = XMM2;
        XMM0     = _mm_sub_ps(XMM0, XMM1);
        XMM2     = _mm_sub_ps(XMM2, XMM3);
        XMM4     = _mm_add_ps(XMM4, XMM1);
        XMM5     = _mm_add_ps(XMM5, XMM3);
#ifdef USE_SSE3
        XMM1     = _mm_moveldup_ps(XMM0);
        XMM3     = _mm_moveldup_ps(XMM2);
        _mm_store_ps(x1  , XMM4);
        _mm_store_ps(x1+4, XMM5);
        XMM0     = _mm_movehdup_ps(XMM0);
        XMM2     = _mm_movehdup_ps(XMM2);
#else
        XMM1     = XMM0;
        XMM3     = XMM2;
        XMM0     = _mm_shuffle_ps(XMM0, XMM0, _MM_SHUFFLE(3,3,1,1));
        XMM1     = _mm_shuffle_ps(XMM1, XMM1, _MM_SHUFFLE(2,2,0,0));
        _mm_store_ps(x1  , XMM4);
        _mm_store_ps(x1+4, XMM5);
        XMM2     = _mm_shuffle_ps(XMM2, XMM2, _MM_SHUFFLE(3,3,1,1));
        XMM3     = _mm_shuffle_ps(XMM3, XMM3, _MM_SHUFFLE(2,2,0,0));
#endif
        XMM4     = _mm_load_ps(T   );
        XMM5     = _mm_load_ps(T+ 4);
        XMM6     = _mm_load_ps(T+ 8);
        XMM7     = _mm_load_ps(T+12);
        XMM0     = _mm_mul_ps(XMM0, XMM4);
        XMM1     = _mm_mul_ps(XMM1, XMM5);
        XMM2     = _mm_mul_ps(XMM2, XMM6);
        XMM3     = _mm_mul_ps(XMM3, XMM7);
        XMM0     = _mm_add_ps(XMM0, XMM1);
        XMM2     = _mm_add_ps(XMM2, XMM3);
        _mm_store_ps(x2  , XMM0);
        _mm_store_ps(x2+4, XMM2);
        T   += 16;
        x1  -= 8;
        x2  -= 8;
    }
    while(x2>=x);
}

static inline void
mdct_butterflies(MDCTContext *mdct, FLOAT *x, int points)
{
    FLOAT *trig = mdct->trig_butterfly_first;
    int stages = mdct->log2n-5;
    int i, j;

    if(--stages > 0) {
        mdct_butterfly_first(trig, x, points);
    }

    for(i=1; --stages>0; i++) {
        for(j=0; j<(1<<i); j++)
            mdct_butterfly_generic(mdct, x+(points>>i)*j, points>>i, 4<<i);
    }

    for(j=0; j<points; j+=32)
        mdct_butterfly_32(x+j);
}

static inline void
mdct_bitreverse(MDCTContext *mdct, FLOAT *x)
{
    int        n   = mdct->n;
    int       *bit = mdct->bitrev;
    float *w0      = x;
    float *w1      = x = w0+(n>>1);
    float *T       = mdct->trig_bitreverse;

    do
    {
        float *x0    = x+bit[0];
        float *x1    = x+bit[1];
        float *x2    = x+bit[2];
        float *x3    = x+bit[3];

        __m128  XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7;
        w1       -= 4;

        XMM0     = _mm_lddqu_ps(x0);
        XMM1     = _mm_lddqu_ps(x1);
        XMM4     = _mm_lddqu_ps(x2);
        XMM7     = _mm_lddqu_ps(x3);
        XMM2     = XMM0;
        XMM3     = XMM1;
        XMM5     = XMM0;
        XMM6     = XMM1;

        XMM0     = _mm_shuffle_ps(XMM0, XMM4, _MM_SHUFFLE(0,1,0,1));
        XMM1     = _mm_shuffle_ps(XMM1, XMM7, _MM_SHUFFLE(0,1,0,1));
        XMM2     = _mm_shuffle_ps(XMM2, XMM4, _MM_SHUFFLE(0,0,0,0));
        XMM3     = _mm_shuffle_ps(XMM3, XMM7, _MM_SHUFFLE(0,0,0,0));
        XMM5     = _mm_shuffle_ps(XMM5, XMM4, _MM_SHUFFLE(1,1,1,1));
        XMM6     = _mm_shuffle_ps(XMM6, XMM7, _MM_SHUFFLE(1,1,1,1));
        XMM4     = _mm_load_ps(T  );
        XMM7     = _mm_load_ps(T+4);

        XMM1     = _mm_xor_ps(XMM1, PCS_RNRN.v);
        XMM2     = _mm_add_ps(XMM2, XMM3);
        XMM5     = _mm_sub_ps(XMM5, XMM6);

        XMM0     = _mm_add_ps(XMM0, XMM1);
        XMM2     = _mm_mul_ps(XMM2, XMM4);
        XMM5     = _mm_mul_ps(XMM5, XMM7);

        XMM0     = _mm_mul_ps(XMM0, PFV_0P5.v);
        XMM2     = _mm_add_ps(XMM2, XMM5);

        XMM1     = XMM0;
        XMM3     = XMM2;

#ifdef USE_SSE3
        XMM1     = _mm_xor_ps(XMM1, PCS_RNRN.v);

        XMM0     = _mm_add_ps(XMM0, XMM2);
        XMM1     = _mm_addsub_ps(XMM1, XMM3);
#else
        XMM1     = _mm_xor_ps(XMM1, PCS_RNRN.v);
        XMM3     = _mm_xor_ps(XMM3, PCS_RNRN.v);

        XMM0     = _mm_add_ps(XMM0, XMM2);
        XMM1     = _mm_sub_ps(XMM1, XMM3);
#endif
        _mm_store_ps(w0, XMM0);
        _mm_storeh_pi((__m64*)(w1  ), XMM1);
        _mm_storel_pi((__m64*)(w1+2), XMM1);

        T       += 8;
        bit     += 4;
        w0      += 4;
    }
    while(w0<w1);
}

static void
mdct(MDCTThreadContext *tmdct, FLOAT *out, FLOAT *in)
{
    MDCTContext *mdct = tmdct->mdct;
    int n = mdct->n;
    int n2 = n>>1;
    int n4 = n>>2;
    int n8 = n>>3;
    FLOAT *w = tmdct->buffer;
    FLOAT *w2 = w+n2;
    float *x0    = in+n2+n4-8;
    float *x1    = in+n2+n4;
    float *T     = mdct->trig_forward;

    int i, j;

#ifdef __INTEL_COMPILER
#pragma warning(disable : 592)
#endif
    for(i=0,j=n2-2;i<n8;i+=4,j-=4)
    {
        __m128  XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7;
        XMM0     = _mm_load_ps(x0    + 4);
        XMM4     = _mm_load_ps(x0       );
        XMM1     = _mm_load_ps(x0+i*4+ 8);
        XMM5     = _mm_load_ps(x0+i*4+12);
        XMM2     = _mm_load_ps(T   );
        XMM3     = _mm_load_ps(T+ 4);
        XMM6     = _mm_load_ps(T+ 8);
        XMM7     = _mm_load_ps(T+12);
        XMM0     = _mm_shuffle_ps(XMM0, XMM0, _MM_SHUFFLE(0,1,2,3));
        XMM4     = _mm_shuffle_ps(XMM4, XMM4, _MM_SHUFFLE(0,1,2,3));
        XMM0     = _mm_add_ps(XMM0, XMM1);
        XMM4     = _mm_add_ps(XMM4, XMM5);
        XMM1     = XMM0;
        XMM5     = XMM4;
        XMM0     = _mm_shuffle_ps(XMM0, XMM0, _MM_SHUFFLE(0,0,3,3));
        XMM1     = _mm_shuffle_ps(XMM1, XMM1, _MM_SHUFFLE(2,2,1,1));
        XMM4     = _mm_shuffle_ps(XMM4, XMM4, _MM_SHUFFLE(0,0,3,3));
        XMM5     = _mm_shuffle_ps(XMM5, XMM5, _MM_SHUFFLE(2,2,1,1));
        XMM0     = _mm_mul_ps(XMM0, XMM2);
        XMM1     = _mm_mul_ps(XMM1, XMM3);
        XMM4     = _mm_mul_ps(XMM4, XMM6);
        XMM5     = _mm_mul_ps(XMM5, XMM7);
        XMM0     = _mm_sub_ps(XMM0, XMM1);
        XMM4     = _mm_sub_ps(XMM4, XMM5);
        _mm_storel_pi((__m64*)(w2+i  ), XMM0);
        _mm_storeh_pi((__m64*)(w2+j  ), XMM0);
        _mm_storel_pi((__m64*)(w2+i+2), XMM4);
        _mm_storeh_pi((__m64*)(w2+j-2), XMM4);
        x0  -= 8;
        T   += 16;
    }

    x0   = in;
    x1   = in+n2-8;

    for(;i<n4;i+=4,j-=4)
    {
        __m128  XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7;
        XMM1     = _mm_load_ps(x1+4);
        XMM5     = _mm_load_ps(x1  );
        XMM0     = _mm_load_ps(x0  );
        XMM4     = _mm_load_ps(x0+4);
        XMM2     = _mm_load_ps(T   );
        XMM3     = _mm_load_ps(T+ 4);
        XMM6     = _mm_load_ps(T+ 8);
        XMM7     = _mm_load_ps(T+12);
        XMM1     = _mm_shuffle_ps(XMM1, XMM1, _MM_SHUFFLE(0,1,2,3));
        XMM5     = _mm_shuffle_ps(XMM5, XMM5, _MM_SHUFFLE(0,1,2,3));
        XMM0     = _mm_sub_ps(XMM0, XMM1);
        XMM4     = _mm_sub_ps(XMM4, XMM5);
        XMM1     = XMM0;
        XMM5     = XMM4;
        XMM0     = _mm_shuffle_ps(XMM0, XMM0, _MM_SHUFFLE(0,0,3,3));
        XMM1     = _mm_shuffle_ps(XMM1, XMM1, _MM_SHUFFLE(2,2,1,1));
        XMM4     = _mm_shuffle_ps(XMM4, XMM4, _MM_SHUFFLE(0,0,3,3));
        XMM5     = _mm_shuffle_ps(XMM5, XMM5, _MM_SHUFFLE(2,2,1,1));
        XMM0     = _mm_mul_ps(XMM0, XMM2);
        XMM1     = _mm_mul_ps(XMM1, XMM3);
        XMM4     = _mm_mul_ps(XMM4, XMM6);
        XMM5     = _mm_mul_ps(XMM5, XMM7);
        XMM0     = _mm_add_ps(XMM0, XMM1);
        XMM4     = _mm_add_ps(XMM4, XMM5);
        _mm_storel_pi((__m64*)(w2+i  ), XMM0);
        _mm_storeh_pi((__m64*)(w2+j  ), XMM0);
        _mm_storel_pi((__m64*)(w2+i+2), XMM4);
        _mm_storeh_pi((__m64*)(w2+j-2), XMM4);
        x0  += 8;
        x1  -= 8;
        T   += 16;
    }
#ifdef __INTEL_COMPILER
#pragma warning(default : 592)
#endif

    mdct_butterflies(mdct, w2, n2);
    mdct_bitreverse(mdct, w);

    /* roatate + window */

    T    = mdct->trig_forward+n;
    x0    =out +n2;

    for(i=0;i<n4;i+=4)
    {
        __m128  XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7;
        x0  -= 4;
        XMM0     = _mm_load_ps(w+4);
        XMM4     = _mm_load_ps(w  );
        XMM2     = XMM0;
        XMM1     = _mm_load_ps(T   );
        XMM3     = _mm_load_ps(T+ 4);
        XMM6     = _mm_load_ps(T+ 8);
        XMM7     = _mm_load_ps(T+12);
        XMM0     = _mm_shuffle_ps(XMM0, XMM4,_MM_SHUFFLE(0,2,0,2));
        XMM2     = _mm_shuffle_ps(XMM2, XMM4,_MM_SHUFFLE(1,3,1,3));
        XMM4     = XMM0;
        XMM5     = XMM2;
        XMM0     = _mm_mul_ps(XMM0, XMM1);
        XMM2     = _mm_mul_ps(XMM2, XMM3);
        XMM4     = _mm_shuffle_ps(XMM4, XMM4, _MM_SHUFFLE(0,1,2,3));
        XMM5     = _mm_shuffle_ps(XMM5, XMM5, _MM_SHUFFLE(0,1,2,3));
        XMM4     = _mm_mul_ps(XMM4, XMM6);
        XMM5     = _mm_mul_ps(XMM5, XMM7);
        XMM0     = _mm_sub_ps(XMM0, XMM2);
        XMM4     = _mm_add_ps(XMM4, XMM5);
        _mm_store_ps(x0    , XMM0);
        _mm_store_ps(out +i, XMM4);
        w   += 8;
        T   += 16;
    }
}

static void
mdct_512(A52ThreadContext *tctx, FLOAT *out, FLOAT *in)
{
    mdct(&tctx->mdct_tctx_512, out, in);
}

static void
mdct_256(A52ThreadContext *tctx, FLOAT *out, FLOAT *in)
{
    FLOAT *coef_a, *coef_b, *xx;
    int i, j;

    coef_a = in;
    coef_b = &in[128];
    xx = tctx->mdct_tctx_256.buffer1;

    memcpy(xx, in+64, 192 * sizeof(FLOAT));
    xx += 192;
    for(i=0; i<64; i+=4) {
        __m128 XMM0 = _mm_load_ps(in + i);
        XMM0 = _mm_xor_ps(XMM0, PCS_RRRR.v);
        _mm_store_ps(xx + i, XMM0);
    }
    xx -= 192;

    mdct(&tctx->mdct_tctx_256, coef_a, xx);

    in += 256 + 192;
    for(i=0; i<64; i+=4) {
        __m128 XMM0 = _mm_load_ps(in + i);
        XMM0 = _mm_xor_ps(XMM0, PCS_RRRR.v);
        _mm_store_ps(xx + i, XMM0);
    }
    in -= 256 + 192;

    memcpy(xx+64, in+256, 128 * sizeof(FLOAT));
    xx += 192;
    in += 256 + 128;
    for(i=0; i<64; i+=4) {
        __m128 XMM0 = _mm_load_ps(in + i);
        XMM0 = _mm_xor_ps(XMM0, PCS_RRRR.v);
        _mm_store_ps(xx + i, XMM0);
    }
    xx -= 192;
    in -= 256 + 128;

    mdct(&tctx->mdct_tctx_256, coef_b, xx);

    for(i=0, j=0; i<128; i+=4, j+=8) {
        __m128 XMM0 = _mm_load_ps(coef_a + i);
        __m128 XMM1 = _mm_load_ps(coef_b + i);
        __m128 XMM2 = _mm_unpacklo_ps(XMM0, XMM1);
        __m128 XMM3 = _mm_unpackhi_ps(XMM0, XMM1);
        _mm_store_ps(out + j  , XMM2);
        _mm_store_ps(out + j+4, XMM3);
    }
}
