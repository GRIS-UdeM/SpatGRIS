/**
 * Aften: A/52 audio encoder
 *
 * This file is derived from libvorbis
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

/**
 * Allocates and initializes lookup tables in the MDCT context.
 * @param mdct  The MDCT context
 * @param n     Number of time-domain samples used in the MDCT transform
 */
static void
ctx_init(MDCTContext *mdct, int n)
{
    int *bitrev = calloc((n/4), sizeof(int));
    FLOAT *trig = calloc((n+n/4), sizeof(FLOAT));
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
        trig[n+i*2]    =  AFT_COS((AFT_PI/n)*(4*i+2))*FCONST(0.5);
        trig[n+i*2+1]  = -AFT_SIN((AFT_PI/n)*(4*i+2))*FCONST(0.5);
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
    mdct->scale = FCONST(-2.0) / n;
}

/** Deallocates memory use by the lookup tables in the MDCT context. */
static void
ctx_close(MDCTContext *mdct)
{
    if(mdct) {
        if(mdct->trig)    free(mdct->trig);
        if(mdct->bitrev)  free(mdct->bitrev);
        memset(mdct, 0, sizeof(MDCTContext));
    }
}

/** Allocates internal buffers for MDCT calculation. */
static void
tctx_init(MDCTThreadContext *tmdct, int n)
{
     // internal mdct buffers
    tmdct->buffer = calloc(n, sizeof(FLOAT));
    tmdct->buffer1 = calloc(n, sizeof(FLOAT));
}

/** Deallocates internal buffers for MDCT calculation. */
static void
tctx_close(MDCTThreadContext *tmdct)
{
    if(tmdct) {
        if(tmdct->buffer)  free(tmdct->buffer);
        if(tmdct->buffer1) free(tmdct->buffer1);
    }
}

/** 8 point butterfly (in place, 4 register) */
static inline void
mdct_butterfly_8(FLOAT *x) {
    FLOAT r0 = x[6] + x[2];
    FLOAT r1 = x[6] - x[2];
    FLOAT r2 = x[4] + x[0];
    FLOAT r3 = x[4] - x[0];

    x[6] = r0 + r2;
    x[4] = r0 - r2;

    r0   = x[5] - x[1];
    r2   = x[7] - x[3];
    x[0] = r1   + r0;
    x[2] = r1   - r0;

    r0   = x[5] + x[1];
    r1   = x[7] + x[3];
    x[3] = r2   + r3;
    x[1] = r2   - r3;
    x[7] = r1   + r0;
    x[5] = r1   - r0;
}

/** 16 point butterfly (in place, 4 register) */
static inline void
mdct_butterfly_16(FLOAT *x) {
    FLOAT r0 = x[1] - x[9];
    FLOAT r1 = x[0] - x[8];

    x[8]  += x[0];
    x[9]  += x[1];
    x[0]   = ((r0 + r1) * AFT_PI2_8);
    x[1]   = ((r0 - r1) * AFT_PI2_8);

    r0     = x[3]  - x[11];
    r1     = x[10] - x[2];
    x[10] += x[2];
    x[11] += x[3];
    x[2]   = r0;
    x[3]   = r1;

    r0     = x[12] - x[4];
    r1     = x[13] - x[5];
    x[12] += x[4];
    x[13] += x[5];
    x[4]   = ((r0 - r1) * AFT_PI2_8);
    x[5]   = ((r0 + r1) * AFT_PI2_8);

    r0     = x[14] - x[6];
    r1     = x[15] - x[7];
    x[14] += x[6];
    x[15] += x[7];
    x[6]   = r0;
    x[7]   = r1;

    mdct_butterfly_8(x);
    mdct_butterfly_8(x+8);
}

