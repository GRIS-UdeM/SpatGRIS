/**
 * Aften: A/52 audio encoder
 * Copyright (c) 2006 Justin Ruggles
 *                    Prakash Punnoor <prakash@punnoor.de>
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
 * @file bitalloc.c
 * A/52 bit allocation
 */

#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bitalloc.h"
#include "exponent.h"
#include "a52.h"
#include "aften.h"

/* log addition table */
static const uint8_t latab[260]= {
    64, 63, 62, 61, 60, 59, 58, 57, 56, 55,
    54, 53, 52, 52, 51, 50, 49, 48, 47, 47,
    46, 45, 44, 44, 43, 42, 41, 41, 40, 39,
    38, 38, 37, 36, 36, 35, 35, 34, 33, 33,
    32, 32, 31, 30, 30, 29, 29, 28, 28, 27,
    27, 26, 26, 25, 25, 24, 24, 23, 23, 22,
    22, 21, 21, 21, 20, 20, 19, 19, 19, 18,
    18, 18, 17, 17, 17, 16, 16, 16, 15, 15,
    15, 14, 14, 14, 13, 13, 13, 13, 12, 12,
    12, 12, 11, 11, 11, 11, 10, 10, 10, 10,
    10,  9,  9,  9,  9,  9,  8,  8,  8,  8,
     8,  8,  7,  7,  7,  7,  7,  7,  6,  6,
     6,  6,  6,  6,  6,  6,  5,  5,  5,  5,
     5,  5,  5,  5,  4,  4,  4,  4,  4,  4,
     4,  4,  4,  4,  4,  3,  3,  3,  3,  3,
     3,  3,  3,  3,  3,  3,  3,  3,  3,  2,
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
     2,  2,  2,  2,  2,  2,  2,  2,  1,  1,
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

/**
 * absolute hearing threshold table
 * values are in log-psd units (128 psd = -6dB)
 * each table entry corresponds to a critical frequency band
 * each entry has 3 values, 1 for each base sample rate
 * { 48kHz, 44.1kHz, 32kHz }
 */
static const uint16_t hth[50][3]= {
    { 1232, 1264, 1408 },
    { 1232, 1264, 1408 },
    { 1088, 1120, 1200 },
    { 1024, 1040, 1104 },
    {  992,  992, 1056 },
    {  960,  976, 1008 },
    {  944,  960,  992 },
    {  944,  944,  976 },
    {  928,  944,  960 },
    {  928,  928,  944 },
    {  928,  928,  944 },
    {  928,  928,  944 },
    {  928,  928,  928 },
    {  912,  928,  928 },
    {  912,  912,  928 },
    {  912,  912,  928 },
    {  896,  912,  928 },
    {  896,  896,  928 },
    {  880,  896,  928 },
    {  880,  896,  928 },
    {  864,  880,  912 },
    {  864,  880,  912 },
    {  848,  864,  912 },
    {  848,  864,  912 },
    {  832,  848,  896 },
    {  832,  848,  896 },
    {  816,  832,  896 },
    {  800,  832,  880 },
    {  784,  800,  864 },
    {  768,  784,  848 },
    {  752,  768,  832 },
    {  752,  752,  816 },
    {  752,  752,  800 },
    {  752,  752,  784 },
    {  768,  752,  768 },
    {  784,  768,  752 },
    {  832,  800,  752 },
    {  912,  848,  752 },
    {  992,  912,  768 },
    { 1056,  992,  784 },
    { 1120, 1056,  816 },
    { 1168, 1104,  848 },
    { 1184, 1184,  960 },
    { 1120, 1168, 1040 },
    { 1088, 1120, 1136 },
    { 1088, 1088, 1184 },
    { 1312, 1152, 1120 },
    { 2048, 1584, 1088 },
    { 2112, 2112, 1104 },
    { 2112, 2112, 1248 },
};

/* bit allocation pointer table */
static const uint8_t baptab[64]= {
     0,  1,  1,  1,  1,  1,  2,  2,  3,  3,  3,  4,  4,  5,  5,  6,
     6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  8,  9,  9,  9,  9, 10,
    10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 12, 13, 13, 13, 13, 14,
    14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15
};

/* slow gain table */
static const uint16_t sgaintab[4]= {
    1344, 1240, 1144, 1040,
};

/* dB per bit table */
static const uint16_t dbkneetab[4]= {
    0, 1792, 2304, 2816,
};

/* floor table */
static const int16_t floortab[8]= {
    752, 688, 624, 560, 496, 368, 240, -2048,
};

/* band size table (number of bins in each band) */
static const uint8_t bndsz[50]={
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, 1,
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  3,  3,  3,  3,  3, 3,
     3,  6,  6,  6,  6,  6,  6, 12, 12, 12, 12, 24, 24, 24, 24, 24
};

/* slow decay table */
static const uint8_t sdecaytab[4] = {
    15, 17, 19, 21
};

/* fast decay table */
static const uint8_t fdecaytab[4] = {
    63, 83, 103, 123
};

/* fast gain table */
static const uint16_t fgaintab[8] = {
    128, 256, 384, 512, 640, 768, 896, 1024
};

/* power spectral density table */
static uint16_t psdtab[25];

/* mask table (maps bin# to band#) */
static uint8_t masktab[253];

/* band table (starting bin for each band) */
static uint8_t bndtab[51];

/* frame size table */
static uint16_t frmsizetab[38][3];

void
bitalloc_init(void)
{
    int i, j, k, l, v;

    // compute psdtab
    for(i=0; i<25; i++) {
        psdtab[i] = 3072 - (i << 7);
    }

    // compute bndtab and masktab from bandsz
    k = l = i = 0;
    bndtab[i] = l;
    while(i < 50) {
        v = bndsz[i];
        for(j=0; j<v; j++) masktab[k++] = i;
        l += v;
        bndtab[++i] = l;
    }

    // compute frmsizetab (in bits)
    for(i=0; i<19; i++) {
        for(j=0; j<3; j++) {
            v = a52_bitratetab[i] * 96000 / a52_freqs[j];
            frmsizetab[i*2][j] = frmsizetab[i*2+1][j] = v * 16;
            if(j == 1) frmsizetab[i*2+1][j] += 16;
        }
    }
}

static inline int
calc_lowcomp1(int a, int b0, int b1, int c)
{
    if((b0 + 256) == b1) {
        a = c;
    } else if(b0 > b1) {
        a = MAX(a - 64, 0);
    }
    return a;
}

static inline int
calc_lowcomp(int a, int b0, int b1, int bin)
{
    if(bin < 7) {
        return calc_lowcomp1(a, b0, b1, 384);
    } else if(bin < 20) {
        return calc_lowcomp1(a, b0, b1, 320);
    } else {
        return MAX(a - 128, 0);
    }
}

/** combine psd value in multiple bins into a single psd value */
static int
psd_combine(int16_t *psd, int bins)
{
    int i, adr;
    int v;

    v = psd[0];
    for(i=1; i<bins; i++) {
        adr = MIN((ABS(v-psd[i]) >> 1), 255);
        v = MAX(v, psd[i]) + latab[adr];
    }
    return v;
}

/**
 * A52 bit allocation preparation to speed up matching left bits.
 * This generates the power-spectral densities and the masking curve based on
 * the mdct coefficient exponents and bit allocation parameters.
 */
static void
a52_bit_allocation_prepare(A52BitAllocParams *s,
                   uint8_t *exp, int16_t *psd, int16_t *mask,
                   int start, int end,
                   int deltbae,int deltnseg, uint8_t *deltoffst,
                   uint8_t *deltlen, uint8_t *deltba)
{
    int bnd, i, end1, bndstrt, bndend, lowcomp, begin;
    int fastleak, slowleak;
    int16_t bndpsd[50]; // power spectral density for critical bands
    int16_t excite[50]; // excitation function

    // exponent mapping to PSD
    for(i=start; i<end; i++) {
        psd[i] = psdtab[exp[i]];
    }

    // use log addition to combine PSD for each critical band
    bndstrt = masktab[start];
    bndend = masktab[end-1] + 1;
    i = start;
    for(bnd=bndstrt; bnd<bndend; bnd++) {
        int bins = MIN(bndtab[bnd+1], end) - i;
        bndpsd[bnd] = psd_combine(&psd[i], bins);
        i += bins;
    }

    // excitation function
    if(bndstrt == 0) {
        // fbw and lfe channels
        lowcomp = 0;
        lowcomp = calc_lowcomp(lowcomp, bndpsd[0], bndpsd[1], 0);
        excite[0] = bndpsd[0] - s->fgain - lowcomp;
        lowcomp = calc_lowcomp(lowcomp, bndpsd[1], bndpsd[2], 1);
        excite[1] = bndpsd[1] - s->fgain - lowcomp;
        begin = 7;
        for(bnd=2; bnd<7; bnd++) {
            if(bnd+1 < bndend) {
                lowcomp = calc_lowcomp(lowcomp, bndpsd[bnd], bndpsd[bnd+1], bnd);
            }
            fastleak = bndpsd[bnd] - s->fgain;
            slowleak = bndpsd[bnd] - s->sgain;
            excite[bnd] = fastleak - lowcomp;
            if(bnd+1 < bndend) {
                if(bndpsd[bnd] <= bndpsd[bnd+1]) {
                    begin = bnd + 1;
                    break;
                }
            }
        }

        end1 = MIN(bndend, 22);

        for(bnd=begin; bnd<end1; bnd++) {
            if(bnd+1 < bndend) {
                lowcomp = calc_lowcomp(lowcomp, bndpsd[bnd], bndpsd[bnd+1], bnd);
            }
            fastleak = MAX(fastleak-s->fdecay, bndpsd[bnd]-s->fgain);
            slowleak = MAX(slowleak-s->sdecay, bndpsd[bnd]-s->sgain);
            excite[bnd] = MAX(slowleak, fastleak-lowcomp);
        }
        begin = 22;
    } else {
        // coupling channel
        begin = bndstrt;
        fastleak = (s->cplfleak << 8) + 768;
        slowleak = (s->cplsleak << 8) + 768;
    }

    for(bnd=begin; bnd<bndend; bnd++) {
        fastleak = MAX(fastleak-s->fdecay, bndpsd[bnd]-s->fgain);
        slowleak = MAX(slowleak-s->sdecay, bndpsd[bnd]-s->sgain);
        excite[bnd] = MAX(slowleak, fastleak);
    }

    // compute masking curve from excitation function and hearing threshold
    for(bnd=bndstrt; bnd<bndend; bnd++) {
        if(bndpsd[bnd] < s->dbknee) {
            excite[bnd] += (s->dbknee - bndpsd[bnd]) >> 2;
        }
        mask[bnd] = MAX(excite[bnd], hth[bnd >> s->halfratecod][s->fscod]);
    }

    // delta bit allocation
    if(deltbae == 0 || deltbae == 1) {
        int seg, delta;
        bnd = 0;
        for(seg=0; seg<deltnseg; seg++) {
            bnd += deltoffst[seg];
            if(deltba[seg] >= 4) {
                delta = (deltba[seg] - 3) << 7;
            } else {
                delta = (deltba[seg] - 4) << 7;
            }
            for(i=0; i<deltlen[seg]; i++) {
                mask[bnd] += delta;
                bnd++;
            }
        }
    }
}

/**
 * A52 bit allocation
 * Generate bit allocation pointers for each mantissa, which determines the
 * number of bits allocated for each mantissa.  The fine-grain power-spectral
 * densities and the masking curve have been pre-generated in the preparation
 * step.  They are used along with the given snroffset and floor values to
 * calculate each bap value.
 */
static void
a52_bit_allocation(uint8_t *bap, int16_t *psd, int16_t *mask,
                   int start, int end, int snroffset, int floor)
{
    int i, j, endj;
    int v, address1, address2, offset;

    // csnroffst=0 & fsnroffst=0 is a special-case scenario in which all baps
    // are set to zero and the core bit allocation is skipped.
    if(snroffset == SNROFFST(0, 0)) {
        memset(&bap[start], 0, end-start);
        return;
    }

    offset = snroffset + floor;
    for (i = start, j = masktab[start]; end > bndtab[j]; ++j) {
        v = (MAX(mask[j] - offset, 0) & 0x1FE0) + floor;
        endj = MIN(bndtab[j] + bndsz[j], end);
        if ((endj-i) & 1) {
            address1 = (psd[i] - v) >> 5;
            address1 = CLIP(address1, 0, 63);
            bap[i] = baptab[address1];
            ++i;
        }
        while (i < endj) {
            address1 = (psd[i  ] - v) >> 5;
            address2 = (psd[i+1] - v) >> 5;
            address1 = CLIP(address1, 0, 63);
            address2 = CLIP(address2, 0, 63);
            bap[i  ] = baptab[address1];
            bap[i+1] = baptab[address2];
            i+=2;
        }
    }
}

/**
 * Calculate the size in bits taken by the mantissas.
 * This is determined solely by the bit allocation pointers.
 */
static int
compute_mantissa_size(int mant_cnt[5], uint8_t *bap, int ncoefs)
{
    int bits, b, i;

    bits = 0;
    for(i=0; i<ncoefs; i++) {
        b = bap[i];
        if(b <= 4) {
            // bap=1 to bap=4 will be counted in compute_mantissa_size_final
            ++mant_cnt[b];
        } else if(b <= 13) {
            // bap=5 to bap=13 use (bap-1) bits
            bits += b-1;
        } else {
            // bap=14 uses 14 bits and bap=15 uses 16 bits
            bits += 14 + ((b-14)<<1);
        }
    }
    return bits;
}

/** Finalize the mantissa bit count by adding in the grouped mantissas */
static int
compute_mantissa_size_final(int mant_cnt[5])
{
    // bap=1 : 3 mantissas in 5 bits
    int bits = (mant_cnt[1] / 3) * 5;
    // bap=2 : 3 mantissas in 7 bits
    // bap=4 : 2 mantissas in 7 bits
    bits += ((mant_cnt[2] / 3) + (mant_cnt[4] >> 1)) * 7;
    // bap=3 : each mantissa is 3 bits
    bits += mant_cnt[3] * 3;
    return bits;
}

/* call to prepare bit allocation */
static void
bit_alloc_prepare(A52ThreadContext *tctx)
{
    A52Context *ctx = tctx->ctx;
    A52Frame *frame = &tctx->frame;
    A52Block *block;
    int blk, ch;

    for(blk=0; blk<A52_NUM_BLOCKS; blk++) {
        block = &frame->blocks[blk];
        for(ch=0; ch<ctx->n_all_channels; ch++) {
            // We don't have to run the bit allocation when reusing exponents
            if(block->exp_strategy[ch] != EXP_REUSE) {
                a52_bit_allocation_prepare(&frame->bit_alloc,
                               block->exp[ch], block->psd[ch], block->mask[ch],
                               0, frame->ncoefs[ch],
                               2, 0, NULL, NULL, NULL);
            }
        }
    }
}

/**
 * Run the bit allocation routine using the given snroffset values.
 * Returns number of mantissa bits used.
 */
static int
bit_alloc(A52ThreadContext *tctx, int snroffst)
{
    A52Context *ctx = tctx->ctx;
    A52Frame *frame = &tctx->frame;
    A52Block *block;
    int mant_cnt[5];
    int blk, ch;
    int bits;

    bits = 0;
    snroffst = (snroffst << 2) - 960;

    for(blk=0; blk<A52_NUM_BLOCKS; blk++) {
        block = &frame->blocks[blk];
        // initialize grouped mantissa counts. these are set so that they are
        // padded to the next whole group size when bits are counted in
        // compute_mantissa_size_final
        mant_cnt[0] = mant_cnt[3] = 0;
        mant_cnt[1] = mant_cnt[2] = 2;
        mant_cnt[4] = 1;
        for(ch=0; ch<ctx->n_all_channels; ch++) {
            // Currently the encoder is setup so that the only bit allocation
            // parameter which varies across blocks within a frame is the
            // exponent values.  We can take advantage of that by reusing the
            // bit allocation pointers whenever we reuse exponents.
            if(block->exp_strategy[ch] == EXP_REUSE) {
                memcpy(block->bap[ch], frame->blocks[blk-1].bap[ch], 256);
            } else {
                a52_bit_allocation(block->bap[ch], block->psd[ch], block->mask[ch],
                                   0, frame->ncoefs[ch], snroffst,
                                   frame->bit_alloc.floor);
            }
            bits += compute_mantissa_size(mant_cnt, block->bap[ch], frame->ncoefs[ch]);

        }
        bits += compute_mantissa_size_final(mant_cnt);
    }

    return bits;
}

/** Counts all frame bits except for mantissas and exponents */
static void
count_frame_bits(A52ThreadContext *tctx)
{
    static int frame_bits_inc[8] = { 8, 0, 2, 2, 2, 4, 2, 4 };
    A52Context *ctx = tctx->ctx;
    A52Frame *frame = &tctx->frame;
    A52Block *block;
    int blk, ch;
    int frame_bits;

    frame_bits = 0;

    // header size
    frame_bits += 65;
    frame_bits += frame_bits_inc[ctx->acmod];
    if(ctx->meta.xbsi1e) frame_bits += 14;
    if(ctx->meta.xbsi2e) frame_bits += 14;

    // audio blocks
    for(blk=0; blk<A52_NUM_BLOCKS; blk++) {
        block = &frame->blocks[blk];
        frame_bits += ctx->n_channels * 2; // nch * (blksw[1] + dithflg[1])
        frame_bits++; // dynrnge
        if(ctx->acmod == 0) frame_bits++; // dynrng2e
        if(ctx->params.dynrng_profile != DYNRNG_PROFILE_NONE) {
            frame_bits += 8; // dynrng
            if(ctx->acmod == 0) frame_bits += 8; // dynrng2
        }
        frame_bits++; // cplstre
        if(ctx->acmod == 2) {
            frame_bits++; // rematstr
            if(block->rematstr) frame_bits += 4; // rematflg
        }
        frame_bits += 2 * ctx->n_channels; // nch * chexpstr[2]
        if(ctx->lfe) frame_bits++; // lfeexpstr
        for(ch=0; ch<ctx->n_channels; ch++) {
            if(block->exp_strategy[ch] != EXP_REUSE)
                frame_bits += 6 + 2; // chbwcod[6], gainrng[2]
        }
        frame_bits++; // baie
        frame_bits++; // snr
        frame_bits += 2; // delta / skip
    }
    frame_bits++; // cplinu for block 0

    // bit alloc info for block 0
    // sdcycod[2], fdcycod[2], sgaincod[2], dbpbcod[2], floorcod[3]
    // csnroffset[6]
    // nch * (fsnoffset[4] + fgaincod[3])
    frame_bits += 2 + 2 + 2 + 2 + 3 + 6 + ctx->n_all_channels * (4 + 3);

    // auxdatae, crcrsv
    frame_bits += 2;

    // CRC
    frame_bits += 16;

    frame->frame_bits = frame_bits;
}

/**
 * Calculates the snroffset values which, when used, keep the size of the
 * encoded data within a fixed frame size.
 */
static int
cbr_bit_allocation(A52ThreadContext *tctx, int prepare)
{
    A52Context *ctx = tctx->ctx;
    A52Frame *frame = &tctx->frame;
    int csnroffst, fsnroffst;
    int current_bits, avail_bits, leftover;
    int snroffst=0;

    current_bits = frame->frame_bits + frame->exp_bits;
    avail_bits = (16 * frame->frame_size) - current_bits;

    if(prepare)
        bit_alloc_prepare(tctx);

    // starting point
    if(ctx->params.encoding_mode == AFTEN_ENC_MODE_VBR) {
        snroffst = ctx->params.quality;
    } else if(ctx->params.encoding_mode == AFTEN_ENC_MODE_CBR) {
        snroffst = tctx->last_quality;
    }
    leftover = avail_bits - bit_alloc(tctx, snroffst);

    if(ctx->params.bitalloc_fast) {
        // fast bit allocation
        int leftover0, leftover1, snr0, snr1;
        snr0 = snr1 = snroffst;
        leftover0 = leftover1 = leftover;
        if(leftover != 0) {
            if(leftover > 0) {
                while(leftover1 > 0 && snr1+16 <= 1023) {
                    snr0 = snr1;
                    leftover0 = leftover1;
                    snr1 += 16;
                    leftover1 = avail_bits - bit_alloc(tctx, snr1);
                }
            } else {
                while(leftover0 < 0 && snr0-16 >= 0) {
                    snr1 = snr0;
                    leftover1 = leftover0;
                    snr0 -= 16;
                    leftover0 = avail_bits - bit_alloc(tctx, snr0);
                }
            }
        }
        if(snr0 != snr1) {
            snroffst = snr0;
            leftover = avail_bits - bit_alloc(tctx, snroffst);
        }
    } else {
        // take up to 3 jumps based on estimated distance from optimal
        if(leftover < -400) {
            snroffst += (leftover / (16 * ctx->n_channels));
            leftover = avail_bits - bit_alloc(tctx, snroffst);
        }
        if(leftover > 400) {
            snroffst += (leftover / (24 * ctx->n_channels));
            leftover = avail_bits - bit_alloc(tctx, snroffst);
        }
        if(leftover < -200) {
            snroffst += (leftover / (40 * ctx->n_channels));
            leftover = avail_bits - bit_alloc(tctx, snroffst);
        }
        // adjust snroffst until leftover <= -100
        while(leftover > -100) {
            snroffst += (10 / ctx->n_channels);
            if(snroffst > 1023) {
                snroffst = 1023;
                leftover = avail_bits - bit_alloc(tctx, snroffst);
                break;
            }
            leftover = avail_bits - bit_alloc(tctx, snroffst);
        }
        // adjust snroffst until leftover is positive
        while(leftover < 0 && snroffst > 0) {
            snroffst--;
            leftover = avail_bits - bit_alloc(tctx, snroffst);
        }
    }

    frame->mant_bits = avail_bits - leftover;
    if(leftover < 0) {
        fprintf(stderr, "bitrate: %d kbps too small\n", frame->bit_rate);
        return -1;
    }

    // calculate csnroffst and fsnroffst
    snroffst = (snroffst - 240);
    csnroffst = (snroffst / 16) + 15;
    fsnroffst = (snroffst % 16);
    while(fsnroffst < 0) {
        csnroffst--;
        fsnroffst += 16;
    }

    // set encoding parameters
    frame->csnroffst = csnroffst;
    frame->fsnroffst = fsnroffst;
    frame->quality = QUALITY(csnroffst, fsnroffst);
    tctx->last_quality = frame->quality;

    return 0;
}

/**
 * Finds the frame size which will hold all of the data when using an
 * snroffset value as determined by the user-selected quality setting.
 */
static int
vbr_bit_allocation(A52ThreadContext *tctx)
{
    A52Context *ctx = tctx->ctx;
    A52Frame *frame = &tctx->frame;
    int i;
    int frame_size;
    int quality, snroffst, csnroffst, fsnroffst;
    int frame_bits, current_bits;

    current_bits = frame->frame_bits + frame->exp_bits;

    // convert quality in range 0 to 1023 to csnroffst & fsnroffst
    // csnroffst has range 0 to 63, fsnroffst has range 0 to 15
    quality = ctx->params.quality;
    snroffst = (quality - 240);
    csnroffst = (snroffst / 16) + 15;
    fsnroffst = (snroffst % 16);
    while(fsnroffst < 0) {
        csnroffst--;
        fsnroffst += 16;
    }

    bit_alloc_prepare(tctx);
    // find an A52 frame size that can hold the data.
    frame_size = 0;
    frame_bits = current_bits + bit_alloc(tctx, quality);
    for(i=0; i<=ctx->frmsizecod; i++) {
        frame_size = frmsizetab[i][ctx->fscod];
        if(frame_size >= frame_bits) break;
    }
    i = MIN(i, ctx->frmsizecod);
    frame->bit_rate = a52_bitratetab[i/2] >> ctx->halfratecod;

    frame->frmsizecod = i;
    frame->frame_size = frame_size / 16;
    frame->frame_size_min = frame->frame_size;

    // run CBR bit allocation.
    // this will increase snroffst to make optimal use of the frame bits.
    // also it will lower snroffst if vbr frame won't fit in largest frame.
    return cbr_bit_allocation(tctx, 0);
}

/**
 * Loads the bit allocation parameters and counts fixed frame bits.
 */
static void
start_bit_allocation(A52ThreadContext *tctx)
{
    A52Context *ctx = tctx->ctx;
    A52Frame *frame = &tctx->frame;

    // read bit allocation table values
    frame->bit_alloc.fscod = ctx->fscod;
    frame->bit_alloc.halfratecod = ctx->halfratecod;
    frame->bit_alloc.sdecay = sdecaytab[frame->sdecaycod] >> ctx->halfratecod;
    frame->bit_alloc.fdecay = fdecaytab[frame->fdecaycod] >> ctx->halfratecod;
    frame->bit_alloc.fgain = fgaintab[frame->fgaincod];
    frame->bit_alloc.sgain = sgaintab[frame->sgaincod];
    frame->bit_alloc.dbknee = dbkneetab[frame->dbkneecod];
    frame->bit_alloc.floor = floortab[frame->floorcod];

    count_frame_bits(tctx);
}

/** estimated number of bits used for a mantissa, indexed by bap value. */
static FLOAT mant_est_tab[16] = {
    FCONST( 0.000), FCONST( 1.667),
    FCONST( 2.333), FCONST( 3.000),
    FCONST( 3.500), FCONST( 4.000),
    FCONST( 5.000), FCONST( 6.000),
    FCONST( 7.000), FCONST( 8.000),
    FCONST( 9.000), FCONST(10.000),
    FCONST(11.000), FCONST(12.000),
    FCONST(14.000), FCONST(16.000)
};

/**
 * Variable bandwidth bit allocation
 * This estimates the bandwidth code which will give quality around 240.
 */
void
vbw_bit_allocation(A52ThreadContext *tctx)
{
    A52Context *ctx = tctx->ctx;
    A52Frame *frame = &tctx->frame;
    FLOAT mant_bits;
    int blk, ch, bw, nc;
    int avail_bits, bits;
    int wmin, wmax, ncmin, ncmax;

    start_bit_allocation(tctx);
    avail_bits = (16 * frame->frame_size) - frame->frame_bits;

    bit_alloc_prepare(tctx);
    bit_alloc(tctx, 240);

    // deduct any LFE exponent and mantissa bits
    if(ctx->lfe) {
        FLOAT lfe_bits = FCONST(0.0);
        ch = ctx->lfe_channel;
        lfe_bits += expstr_set_bits[frame->expstr_set[ch]][7];
        for(blk=0; blk<A52_NUM_BLOCKS; blk++) {
            uint8_t *bap = frame->blocks[blk].bap[ch];
            for(nc=0; nc<7; nc++) {
                lfe_bits += mant_est_tab[bap[nc]];
            }
        }
        avail_bits -= (int)lfe_bits;
    }

    // set limits
    wmin = ctx->params.min_bwcode;
    wmax = ctx->params.max_bwcode;
    ncmin = wmin * 3 + 73;
    ncmax = wmax * 3 + 73;

    // sum up mantissa bits up to bin 72
    mant_bits = FCONST(0.0);
    for(ch=0; ch<ctx->n_channels; ch++) {
        for(blk=0; blk<A52_NUM_BLOCKS; blk++) {
            uint8_t *bap = frame->blocks[blk].bap[ch];
            for(nc=0; nc<ncmin; nc++) {
                mant_bits += mant_est_tab[bap[nc]];
            }
        }
    }

    // add bins while estimated bits fit in the frame
    for(nc=ncmin; nc<=ncmax; nc++) {
        bw = (nc - 73) / 3;
        bits = 0;
        for(ch=0; ch<ctx->n_channels; ch++) {
            bits += expstr_set_bits[frame->expstr_set[ch]][nc];
            for(blk=0; blk<A52_NUM_BLOCKS; blk++) {
                mant_bits += mant_est_tab[frame->blocks[blk].bap[ch][nc]];
            }
        }
        if((bits + (int)mant_bits) > avail_bits) {
            break;
        }
    }

    // set frame bandwidth parameters
    bw = CLIP((nc - 73) / 3, 0, 60);
    nc = bw * 3 + 73;
    frame->bwcode = bw;
    for(ch=0; ch<ctx->n_channels; ch++) {
        frame->ncoefs[ch] = nc;
    }
}

/**
 * Run the bit allocation encoding routine.
 * Runs the bit allocation in either CBR or VBR mode, depending on the mode
 * selected by the user.
 */
int
compute_bit_allocation(A52ThreadContext *tctx)
{
    A52Context *ctx = tctx->ctx;

    start_bit_allocation(tctx);
    if(ctx->params.encoding_mode == AFTEN_ENC_MODE_VBR) {
        if(vbr_bit_allocation(tctx)) {
            return -1;
        }
    } else if(ctx->params.encoding_mode == AFTEN_ENC_MODE_CBR) {
        if(cbr_bit_allocation(tctx, 1)) {
            return -1;
        }
    } else {
        return -1;
    }
    return 0;
}
