/**
 * Aften: A/52 audio encoder
 * Copyright (c) 2007 Prakash Punnoor <prakash@punnoor.de>
 *               2006 Justin Ruggles
 *
 * Based on "The simplest AC3 encoder" from FFmpeg
 * Copyright (c) 2000 Fabrice Bellard.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License
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
 * @file x86_sse2_exponent.c
 * A/52 sse2 optimized exponent functions
 */

#include "exponent_common.c"
#include "x86_simd_support.h"

#include <emmintrin.h>


/* set exp[i] to min(exp[i], exp1[i]) */
static void
exponent_min(uint8_t *exp, uint8_t *exp1, int n)
{
    int i;

    for(i=0; i<(n & ~15); i+=16) {
        __m128i vexp = _mm_loadu_si128((__m128i*)&exp[i]);
        __m128i vexp1 = _mm_loadu_si128((__m128i*)&exp1[i]);
        vexp = _mm_min_epu8(vexp, vexp1);
        _mm_storeu_si128 ((__m128i*)&exp[i], vexp);
    }
    for(; i<n; ++i)
        exp[i] = MIN(exp[i], exp1[i]);
}


/**
 * Update the exponents so that they are the ones the decoder will decode.
 * Constrain DC exponent, group exponents based on strategy, constrain delta
 * between adjacent exponents to +2/-2.
 */