/** 32 point butterfly (in place, 4 register) */
static inline void
mdct_butterfly_32(FLOAT *x) {
    FLOAT r0 = x[30] - x[14];
    FLOAT r1 = x[31] - x[15];

    x[30] += x[14];
    x[31] += x[15];
    x[14]  = r0;
    x[15]  = r1;

    r0     = x[28] - x[12];
    r1     = x[29] - x[13];
    x[28] +=         x[12];
    x[29] +=         x[13];
    x[12]  = (r0 * AFT_PI1_8 - r1 * AFT_PI3_8);
    x[13]  = (r0 * AFT_PI3_8 + r1 * AFT_PI1_8);

    r0     = x[26] - x[10];
    r1     = x[27] - x[11];
    x[26] +=         x[10];
    x[27] +=         x[11];
    x[10]  = ((r0 - r1) * AFT_PI2_8);
    x[11]  = ((r0 + r1) * AFT_PI2_8);

    r0     = x[24] - x[8];
    r1     = x[25] - x[9];
    x[24] += x[8];
    x[25] += x[9];
    x[8]   = (r0 * AFT_PI3_8 - r1 * AFT_PI1_8);
    x[9]   = (r1 * AFT_PI3_8 + r0 * AFT_PI1_8);

    r0     = x[22] - x[6];
    r1     = x[7]  - x[23];
    x[22] += x[6];
    x[23] += x[7];
    x[6]   = r1;
    x[7]   = r0;

    r0     = x[4] - x[20];
    r1     = x[5] - x[21];
    x[20] += x[4];
    x[21] += x[5];
    x[4]   = (r1 * AFT_PI1_8 + r0 * AFT_PI3_8);
    x[5]   = (r1 * AFT_PI3_8 - r0 * AFT_PI1_8);

    r0     = x[2]  - x[18];
    r1     = x[3]  - x[19];
    x[18] += x[2];
    x[19] += x[3];
    x[2]   = ((r1 + r0) * AFT_PI2_8);
    x[3]   = ((r1 - r0) * AFT_PI2_8);

    r0     = x[0] - x[16];
    r1     = x[1] - x[17];
    x[16] += x[0];
    x[17] += x[1];
    x[0]   = (r1 * AFT_PI3_8 + r0 * AFT_PI1_8);
    x[1]   = (r1 * AFT_PI1_8 - r0 * AFT_PI3_8);

    mdct_butterfly_16(x);
    mdct_butterfly_16(x+16);
}

/** N-point first stage butterfly (in place, 2 register) */
static inline void
mdct_butterfly_first(FLOAT *trig, FLOAT *x, int points)
{
    FLOAT *x1 = x + points - 8;
    FLOAT *x2 = x + (points>>1) - 8;
    FLOAT r0;
    FLOAT r1;

    do {
        r0     = x1[6] - x2[6];
        r1     = x1[7] - x2[7];
        x1[6] += x2[6];
        x1[7] += x2[7];
        x2[6]  = (r1 * trig[1] + r0 * trig[0]);
        x2[7]  = (r1 * trig[0] - r0 * trig[1]);

        r0     = x1[4] - x2[4];
        r1     = x1[5] - x2[5];
        x1[4] += x2[4];
        x1[5] += x2[5];
        x2[4]  = (r1 * trig[5] + r0 * trig[4]);
        x2[5]  = (r1 * trig[4] - r0 * trig[5]);

        r0     = x1[2] - x2[2];
        r1     = x1[3] - x2[3];
        x1[2] += x2[2];
        x1[3] += x2[3];
        x2[2]  = (r1 * trig[9] + r0 * trig[8]);
        x2[3]  = (r1 * trig[8] - r0 * trig[9]);

        r0     = x1[0] - x2[0];
        r1     = x1[1] - x2[1];
        x1[0] += x2[0];
        x1[1] += x2[1];
        x2[0]  = (r1 * trig[13] + r0 * trig[12]);
        x2[1]  = (r1 * trig[12] - r0 * trig[13]);

        x1 -= 8;
        x2 -= 8;
        trig += 16;
    } while(x2 >= x);
}

/** N/stage point generic N stage butterfly (in place, 2 register) */
static inline void
mdct_butterfly_generic(FLOAT *trig, FLOAT *x, int points, int trigint)
{
    FLOAT *x1 = x + points - 8;
    FLOAT *x2 = x + (points>>1) - 8;
    FLOAT r0;
    FLOAT r1;

    do {
        r0     = x1[6] - x2[6];
        r1     = x1[7] - x2[7];
        x1[6] += x2[6];
        x1[7] += x2[7];
        x2[6]  = (r1 * trig[1] + r0 * trig[0]);
        x2[7]  = (r1 * trig[0] - r0 * trig[1]);

        trig += trigint;

        r0     = x1[4] - x2[4];
        r1     = x1[5] - x2[5];
        x1[4] += x2[4];
        x1[5] += x2[5];
        x2[4]  = (r1 * trig[1] + r0 * trig[0]);
        x2[5]  = (r1 * trig[0] - r0 * trig[1]);

        trig += trigint;

        r0     = x1[2] - x2[2];
        r1     = x1[3] - x2[3];
        x1[2] += x2[2];
        x1[3] += x2[3];
        x2[2]  = (r1 * trig[1] + r0 * trig[0]);
        x2[3]  = (r1 * trig[0] - r0 * trig[1]);

        trig += trigint;

        r0     = x1[0] - x2[0];
        r1     = x1[1] - x2[1];
        x1[0] += x2[0];
        x1[1] += x2[1];
        x2[0]  = (r1 * trig[1] + r0 * trig[0]);
        x2[1]  = (r1 * trig[0] - r0 * trig[1]);

        trig += trigint;
        x1 -= 8;
        x2 -= 8;
    } while(x2 >= x);
}

