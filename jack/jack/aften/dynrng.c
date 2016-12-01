/**
 * Aften: A/52 audio encoder
 * Copyright (c) 2006 Justin Ruggles
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
 * @file dynrng.c
 * A/52 Dynamic Range Compression
 */

#include "common.h"

#include "aften.h"
#include "a52.h"
#include "dynrng.h"

typedef struct {
    int boost_start;
    int null_start;
    int early_cut_start;
    int cut_start;
    int cut_end;
    FLOAT boost_ratio;
    FLOAT early_cut_ratio;
    FLOAT cut_ratio;
} DRCProfile;

static const DRCProfile drc_profiles[5] = {
    // Film Light
    { -22, -10, 10, 20, 35, FCONST(0.50), FCONST(0.50), FCONST(0.05) },
    // Film Standard
    { -12,   0,  5, 15, 35, FCONST(0.50), FCONST(0.50), FCONST(0.05) },
    // Music Light
    { -34, -10, 10, 10, 40, FCONST(0.50), FCONST(0.50), FCONST(0.50) },
    // Music Standard
    { -24,   0,  5, 15, 35, FCONST(0.50), FCONST(0.50), FCONST(0.05) },
    // Speech
    { -19,   0,  5, 15, 35, FCONST(0.20), FCONST(0.50), FCONST(0.05) }
};

/**
 * table of dynrng scale factors indexed by dynrng code
 * scale factor is given in 1/512 units
 *   0- 31 :  512 - 1008 step  16 (1.0000 -  1.968750000 step 0.031250000)
 *  32- 63 : 1024 - 2016 step  32 (2.0000 -  3.937500000 step 0.062500000)
 *  64- 95 : 2048 - 4032 step  64 (4.0000 -  7.875000000 step 0.125000000)
 *  96-127 : 4096 - 8064 step 128 (8.0000 - 15.750000000 step 0.250000000)
 * 128-159 :   32 -   63 step   1 (0.0625 -  0.123046875 step 0.001953125)
 * 160-191 :   64 -  126 step   2 (0.1250 -  0.246093750 step 0.003906250)
 * 192-223 :  128 -  252 step   4 (0.2500 -  0.492187500 step 0.007812500)
 * 224-255 :  256 -  504 step   8 (0.5000 -  0.984375000 step 0.015625000)
 */
#if 0
static FLOAT dynrngscaletab[256];
#endif

#define DB_TO_SCALE(db) (AFT_EXP10(db * FCONST(0.05)))

#define SCALE_TO_DB(scale) (FCONST(20.0) * AFT_LOG10(scale))

void
dynrng_init(void)
{
#if 0
    int i, logscale;

    for(i=0; i<256; i++) {
        logscale = ((i >> 5) + 4) & 7;
        dynrngscaletab[i] = (1 << (logscale+5)) + ((i & 31) << logscale);
    }
#endif
}

static int
scale_to_dynrng(FLOAT scale)
{
    int scale512, logscale, code;

    scale512 = (int)(AFT_FABS(scale) * FCONST(512.0));
    scale512 = CLIP(scale512, 32, 8064);
    logscale = log2i(scale512)-5;
    code = (((logscale + 4) & 7) << 5) + ((scale512 - (1 << (logscale+5))) >> logscale);
    return code;
}

#if 0
// save for later use
static FLOAT
dynrng_to_scale(int code)
{
    FLOAT scale;

    code = CLIP(code, 0, 255);
    scale = ((FLOAT)dynrngscaletab[code]) / FCONST(512.0);
    return scale;
}
#endif

/**
 * Calculates decibel gain which should be applied
 */
static FLOAT
calculate_gain_from_profile(FLOAT rms, int dialnorm, int profile)
{
    DRCProfile ps;
    FLOAT max_ecut, gain, target;

    ps = drc_profiles[profile];
    ps.boost_start     += dialnorm;
    ps.null_start      += dialnorm;
    ps.early_cut_start += dialnorm;
    ps.cut_start       += dialnorm;
    ps.cut_end         += dialnorm;

    max_ecut = ps.early_cut_start + ((ps.cut_start - ps.early_cut_start) * ps.early_cut_ratio);
    gain = 0;
    if(rms <= ps.boost_start) {
        gain = (ps.null_start - ps.boost_start) * ps.boost_ratio;
    } else if(rms <= ps.null_start) {
        gain = (ps.null_start - rms) * ps.boost_ratio;
    } else if(rms <= ps.early_cut_start) {
        gain = 0;
    } else if(rms <= ps.cut_start) {
        target = ps.early_cut_start + ((rms - ps.early_cut_start) * ps.early_cut_ratio);
        gain = target - rms;
    } else if(rms <= ps.cut_end) {
        target = max_ecut + ((rms - ps.cut_start) * ps.cut_ratio);
        gain = target - rms;
    } else {
        target = max_ecut + ((ps.cut_end - ps.cut_start) * ps.cut_ratio);
        gain = target - rms;
    }

    return gain;
}

static FLOAT
calculate_rms(FLOAT *samples[A52_MAX_CHANNELS], int ch, int n)
{
    FLOAT rms_all, rms_left, rms_right;
    int i;

    // For now, use only the left and right channels to calculate loudness
    if(ch == 1) {
        rms_all = 0;
        for(i=0; i<n; i++) {
            rms_all += (samples[0][i] * samples[0][i]);
        }
        rms_all /= n;
    } else {
        rms_left = rms_right = 0;
        for(i=0; i<n; i++) {
            rms_left  += (samples[0][i] * samples[0][i]);
            rms_right += (samples[1][i] * samples[1][i]);
        }
        rms_all = (rms_left + rms_right) / (FCONST(2.0) * n);
    }

    // Convert to dB
    rms_all = FCONST(10.0) * AFT_LOG10(rms_all + FCONST(1e-10));
    return rms_all;
}

int
calculate_block_dynrng(FLOAT *samples[A52_MAX_CHANNELS], int num_ch,
                       int dialnorm, DynRngProfile profile)
{
    int ch, i;
    FLOAT max_gain, rms, gain;

    if(profile == DYNRNG_PROFILE_NONE) return 0;

    // Find the maximum dB gain that can be used without clipping
    max_gain = 0;
    for(ch=0; ch<num_ch; ch++) {
        for(i=0; i<256; i++) {
            max_gain = MAX(AFT_FABS(samples[ch][i]), max_gain);
        }
    }
    max_gain = SCALE_TO_DB(FCONST(1.0) / max_gain);

    rms = calculate_rms(samples, num_ch, 256);
    gain = calculate_gain_from_profile(rms, dialnorm, (int)profile);
    gain = MIN(gain, max_gain);

    return scale_to_dynrng(DB_TO_SCALE(gain));
}
