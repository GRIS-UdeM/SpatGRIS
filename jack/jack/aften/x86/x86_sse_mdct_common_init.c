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
#include "x86_sse_mdct_common_init.h"


static const union __m128ui PCS_NNRR = {{0x80000000, 0x80000000, 0x00000000, 0x00000000}};
static const union __m128ui PCS_RNRN = {{0x00000000, 0x80000000, 0x00000000, 0x80000000}};
static const union __m128ui PCS_RRNN = {{0x00000000, 0x00000000, 0x80000000, 0x80000000}};
static const union __m128ui PCS_RNNR = {{0x80000000, 0x00000000, 0x00000000, 0x80000000}};
static const union __m128ui PCS_RRRR = {{0x80000000, 0x80000000, 0x80000000, 0x80000000}};

void
sse_mdct_ctx_init(MDCTContext *mdct, int n)
{
    int *bitrev = aligned_malloc((n/4) * sizeof(int));
    FLOAT *trig = aligned_malloc((n+n/4) * sizeof(FLOAT));
    int i;
    int n2 = (n >> 1);
    int log2n = mdct->log2n = log2i(n);
    mdct->n = n;
    mdct->trig = trig;
    mdct->bitrev = bitrev;

    // trig lookups
    for(i=0;i<n/4;i++){
        trig[i*2]      =  AFT_COS((AFT_PI/n)*(4*i));
        trig[i*2+1]    = -AFT_SIN((AFT_PI/n)*(4*i));
        trig[n2+i*2]   =  AFT_COS((AFT_PI/(2*n))*(2*i+1));
        trig[n2+i*2+1] =  AFT_SIN((AFT_PI/(2*n))*(2*i+1));
    }
    for(i=0;i<n/8;i++){
        trig[n+i*2]    =  AFT_COS((AFT_PI/n)*(4*i+2))*0.5f;
        trig[n+i*2+1]  = -AFT_SIN((AFT_PI/n)*(4*i+2))*0.5f;
    }

    // bitreverse lookup
    {
        int j, acc;
        int mask = (1 << (log2n-1)) - 1;
        int msb = (1 << (log2n-2));
        for(i=0; i<n/8; i++) {
            acc = 0;
            for(j=0; msb>>j; j++) {
                if((msb>>j)&i) {
                    acc |= (1 << j);
                }
            }
            bitrev[i*2]= ((~acc) & mask) - 1;
            bitrev[i*2+1] = acc;
        }
    }

    // MDCT scale used in AC3
    mdct->scale = -2.0f / n;
    {
        __m128  pscalem  = _mm_set_ps1(mdct->scale);
        float *T, *S;
        int n4   = n>>2;
        int n8   = n>>3;
        int j;
        /*
            for mdct_bitreverse
        */
        T    = aligned_malloc(sizeof(*T)*n2);
        mdct->trig_bitreverse    = T;
        S    = mdct->trig+n;
        for(i=0;i<n4;i+=8)
        {
            __m128  XMM0     = _mm_load_ps(S+i   );
            __m128  XMM1     = _mm_load_ps(S+i+ 4);
            __m128  XMM2     = XMM0;
            __m128  XMM3     = XMM1;
            XMM2     = _mm_shuffle_ps(XMM2, XMM2, _MM_SHUFFLE(2,3,0,1));
            XMM3     = _mm_shuffle_ps(XMM3, XMM3, _MM_SHUFFLE(2,3,0,1));
            XMM2     = _mm_xor_ps(XMM2, PCS_RNRN.v);
            XMM3     = _mm_xor_ps(XMM3, PCS_RNRN.v);
            _mm_store_ps(T+i*2   , XMM0);
            _mm_store_ps(T+i*2+ 4, XMM2);
            _mm_store_ps(T+i*2+ 8, XMM1);
            _mm_store_ps(T+i*2+12, XMM3);
        }
        /*
            for mdct_forward part 0
        */
        T    = aligned_malloc(sizeof(*T)*(n*2));
        mdct->trig_forward   = T;
        S    = mdct->trig;
        for(i=0,j=n2-4;i<n8;i+=4,j-=4)
        {
            __m128  XMM0, XMM1, XMM2, XMM3;
#ifdef __INTEL_COMPILER
#pragma warning(disable : 592)
#endif
            XMM0     = _mm_loadl_pi(XMM0, (__m64*)(S+j+2));
            XMM2     = _mm_loadl_pi(XMM2, (__m64*)(S+j  ));
            XMM0     = _mm_loadh_pi(XMM0, (__m64*)(S+i  ));
            XMM2     = _mm_loadh_pi(XMM2, (__m64*)(S+i+2));
#ifdef __INTEL_COMPILER
#pragma warning(default : 592)
#endif
            XMM1     = XMM0;
            XMM3     = XMM2;
            XMM0     = _mm_shuffle_ps(XMM0, XMM0, _MM_SHUFFLE(2,3,0,1));
            XMM2     = _mm_shuffle_ps(XMM2, XMM2, _MM_SHUFFLE(2,3,0,1));
            XMM0     = _mm_xor_ps(XMM0, PCS_RRNN.v);
            XMM1     = _mm_xor_ps(XMM1, PCS_RNNR.v);
            XMM2     = _mm_xor_ps(XMM2, PCS_RRNN.v);
            XMM3     = _mm_xor_ps(XMM3, PCS_RNNR.v);
            _mm_store_ps(T+i*4   , XMM0);
            _mm_store_ps(T+i*4+ 4, XMM1);
            _mm_store_ps(T+i*4+ 8, XMM2);
            _mm_store_ps(T+i*4+12, XMM3);
        }
        for(;i<n4;i+=4,j-=4)
        {
            __m128  XMM0, XMM1, XMM2, XMM3;
#ifdef __INTEL_COMPILER
#pragma warning(disable : 592)
#endif
            XMM0     = _mm_loadl_pi(XMM0, (__m64*)(S+j+2));
            XMM2     = _mm_loadl_pi(XMM2, (__m64*)(S+j  ));
            XMM0     = _mm_loadh_pi(XMM0, (__m64*)(S+i  ));
            XMM2     = _mm_loadh_pi(XMM2, (__m64*)(S+i+2));
#ifdef __INTEL_COMPILER
#pragma warning(default : 592)
#endif
            XMM1     = XMM0;
            XMM3     = XMM2;
            XMM0     = _mm_shuffle_ps(XMM0, XMM0, _MM_SHUFFLE(2,3,0,1));
            XMM2     = _mm_shuffle_ps(XMM2, XMM2, _MM_SHUFFLE(2,3,0,1));
            XMM0     = _mm_xor_ps(XMM0, PCS_NNRR.v);
            XMM2     = _mm_xor_ps(XMM2, PCS_NNRR.v);
            XMM1     = _mm_xor_ps(XMM1, PCS_RNNR.v);
            XMM3     = _mm_xor_ps(XMM3, PCS_RNNR.v);
            _mm_store_ps(T+i*4   , XMM0);
            _mm_store_ps(T+i*4+ 4, XMM1);
            _mm_store_ps(T+i*4+ 8, XMM2);
            _mm_store_ps(T+i*4+12, XMM3);
        }
        /*
            for mdct_forward part 1
        */
        T    = mdct->trig_forward+n;
        S    = mdct->trig+n2;
        for(i=0;i<n4;i+=4){
            __m128  XMM0, XMM1, XMM2;
            XMM0     = _mm_load_ps(S+4);
            XMM2     = _mm_load_ps(S  );
            XMM1     = XMM0;
            XMM0     = _mm_shuffle_ps(XMM0, XMM2,_MM_SHUFFLE(1,3,1,3));
            XMM1     = _mm_shuffle_ps(XMM1, XMM2,_MM_SHUFFLE(0,2,0,2));
            XMM0     = _mm_mul_ps(XMM0, pscalem);
            XMM1     = _mm_mul_ps(XMM1, pscalem);
            _mm_store_ps(T   , XMM0);
            _mm_store_ps(T+ 4, XMM1);
            XMM1     = _mm_shuffle_ps(XMM1, XMM1, _MM_SHUFFLE(0,1,2,3));
            XMM0     = _mm_shuffle_ps(XMM0, XMM0, _MM_SHUFFLE(0,1,2,3));
            _mm_store_ps(T+ 8, XMM1);
            _mm_store_ps(T+12, XMM0);
            S       += 8;
            T       += 16;
        }
        /*
            for mdct_butterfly_first
        */
        S    = mdct->trig;
        T    = aligned_malloc(sizeof(*T)*n*2);
        mdct->trig_butterfly_first   = T;
        for(i=0;i<n4;i+=4)
        {
            __m128  XMM0, XMM1, XMM2, XMM3, XMM4, XMM5;
            XMM2     = _mm_load_ps(S   );
            XMM0     = _mm_load_ps(S+ 4);
            XMM5     = _mm_load_ps(S+ 8);
            XMM3     = _mm_load_ps(S+12);
            XMM1     = XMM0;
            XMM4     = XMM3;
            XMM0     = _mm_shuffle_ps(XMM0, XMM2, _MM_SHUFFLE(0,1,0,1));
            XMM1     = _mm_shuffle_ps(XMM1, XMM2, _MM_SHUFFLE(1,0,1,0));
            XMM3     = _mm_shuffle_ps(XMM3, XMM5, _MM_SHUFFLE(0,1,0,1));
            XMM4     = _mm_shuffle_ps(XMM4, XMM5, _MM_SHUFFLE(1,0,1,0));
            XMM1     = _mm_xor_ps(XMM1, PCS_RNRN.v);
            XMM4     = _mm_xor_ps(XMM4, PCS_RNRN.v);
            _mm_store_ps(T   , XMM1);
            _mm_store_ps(T+ 4, XMM4);
            _mm_store_ps(T+ 8, XMM0);
            _mm_store_ps(T+12, XMM3);
            XMM1     = _mm_xor_ps(XMM1, PCS_RRRR.v);
            XMM4     = _mm_xor_ps(XMM4, PCS_RRRR.v);
            XMM0     = _mm_xor_ps(XMM0, PCS_RRRR.v);
            XMM3     = _mm_xor_ps(XMM3, PCS_RRRR.v);
            _mm_store_ps(T+n   , XMM1);
            _mm_store_ps(T+n+ 4, XMM4);
            _mm_store_ps(T+n+ 8, XMM0);
            _mm_store_ps(T+n+12, XMM3);
            S   += 16;
            T   += 16;
        }
        /*
            for mdct_butterfly_generic(trigint=8)
        */
        S    = mdct->trig;
        T    = aligned_malloc(sizeof(*T)*n2);
        mdct->trig_butterfly_generic8    = T;
        for(i=0;i<n;i+=32)
        {
            __m128  XMM0, XMM1, XMM2, XMM3, XMM4, XMM5;

            XMM0     = _mm_load_ps(S+ 24);
            XMM2     = _mm_load_ps(S+ 16);
            XMM3     = _mm_load_ps(S+  8);
            XMM5     = _mm_load_ps(S    );
            XMM1     = XMM0;
            XMM4     = XMM3;
            XMM0     = _mm_shuffle_ps(XMM0, XMM2, _MM_SHUFFLE(0,1,0,1));
            XMM1     = _mm_shuffle_ps(XMM1, XMM2, _MM_SHUFFLE(1,0,1,0));
            XMM3     = _mm_shuffle_ps(XMM3, XMM5, _MM_SHUFFLE(0,1,0,1));
            XMM4     = _mm_shuffle_ps(XMM4, XMM5, _MM_SHUFFLE(1,0,1,0));
            XMM1     = _mm_xor_ps(XMM1, PCS_RNRN.v);
            XMM4     = _mm_xor_ps(XMM4, PCS_RNRN.v);
            _mm_store_ps(T   , XMM0);
            _mm_store_ps(T+ 4, XMM1);
            _mm_store_ps(T+ 8, XMM3);
            _mm_store_ps(T+12, XMM4);
            S   += 32;
            T   += 16;
        }
        /*
            for mdct_butterfly_generic(trigint=16)
        */
        S    = mdct->trig;
        T    = aligned_malloc(sizeof(*T)*n4);
        mdct->trig_butterfly_generic16   = T;
        for(i=0;i<n;i+=64)
        {
            __m128  XMM0, XMM1, XMM2, XMM3, XMM4, XMM5;

            XMM0     = _mm_load_ps(S+ 48);
            XMM2     = _mm_load_ps(S+ 32);
            XMM3     = _mm_load_ps(S+ 16);
            XMM5     = _mm_load_ps(S    );
            XMM1     = XMM0;
            XMM4     = XMM3;
            XMM0     = _mm_shuffle_ps(XMM0, XMM2, _MM_SHUFFLE(0,1,0,1));
            XMM1     = _mm_shuffle_ps(XMM1, XMM2, _MM_SHUFFLE(1,0,1,0));
            XMM3     = _mm_shuffle_ps(XMM3, XMM5, _MM_SHUFFLE(0,1,0,1));
            XMM4     = _mm_shuffle_ps(XMM4, XMM5, _MM_SHUFFLE(1,0,1,0));
            XMM1     = _mm_xor_ps(XMM1, PCS_RNRN.v);
            XMM4     = _mm_xor_ps(XMM4, PCS_RNRN.v);
            _mm_store_ps(T   , XMM0);
            _mm_store_ps(T+ 4, XMM1);
            _mm_store_ps(T+ 8, XMM3);
            _mm_store_ps(T+12, XMM4);
            S   += 64;
            T   += 16;
        }
        /*
            for mdct_butterfly_generic(trigint=32)
        */
        if(n<128)
            mdct->trig_butterfly_generic32   = NULL;
        else
        {
            S    = mdct->trig;
            T    = aligned_malloc(sizeof(*T)*n8);
            mdct->trig_butterfly_generic32   = T;
            for(i=0;i<n;i+=128)
            {
                __m128  XMM0, XMM1, XMM2, XMM3, XMM4, XMM5;

                XMM0     = _mm_load_ps(S+ 96);
                XMM2     = _mm_load_ps(S+ 64);
                XMM3     = _mm_load_ps(S+ 32);
                XMM5     = _mm_load_ps(S    );
                XMM1     = XMM0;
                XMM4     = XMM3;
                XMM0     = _mm_shuffle_ps(XMM0, XMM2, _MM_SHUFFLE(0,1,0,1));
                XMM1     = _mm_shuffle_ps(XMM1, XMM2, _MM_SHUFFLE(1,0,1,0));
                XMM3     = _mm_shuffle_ps(XMM3, XMM5, _MM_SHUFFLE(0,1,0,1));
                XMM4     = _mm_shuffle_ps(XMM4, XMM5, _MM_SHUFFLE(1,0,1,0));
                XMM1     = _mm_xor_ps(XMM1, PCS_RNRN.v);
                XMM4     = _mm_xor_ps(XMM4, PCS_RNRN.v);
                _mm_store_ps(T   , XMM0);
                _mm_store_ps(T+ 4, XMM1);
                _mm_store_ps(T+ 8, XMM3);
                _mm_store_ps(T+12, XMM4);
                S   += 128;
                T   += 16;
            }
        }
        /*
            for mdct_butterfly_generic(trigint=64)
        */
        if(n<256)
            mdct->trig_butterfly_generic64   = NULL;
        else
        {
            S    = mdct->trig;
            T    = aligned_malloc(sizeof(*T)*(n8>>1));
            mdct->trig_butterfly_generic64   = T;
            for(i=0;i<n;i+=256)
            {
                __m128  XMM0, XMM1, XMM2, XMM3, XMM4, XMM5;

                XMM0     = _mm_load_ps(S+192);
                XMM2     = _mm_load_ps(S+128);
                XMM3     = _mm_load_ps(S+ 64);
                XMM5     = _mm_load_ps(S    );
                XMM1     = XMM0;
                XMM4     = XMM3;
                XMM0     = _mm_shuffle_ps(XMM0, XMM2, _MM_SHUFFLE(0,1,0,1));
                XMM1     = _mm_shuffle_ps(XMM1, XMM2, _MM_SHUFFLE(1,0,1,0));
                XMM3     = _mm_shuffle_ps(XMM3, XMM5, _MM_SHUFFLE(0,1,0,1));
                XMM4     = _mm_shuffle_ps(XMM4, XMM5, _MM_SHUFFLE(1,0,1,0));
                XMM1     = _mm_xor_ps(XMM1, PCS_RNRN.v);
                XMM4     = _mm_xor_ps(XMM4, PCS_RNRN.v);
                _mm_store_ps(T   , XMM0);
                _mm_store_ps(T+ 4, XMM1);
                _mm_store_ps(T+ 8, XMM3);
                _mm_store_ps(T+12, XMM4);
                S   += 256;
                T   += 16;
            }
        }
    }
}