static inline void
mdct_butterflies(MDCTContext *mdct, FLOAT *x, int points)
{
    FLOAT *trig = mdct->trig;
    int stages = mdct->log2n-5;
    int i, j;

    if(--stages > 0) {
        mdct_butterfly_first(trig, x, points);
    }

    for(i=1; --stages>0; i++) {
        for(j=0; j<(1<<i); j++)
            mdct_butterfly_generic(trig, x+(points>>i)*j, points>>i, 4<<i);
    }

    for(j=0; j<points; j+=32)
        mdct_butterfly_32(x+j);
}

static inline void
mdct_bitreverse(MDCTContext *mdct, FLOAT *x)
{
    int n = mdct->n;
    int *bit = mdct->bitrev;
    FLOAT *w0 = x;
    FLOAT *w1 = x = w0+(n>>1);
    FLOAT *trig = mdct->trig+n;

    do{
        FLOAT *x0 = x+bit[0];
        FLOAT *x1 = x+bit[1];

        FLOAT  r0 = x0[1]- x1[1];
        FLOAT  r1 = x0[0]+ x1[0];
        FLOAT  r2 = (r1 * trig[0] + r0 * trig[1]);
        FLOAT  r3 = (r1 * trig[1] - r0 * trig[0]);

        w1 -= 4;

        r0 = (x0[1] + x1[1]) * FCONST(0.5);
        r1 = (x0[0] - x1[0]) * FCONST(0.5);

        w0[0] = r0 + r2;
        w1[2] = r0 - r2;
        w0[1] = r1 + r3;
        w1[3] = r3 - r1;

        x0 = x+bit[2];
        x1 = x+bit[3];

        r0 = x0[1] - x1[1];
        r1 = x0[0] + x1[0];
        r2 = (r1 * trig[2] + r0 * trig[3]);
        r3 = (r1 * trig[3] - r0 * trig[2]);

        r0 = (x0[1] + x1[1]) * FCONST(0.5);
        r1 = (x0[0] - x1[0]) * FCONST(0.5);

        w0[2] = r0 + r2;
        w1[0] = r0 - r2;
        w0[3] = r1 + r3;
        w1[1] = r3 - r1;

        trig += 4;
        bit += 4;
        w0 += 4;
    } while(w0 < w1);
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
    FLOAT *x0 = in+n2+n4;
    FLOAT *x1 = x0+1;
    FLOAT *trig = mdct->trig + n2;
    FLOAT r0;
    FLOAT r1;
    int i;

    for(i=0; i<n8; i+=2) {
        x0 -= 4;
        trig -= 2;
        r0 = x0[2] + x1[0];
        r1 = x0[0] + x1[2];
        w2[i]   = (r1*trig[1] + r0*trig[0]);
        w2[i+1] = (r1*trig[0] - r0*trig[1]);
        x1 += 4;
    }

    x1 = in+1;
    for(; i<n2-n8; i+=2) {
        trig -= 2;
        x0 -= 4;
        r0 = x0[2] - x1[0];
        r1 = x0[0] - x1[2];
        w2[i]   = (r1*trig[1] + r0*trig[0]);
        w2[i+1] = (r1*trig[0] - r0*trig[1]);
        x1 += 4;
    }

    x0 = in+n;
    for(; i<n2; i+=2) {
        trig -= 2;
        x0 -= 4;
        r0 = -x0[2] - x1[0];
        r1 = -x0[0] - x1[2];
        w2[i]   = (r1*trig[1] + r0*trig[0]);
        w2[i+1] = (r1*trig[0] - r0*trig[1]);
        x1 += 4;
    }

    mdct_butterflies(mdct, w2, n2);
    mdct_bitreverse(mdct, w);

    trig = mdct->trig+n2;
    x0 = out+n2;
    for(i=0; i<n4; i++) {
        x0--;
        out[i] = ((w[0]*trig[0]+w[1]*trig[1])*mdct->scale);
        x0[0]  = ((w[0]*trig[1]-w[1]*trig[0])*mdct->scale);
        w += 2;
        trig += 2;
    }
}

