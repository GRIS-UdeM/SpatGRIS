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
 * @file util.c
 * libaften utility functions
 */

#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "aften.h"
#include "a52.h"

static const int ch_to_acmod[7] = { -1, 1, 2, 3, 6, 7, 7 };

const uint8_t log2tab[256] = {
	0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};


/**
 * Determines the proper A/52 acmod and lfe parameters based on the
 * number of channels and the WAVE_FORMAT_EXTENSIBLE channel mask.  If the
 * chmask value has the high bit set to 1 (e.g. 0xFFFFFFFF), then the default
 * plain WAVE channel selection is assumed.
 * On error, the acmod and lfe output params are set to -1 and the function
 * returns -1.  On success, the acmod and lfe params are set to appropriate
 * values and the function returns 0.
 */
int
aften_wav_channels_to_acmod(int ch, unsigned int chmask, int *acmod, int *lfe)
{
    int tmp_lfe, tmp_acmod;

    // check for null output pointers
    if(acmod == NULL || lfe == NULL) {
        fprintf(stderr, "One or more NULL parameters passed to aften_wav_chmask_to_acmod\n");
        return -1;
    }
    *acmod = tmp_acmod = -1;
    *lfe = tmp_lfe = -1;

    // check for valid number of channels
    if(ch < 1 || ch > A52_MAX_CHANNELS) {
        fprintf(stderr, "Unsupported # of channels passed to aften_wav_chmask_to_acmod\n");
        return -1;
    }

    if(chmask & 0x80000000) {
        // set values for plain WAVE format or unknown configuration
        tmp_lfe = (ch == 6);
        if(tmp_lfe) ch--;
        tmp_acmod = ch_to_acmod[ch];
    } else {
        // read chmask value for LFE channel
        tmp_lfe = !!(chmask & 0x08);
        if(tmp_lfe) {
            ch--;
            chmask -= 0x08;
        }

        // check for fbw channel layouts which are compatible with A/52
        if(chmask == 0x04 && ch == 1) {
            // 1/0 mode (C)
            tmp_acmod = A52_ACMOD_MONO;
        } else if(chmask == 0x03 && ch == 2) {
            // 2/0 mode (L,R)
            tmp_acmod = A52_ACMOD_STEREO;
        } else if(chmask == 0x07 && ch == 3) {
            // 3/0 mode (L,C,R)
            tmp_acmod = A52_ACMOD_3_0;
        } else if(chmask == 0x103 && ch == 3) {
            // 2/1 mode (L,R,S)
            tmp_acmod = A52_ACMOD_2_1;
        } else if(chmask == 0x107 && ch == 4) {
            // 3/1 mode (L,C,R,S)
            tmp_acmod = A52_ACMOD_3_1;
        } else if(chmask == 0x33 && ch == 4) {
            // 2/2 mode (L,R,SL,SR)
            tmp_acmod = A52_ACMOD_2_2;
        } else if((chmask == 0x37 || chmask == 0x607) && ch == 5) {
            // 3/2 mode (L,C,R,SL,SR)
            // supports either back-left/back-right or side-left/side-right
            tmp_acmod = A52_ACMOD_3_2;
        } else {
            // use default
            tmp_acmod = ch_to_acmod[ch];
            if(tmp_acmod < 0) {
                return -1;
            }
        }
    }

    *acmod = tmp_acmod;
    *lfe = tmp_lfe;
    return 0;
}

/**
 * WAV to A/52 channel mapping
 * note: thanks to Tebasuna for help in getting this order right.
 */

static const int wav_chmap[6] = { 0, 2, 1, 4, 5, 3 };

static void
remap_wav_to_a52_u8(uint8_t *samples, int n, int ch, int acmod)
{
    int i, j;
    uint8_t tmp[6];

    if(ch > 2 && acmod != 4 && acmod != 6) {
        if(ch == 6) {
            for(i=0; i<n*6; i+=6) {
                memcpy(tmp, &samples[i], 6*sizeof(uint8_t));
                for(j=0; j<6; j++) {
                    samples[i+j] = tmp[wav_chmap[j]];
                }
            }
        } else {
            for(i=0; i<n*ch; i+=ch) {
                tmp[0] = samples[i+1];
                samples[i+1] = samples[i+2];
                samples[i+2] = tmp[0];
            }
        }
    }
}

static void
remap_wav_to_a52_s16(int16_t *samples, int n, int ch, int acmod)
{
    int i, j;
    int16_t tmp[6];

    if(ch > 2 && acmod != 4 && acmod != 6) {
        if(ch == 6) {
            for(i=0; i<n*6; i+=6) {
                memcpy(tmp, &samples[i], 6*sizeof(int16_t));
                for(j=0; j<6; j++) {
                    samples[i+j] = tmp[wav_chmap[j]];
                }
            }
        } else {
            for(i=0; i<n*ch; i+=ch) {
                tmp[0] = samples[i+1];
                samples[i+1] = samples[i+2];
                samples[i+2] = tmp[0];
            }
        }
    }
}

static void
remap_wav_to_a52_s32(int32_t *samples, int n, int ch, int acmod)
{
    int i, j;
    int32_t tmp[6];

    if(ch > 2 && acmod != 4 && acmod != 6) {
        if(ch == 6) {
            for(i=0; i<n*6; i+=6) {
                memcpy(tmp, &samples[i], 6*sizeof(int32_t));
                for(j=0; j<6; j++) {
                    samples[i+j] = tmp[wav_chmap[j]];
                }
            }
        } else {
            for(i=0; i<n*ch; i+=ch) {
                tmp[0] = samples[i+1];
                samples[i+1] = samples[i+2];
                samples[i+2] = tmp[0];
            }
        }
    }
}