void
sse_mdct_ctx_close(MDCTContext *mdct)
{
    if(mdct) {
        if(mdct->trig)   aligned_free(mdct->trig);
        if(mdct->bitrev) aligned_free(mdct->bitrev);
        if(mdct->trig_bitreverse) aligned_free(mdct->trig_bitreverse);
        if(mdct->trig_forward) aligned_free(mdct->trig_forward);
        if(mdct->trig_butterfly_first) aligned_free(mdct->trig_butterfly_first);
        if(mdct->trig_butterfly_generic8) aligned_free(mdct->trig_butterfly_generic8);
        if(mdct->trig_butterfly_generic16) aligned_free(mdct->trig_butterfly_generic16);
        if(mdct->trig_butterfly_generic32) aligned_free(mdct->trig_butterfly_generic32);
        if(mdct->trig_butterfly_generic64) aligned_free(mdct->trig_butterfly_generic64);
        memset(mdct, 0, sizeof(MDCTContext));
    }
}

void
sse_mdct_tctx_init(MDCTThreadContext *tmdct, int n)
{
    // internal mdct buffers
    tmdct->buffer = aligned_malloc((n+2) * sizeof(FLOAT));/* +2 to prevent illegal read in bitreverse*/
    tmdct->buffer1 = aligned_malloc(n * sizeof(FLOAT));
}

void
sse_mdct_tctx_close(MDCTThreadContext *tmdct)
{
    if(tmdct) {
        if(tmdct->buffer) aligned_free(tmdct->buffer);
        if(tmdct->buffer1) aligned_free(tmdct->buffer1);
    }
}