static void
mdct_512(A52ThreadContext *tctx, FLOAT *out, FLOAT *in)
{
    mdct(&tctx->mdct_tctx_512, out, in);
}

#if 0
static void
mdct_256(A52Context *ctx, FLOAT *out, FLOAT *in)
{
    int k, n;
    FLOAT s, a;
    FLOAT pi_128 = AFT_PI / 128.0;
    FLOAT half = 0.5;

    for(k=0; k<128; k++) {
        s = 0;
        for(n=0; n<256; n++) {
            a = pi_128 * (n+half) * (k+half);
            s += in[n] * AFT_COS(a);
        }
        out[k*2] = s / -128.0;
    }
    for(k=0; k<128; k++) {
        s = 0;
        for(n=0; n<256; n++) {
            a = pi_128 * (n+half) * (k+half) + AFT_PI * (k+half);
            s += in[n+256] * AFT_COS(a);
        }
        out[2*k+1] = s / -128.0;
    }
}
#else
static void
mdct_256(A52ThreadContext *tctx, FLOAT *out, FLOAT *in)
{
    FLOAT *coef_a = in;
    FLOAT *coef_b = in+128;
    FLOAT *xx = tctx->mdct_tctx_256.buffer1;
    int i;

    memcpy(xx, in+64, 192 * sizeof(FLOAT));
    for(i=0; i<64; i++)
        xx[i+192] = -in[i];

    mdct(&tctx->mdct_tctx_256, coef_a, xx);

    for(i=0; i<64; i++)
        xx[i] = -in[i+256+192];

    memcpy(xx+64, in+256, 128 * sizeof(FLOAT));
    for(i=0; i<64; i++)
        xx[i+192] = -in[i+256+128];

    mdct(&tctx->mdct_tctx_256, coef_b, xx);

    for(i=0; i<128; i++) {
        out[2*i  ] = coef_a[i];
        out[2*i+1] = coef_b[i];
    }
}
#endif

void
alloc_block_buffers(A52ThreadContext *tctx)
{
    A52Frame *frame = &tctx->frame;
    int i, j;

    // we alloced one continous block and
    // now interleave in and out buffers
    for (i=0; i<A52_NUM_BLOCKS; ++i) {
        if (i) {
            frame->blocks[i].input_samples[0] =
                frame->blocks[i-1].input_samples[A52_MAX_CHANNELS-1] + 512 + 256;
        }
        frame->blocks[i].mdct_coef[0] =
            frame->blocks[i].input_samples[0] + 512;

        for (j=1; j<A52_MAX_CHANNELS; ++j) {
            frame->blocks[i].input_samples[j] =
                frame->blocks[i].input_samples[j-1] + 512 + 256;
            frame->blocks[i].mdct_coef[j] =
                frame->blocks[i].mdct_coef[j-1] + 512 + 256;
        }
    }
}

static void
mdct_close(A52Context *ctx)
{
    ctx_close(&ctx->mdct_ctx_512);
    ctx_close(&ctx->mdct_ctx_256);
}

static void
mdct_thread_close(A52ThreadContext *tctx)
{
    tctx_close(&tctx->mdct_tctx_512);
    tctx_close(&tctx->mdct_tctx_256);

    free(tctx->frame.blocks[0].input_samples[0]);
}

void
mdct_init(A52Context *ctx)
{
    ctx_init(&ctx->mdct_ctx_512, 512);
    ctx_init(&ctx->mdct_ctx_256, 256);

    ctx->mdct_ctx_512.mdct = mdct_512;
    ctx->mdct_ctx_256.mdct = mdct_256;

    ctx->mdct_ctx_512.mdct_close = mdct_close;
    ctx->mdct_ctx_256.mdct_close = mdct_close;
}

void
mdct_thread_init(A52ThreadContext *tctx)
{
    tctx_init(&tctx->mdct_tctx_512, 512);
    tctx_init(&tctx->mdct_tctx_256, 256);

    tctx->mdct_tctx_512.mdct_thread_close = mdct_thread_close;
    tctx->mdct_tctx_256.mdct_thread_close = mdct_thread_close;

    tctx->mdct_tctx_512.mdct = &tctx->ctx->mdct_ctx_512;
    tctx->mdct_tctx_256.mdct = &tctx->ctx->mdct_ctx_256;

    tctx->frame.blocks[0].input_samples[0] =
        malloc(A52_NUM_BLOCKS * A52_MAX_CHANNELS * (256 + 512) * sizeof(FLOAT));
    alloc_block_buffers(tctx);
}