static void
remap_wav_to_a52_float(float *samples, int n, int ch, int acmod)
{
    int i, j;
    float tmp[6];

    if(ch > 2 && acmod != 4 && acmod != 6) {
        if(ch == 6) {
            for(i=0; i<n*6; i+=6) {
                memcpy(tmp, &samples[i], 6*sizeof(float));
                for(j=0; j<6; j++) {
                    samples[i+j] = tmp[wav_chmap[j]];
                }
            }
        } else {
            for(i=0; i<n*ch; i+=ch) {
                tmp[0] = samples[i+1];
                samples[i+1] = samples[i+2];
                samples[i+2] = tmp[0];
            }
        }
    }
}

static void
remap_wav_to_a52_double(double *samples, int n, int ch, int acmod)
{
    int i, j;
    double tmp[6];

    if(ch > 2 && acmod != 4 && acmod != 6) {
        if(ch == 6) {
            for(i=0; i<n*6; i+=6) {
                memcpy(tmp, &samples[i], 6*sizeof(double));
                for(j=0; j<6; j++) {
                    samples[i+j] = tmp[wav_chmap[j]];
                }
            }
        } else {
            for(i=0; i<n*ch; i+=ch) {
                tmp[0] = samples[i+1];
                samples[i+1] = samples[i+2];
                samples[i+2] = tmp[0];
            }
        }
    }
}

void
aften_remap_wav_to_a52(void *samples, int n, int ch, A52SampleFormat fmt,
                       int acmod)
{
    if(samples == NULL) {
        fprintf(stderr, "NULL parameter passed to aften_remap_wav_to_a52\n");
        return;
    }

    switch(fmt) {
        case A52_SAMPLE_FMT_U8:  remap_wav_to_a52_u8(samples, n, ch, acmod);
                                 break;
        case A52_SAMPLE_FMT_S16: remap_wav_to_a52_s16(samples, n, ch, acmod);
                                 break;
        case A52_SAMPLE_FMT_S20:
        case A52_SAMPLE_FMT_S24:
        case A52_SAMPLE_FMT_S32: remap_wav_to_a52_s32(samples, n, ch, acmod);
                                 break;
        case A52_SAMPLE_FMT_FLT: remap_wav_to_a52_float(samples, n, ch, acmod);
                                 break;
        case A52_SAMPLE_FMT_DBL: remap_wav_to_a52_double(samples, n, ch, acmod);
                                 break;
    }
}

/**
 * MPEG to A/52 channel mapping
 * Can be used for multi-channel DTS, MP2, or AAC
 */

static void
remap_mpeg_to_a52_u8(uint8_t *samples, int n, int ch, int acmod)
{
    int i;

    if(ch > 2 && acmod & 1) {
        for(i=0; i<n*ch; i+=ch) {
            uint8_t tmp = samples[i];
            samples[i] = samples[i+1];
            samples[i+1] = tmp;
        }
    }
}

static void
remap_mpeg_to_a52_s16(int16_t *samples, int n, int ch, int acmod)
{
    int i;

    if(ch > 2 && acmod & 1) {
        fprintf(stderr, "converting s16 mpeg to s16 ac3\n");
        for(i=0; i<n*ch; i+=ch) {
            int16_t tmp = samples[i];
            samples[i] = samples[i+1];
            samples[i+1] = tmp;
        }
    }
}

static void
remap_mpeg_to_a52_s32(int32_t *samples, int n, int ch, int acmod)
{
    int i;

    if(ch > 2 && acmod & 1) {
        for(i=0; i<n*ch; i+=ch) {
            int32_t tmp = samples[i];
            samples[i] = samples[i+1];
            samples[i+1] = tmp;
        }
    }
}

static void
remap_mpeg_to_a52_float(float *samples, int n, int ch, int acmod)
{
    int i;

    if(ch > 2 && acmod & 1) {
        for(i=0; i<n*ch; i+=ch) {
            float tmp = samples[i];
            samples[i] = samples[i+1];
            samples[i+1] = tmp;
        }
    }
}

static void
remap_mpeg_to_a52_double(double *samples, int n, int ch, int acmod)
{
    int i;

    if(ch > 2 && acmod & 1) {
        for(i=0; i<n*ch; i+=ch) {
            double tmp = samples[i];
            samples[i] = samples[i+1];
            samples[i+1] = tmp;
        }
    }
}

void
aften_remap_mpeg_to_a52(void *samples, int n, int ch, A52SampleFormat fmt,
                        int acmod)
{
    if(samples == NULL) {
        fprintf(stderr, "NULL parameter passed to aften_remap_wav_to_a52\n");
        return;
    }

    switch(fmt) {
        case A52_SAMPLE_FMT_U8:  remap_mpeg_to_a52_u8(samples, n, ch, acmod);
                                 break;
        case A52_SAMPLE_FMT_S16: remap_mpeg_to_a52_s16(samples, n, ch, acmod);
                                 break;
        case A52_SAMPLE_FMT_S20:
        case A52_SAMPLE_FMT_S24:
        case A52_SAMPLE_FMT_S32: remap_mpeg_to_a52_s32(samples, n, ch, acmod);
                                 break;
        case A52_SAMPLE_FMT_FLT: remap_mpeg_to_a52_float(samples, n, ch, acmod);
                                 break;
        case A52_SAMPLE_FMT_DBL: remap_mpeg_to_a52_double(samples, n, ch, acmod);
                                 break;
    }
}

FloatType
aften_get_float_type(void)
{
#ifdef CONFIG_DOUBLE
    return FLOAT_TYPE_DOUBLE;
#else
    return FLOAT_TYPE_FLOAT;
#endif
}
