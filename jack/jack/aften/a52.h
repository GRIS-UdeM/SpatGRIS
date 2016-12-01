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
 * @file a52.h
 * A/52 encoder header
 */

#ifndef A52_H
#define A52_H

#include "common.h"
#include "bitio.h"
#include "aften.h"
#include "filter.h"
#include "mdct.h"
#include "threading.h"

#define AFTEN_VERSION "0.0.8"

#define A52_MAX_CHANNELS 6

#define A52_NUM_BLOCKS 6

/* exponent encoding strategy */
#define EXP_REUSE 0
#define EXP_NEW   1
#define EXP_D15   1
#define EXP_D25   2
#define EXP_D45   3

#define SNROFFST(csnr, fsnr) (((((csnr)-15) << 4) + (fsnr)) << 2)
#define QUALITY(csnr, fsnr) ((SNROFFST(csnr, fsnr)+960)/4)

/* possible frequencies */
extern const uint16_t a52_freqs[3];

/* possible bitrates */
extern const uint16_t a52_bitratetab[19];


extern const uint8_t log2tab[256];

static inline int
log2i(uint32_t v)
{
    int n = 0;
    if(v & 0xffff0000){ v >>= 16; n += 16; }
    if(v & 0xff00){ v >>= 8; n += 8; }
    n += log2tab[v];

    return n;
}

typedef struct A52Block {
    FLOAT *input_samples[A52_MAX_CHANNELS]; /* 512 per ch */
    FLOAT *mdct_coef[A52_MAX_CHANNELS]; /* 256 per ch */
    FLOAT transient_samples[A52_MAX_CHANNELS][512];
    int block_num;
    int blksw[A52_MAX_CHANNELS];
    int dithflag[A52_MAX_CHANNELS];
    int dynrng;
    uint8_t exp[A52_MAX_CHANNELS][256];
    int16_t psd[A52_MAX_CHANNELS][256];
    int16_t mask[A52_MAX_CHANNELS][50];
    uint8_t exp_strategy[A52_MAX_CHANNELS];
    uint8_t nexpgrps[A52_MAX_CHANNELS];
    uint8_t grp_exp[A52_MAX_CHANNELS][85];
    uint8_t bap[A52_MAX_CHANNELS][256];
    uint16_t qmant[A52_MAX_CHANNELS][256];
    uint8_t rematstr;
    uint8_t rematflg[4];
} A52Block;

typedef struct A52BitAllocParams {
    int fscod;
    int halfratecod;
    int fgain;
    int sgain, sdecay, fdecay, dbknee, floor;
    int cplfleak, cplsleak;
} A52BitAllocParams;

typedef struct A52Frame {
    int quality;
    int bit_rate;
    int bwcode;

    FLOAT input_audio[A52_MAX_CHANNELS][A52_SAMPLES_PER_FRAME];
    A52Block blocks[A52_NUM_BLOCKS];
    int frame_bits;
    int exp_bits;
    int mant_bits;
    unsigned int frame_size_min; // minimum frame size
    unsigned int frame_size;     // current frame size in words
    unsigned int frmsizecod;

    // bitrate allocation control
    int sgaincod, sdecaycod, fdecaycod, dbkneecod, floorcod;
    A52BitAllocParams bit_alloc;
    int csnroffst;
    int fgaincod;
    int fsnroffst;
    int ncoefs[A52_MAX_CHANNELS];
    int expstr_set[A52_MAX_CHANNELS];
} A52Frame;

typedef struct A52ThreadContext {
    struct A52Context *ctx;
#ifndef NO_THREADS
    A52ThreadSync ts;
#endif
    ThreadState state;
    int thread_num;
    int framesize;

    AftenStatus status;
    A52Frame frame;
    BitWriter bw;
    uint8_t frame_buffer[A52_MAX_CODED_FRAME_SIZE];

    uint32_t bit_cnt;
    uint32_t sample_cnt;

    int last_quality;

    MDCTThreadContext mdct_tctx_512;
    MDCTThreadContext mdct_tctx_256;
} A52ThreadContext;

typedef struct A52Context {
    A52ThreadContext *tctx;
#ifndef NO_THREADS
    A52GlobalThreadSync ts;
#endif
    AftenEncParams params;
    AftenMetadata meta;
    void (*fmt_convert_from_src)(FLOAT dest[A52_MAX_CHANNELS][A52_SAMPLES_PER_FRAME],
          const void *vsrc, int nch, int n);
    void (*apply_a52_window)(FLOAT *samples);
    void (*process_exponents)(A52ThreadContext *tctx);

    int n_threads;
    int n_channels;
    int n_all_channels;
    int acmod;
    int lfe;
    int lfe_channel;
    int sample_rate;
    int halfratecod;
    int bsid;
    int fscod;
    int bsmod;
    int target_bitrate;
    int frmsizecod;
    int fixed_bwcode;

    FilterContext bs_filter[A52_MAX_CHANNELS];
    FilterContext dc_filter[A52_MAX_CHANNELS];
    FilterContext bw_filter[A52_MAX_CHANNELS];
    FilterContext lfe_filter;

    FLOAT last_samples[A52_MAX_CHANNELS][256];
    FLOAT last_transient_samples[A52_MAX_CHANNELS][256];

    MDCTContext mdct_ctx_512;
    MDCTContext mdct_ctx_256;
} A52Context;

#endif /* A52_H */