static void
encode_exp_blk_ch(uint8_t *exp, int ncoefs, int exp_strategy)
{
    int grpsize, ngrps, i, k, exp_min1, exp_min2;
    uint8_t v;

    ngrps = nexpgrptab[exp_strategy-1][ncoefs] * 3;
    grpsize = exp_strategy + (exp_strategy == EXP_D45);

    // for D15 strategy, there is no need to group/ungroup exponents
    switch (grpsize) {
    case 1: {
        // constraint for DC exponent
        exp[0] = MIN(exp[0], 15);

        // Decrease the delta between each groups to within 2
        // so that they can be differentially encoded
        for(i=1; i<=ngrps; i++)
            exp[i] = MIN(exp[i], exp[i-1]+2);
        for(i=ngrps-1; i>=0; i--)
            exp[i] = MIN(exp[i], exp[i+1]+2);

        return;
    }
    // for each group, compute the minimum exponent
    case 2: {
        ALIGN16(uint16_t) exp1[256];
        ALIGN16(const union __m128iui) vmask = {{0x00ff00ff, 0x00ff00ff, 0x00ff00ff, 0x00ff00ff}};

        i=0;k=1;
        for(; i<(ngrps & ~7); i+=8, k+=16) {
            __m128i v1 = _mm_loadu_si128((__m128i*)&exp[k]);
            __m128i v2 = _mm_srli_si128(v1, 1);
            v1 = _mm_and_si128(v1, vmask.v);
            v1 = _mm_min_epu8(v1, v2);
            _mm_store_si128((__m128i*)&exp1[i], v1);
        }
        switch (ngrps & 7) {
        case 7:
            exp1[i] = MIN(exp[k], exp[k+1]);
            ++i;
            k += 2;
        case 6:
            exp1[i] = MIN(exp[k], exp[k+1]);
            ++i;
            k += 2;
        case 5:
            exp1[i] = MIN(exp[k], exp[k+1]);
            ++i;
            k += 2;
        case 4:
            exp1[i] = MIN(exp[k], exp[k+1]);
            ++i;
            k += 2;
        case 3:
            exp1[i] = MIN(exp[k], exp[k+1]);
            ++i;
            k += 2;
        case 2:
            exp1[i] = MIN(exp[k], exp[k+1]);
            ++i;
            k += 2;
        case 1:
            exp1[i] = MIN(exp[k], exp[k+1]);
        case 0:
            ;
        }
        // constraint for DC exponent
        exp[0] = MIN(exp[0], 15);
        // Decrease the delta between each groups to within 2
        // so that they can be differentially encoded
        exp1[0] = MIN(exp1[0], (uint16_t)exp[0]+2);
        for(i=1; i<ngrps; i++)
            exp1[i] = MIN(exp1[i], exp1[i-1]+2);
        for(i=ngrps-2; i>=0; i--)
            exp1[i] = MIN(exp1[i], exp1[i+1]+2);
        // now we have the exponent values the decoder will see
        exp[0] = MIN(exp[0], exp1[0]+2); // DC exponent is handled separately

        i=0;k=1;
        for(; i<(ngrps & ~7); i+=8, k+=16) {
            __m128i v1 = _mm_load_si128((__m128i*)&exp1[i]);
            __m128i v2 = _mm_slli_si128(v1, 1);
            v1 = _mm_or_si128(v1, v2);
            _mm_storeu_si128((__m128i*)&exp[k], v1);
        }
        switch (ngrps & 7) {
        case 7:
            v = exp1[i];
            exp[k] = v;
            exp[k+1] = v;
            ++i;
            k += 2;
        case 6:
            v = exp1[i];
            exp[k] = v;
            exp[k+1] = v;
            ++i;
            k += 2;
        case 5:
            v = exp1[i];
            exp[k] = v;
            exp[k+1] = v;
            ++i;
            k += 2;
        case 4:
            v = exp1[i];
            exp[k] = v;
            exp[k+1] = v;
            ++i;
            k += 2;
        case 3:
            v = exp1[i];
            exp[k] = v;
            exp[k+1] = v;
            ++i;
            k += 2;
        case 2:
            v = exp1[i];
            exp[k] = v;
            exp[k+1] = v;
            ++i;
            k += 2;
        case 1:
            v = exp1[i];
            exp[k] = v;
            exp[k+1] = v;
        case 0:
            ;
        }
        return;
        }
    default: {
        ALIGN16(uint32_t) exp1[256];
        ALIGN16(const union __m128iui) vmask2 = {{0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff}};

        i=0;k=1;
        for(; i<(ngrps & ~3); i+=4, k+=16) {
            __m128i v1 = _mm_loadu_si128((__m128i*)&exp[k]);
            __m128i v2 = _mm_srli_si128(v1, 1);
            v1 = _mm_min_epu8(v1, v2);
            v2 = _mm_srli_si128(v1, 2);
            v1 = _mm_min_epu8(v1, v2);
            v1 = _mm_and_si128(v1, vmask2.v);
            _mm_store_si128((__m128i*)&exp1[i], v1);
        }
        switch (ngrps & 3) {
        case 3:
            exp_min1 = MIN(exp[k  ], exp[k+1]);
            exp_min2 = MIN(exp[k+2], exp[k+3]);
            exp1[i]  = MIN(exp_min1, exp_min2);
            ++i;
            k += 4;
        case 2:
            exp_min1 = MIN(exp[k  ], exp[k+1]);
            exp_min2 = MIN(exp[k+2], exp[k+3]);
            exp1[i]  = MIN(exp_min1, exp_min2);
            ++i;
            k += 4;
        case 1:
            exp_min1 = MIN(exp[k  ], exp[k+1]);
            exp_min2 = MIN(exp[k+2], exp[k+3]);
            exp1[i]  = MIN(exp_min1, exp_min2);
        case 0:
            ;
        }
        // constraint for DC exponent
        exp[0] = MIN(exp[0], 15);
        // Decrease the delta between each groups to within 2
        // so that they can be differentially encoded
        exp1[0] = MIN(exp1[0], (uint32_t)exp[0]+2);
        for(i=1; i<ngrps; i++)
            exp1[i] = MIN(exp1[i], exp1[i-1]+2);
        for(i=ngrps-2; i>=0; i--)
            exp1[i] = MIN(exp1[i], exp1[i+1]+2);
        // now we have the exponent values the decoder will see
        exp[0] = MIN(exp[0], exp1[0]+2); // DC exponent is handled separately

        i=0;k=1;
        for(; i<(ngrps & ~3); i+=4, k+=16) {
            __m128i v1 = _mm_load_si128((__m128i*)&exp1[i]);
            __m128i v2 = _mm_slli_si128(v1, 1);
            v1 = _mm_or_si128(v1, v2);
            v2 = _mm_slli_si128(v1, 2);
            v1 = _mm_or_si128(v1, v2);
            _mm_storeu_si128((__m128i*)&exp[k], v1);
        }
        switch (ngrps & 3) {
        case 3:
            v = exp1[i];
            exp[k] = v;
            exp[k+1] = v;
            exp[k+2] = v;
            exp[k+3] = v;
            ++i;
            k += 4;
        case 2:
            v = exp1[i];
            exp[k] = v;
            exp[k+1] = v;
            exp[k+2] = v;
            exp[k+3] = v;
            ++i;
            k += 4;
        case 1:
            v = exp1[i];
            exp[k] = v;
            exp[k+1] = v;
            exp[k+2] = v;
            exp[k+3] = v;
        case 0:
            ;
        }
        return;
    }
    }
}


/**
 * Determine a good exponent strategy for all blocks of a single channel.
 * A pre-defined set of strategies is chosen based on the SSE between each set
 * and the most accurate strategy set (all blocks EXP_D15).
 */
static int
compute_expstr_ch(uint8_t *exp[A52_NUM_BLOCKS], int ncoefs)
{
    ALIGN16(uint8_t) exponents[A52_NUM_BLOCKS][256];
    int blk, str, i, j, k;
    int min_error, exp_error[6];
    int err;

    min_error = 1;
    for(str=1; str<6; str++) {
        // collect exponents
        for(blk=0; blk<A52_NUM_BLOCKS; blk++) {
            memcpy(exponents[blk], exp[blk], 256);
        }

        // encode exponents
        i = 0;
        while(i < A52_NUM_BLOCKS) {
            j = i + 1;
            while(j < A52_NUM_BLOCKS && str_predef[str][j]==EXP_REUSE) {
                exponent_min(exponents[i], exponents[j], ncoefs);
                j++;
            }
            encode_exp_blk_ch(exponents[i], ncoefs, str_predef[str][i]);
            for(k=i+1; k<j; k++) {
                memcpy(exponents[k], exponents[i], 256);
            }
            i = j;
        }

        // select strategy based on minimum error from unencoded exponents
        exp_error[str] = 0;
        for(blk=0; blk<A52_NUM_BLOCKS; blk++) {
            uint8_t *exp_blk = exp[blk];
            uint8_t *exponents_blk = exponents[blk];
            union {
                __m128i v;
                int32_t res[4];
            } ures;
            __m128i vzero = _mm_setzero_si128();
            __m128i vres = vzero;

            for(i=0; i<(ncoefs & ~15); i+=16) {
                __m128i vexp = _mm_loadu_si128((__m128i*)&exp_blk[i]);
                __m128i vexp2 = _mm_load_si128((__m128i*)&exponents_blk[i]);
#if 0
                //safer but needed?
                __m128i vexphi = _mm_unpackhi_epi8(vexp, vzero);
                __m128i vexp2hi = _mm_unpackhi_epi8(vexp2, vzero);
                __m128i vexplo = _mm_unpacklo_epi8(vexp, vzero);
                __m128i vexp2lo = _mm_unpacklo_epi8(vexp2, vzero);
                __m128i verrhi = _mm_sub_epi16(vexphi, vexp2hi);
                __m128i verrlo = _mm_sub_epi16(vexplo, vexp2lo);
#else
                __m128i verr = _mm_sub_epi8(vexp, vexp2);
                __m128i vsign = _mm_cmplt_epi8(verr, vzero);
                __m128i verrhi = _mm_unpackhi_epi8(verr, vsign);
                __m128i verrlo = _mm_unpacklo_epi8(verr, vsign);
#endif
                verrhi = _mm_madd_epi16(verrhi, verrhi);
                verrlo = _mm_madd_epi16(verrlo, verrlo);
                verrhi = _mm_add_epi32(verrhi, verrlo);
                vres = _mm_add_epi32(vres, verrhi);
            }
            _mm_store_si128(&ures.v, vres);
            ures.res[0]+=ures.res[1];
            ures.res[2]+=ures.res[3];
            exp_error[str] += ures.res[0]+ures.res[2];
            for(; i<ncoefs; ++i) {
                err = exp_blk[i] - exponents_blk[i];
                exp_error[str] += (err * err);
            }
        }
        if(exp_error[str] < exp_error[min_error]) {
            min_error = str;
        }
    }
    return min_error;
}


/**
 * Runs all the processes in extracting, analyzing, and encoding exponents
 */
void
sse2_process_exponents(A52ThreadContext *tctx)
{
    extract_exponents(tctx);

    compute_exponent_strategy(tctx);

    encode_exponents(tctx);

    group_exponents(tctx);
}
