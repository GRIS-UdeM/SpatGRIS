/**
 * Aften: A/52 audio encoder
 * Copyright (c) 2006-2007 Justin Ruggles
 *               2006-2007 Prakash Punnoor <prakash@punnoor.de>
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
 * @file aften.h
 * libaften public header for type definitions
 */

#ifndef AFTEN_TYPES_H
#define AFTEN_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Some helpful size constants
 */
enum {
    A52_MAX_CODED_FRAME_SIZE = 3840,
    A52_SAMPLES_PER_FRAME = 1536
};

/**
 * Aften Encoding Mode
 */
typedef enum {
    AFTEN_ENC_MODE_CBR = 0,
    AFTEN_ENC_MODE_VBR
} AftenEncMode;

/**
 * Floating-Point Data Types
 */
typedef enum {
    FLOAT_TYPE_DOUBLE,
    FLOAT_TYPE_FLOAT
} FloatType;

/**
 * Audio Sample Formats
 */
typedef enum {
    A52_SAMPLE_FMT_U8 = 0,
    A52_SAMPLE_FMT_S16,
    A52_SAMPLE_FMT_S20,
    A52_SAMPLE_FMT_S24,
    A52_SAMPLE_FMT_S32,
    A52_SAMPLE_FMT_FLT,
    A52_SAMPLE_FMT_DBL
} A52SampleFormat;

/**
 * Dynamic Range Profiles
 */
typedef enum {
    DYNRNG_PROFILE_FILM_LIGHT=0,
    DYNRNG_PROFILE_FILM_STANDARD,
    DYNRNG_PROFILE_MUSIC_LIGHT,
    DYNRNG_PROFILE_MUSIC_STANDARD,
    DYNRNG_PROFILE_SPEECH,
    DYNRNG_PROFILE_NONE
} DynRngProfile;

/**
 * Audio Coding Mode (acmod) options
 */
enum {
    A52_ACMOD_DUAL_MONO = 0,
    A52_ACMOD_MONO,
    A52_ACMOD_STEREO,
    A52_ACMOD_3_0,
    A52_ACMOD_2_1,
    A52_ACMOD_3_1,
    A52_ACMOD_2_2,
    A52_ACMOD_3_2
};

/**
 * SIMD instruction sets
 */
typedef struct {
    int mmx;
    int sse;
    int sse2;
    int sse3;
    int ssse3;
    int amd_3dnow;
    int amd_3dnowext;
    int amd_sse_mmx;
    int altivec;
} AftenSimdInstructions;

/**
 * Performance related parameters
 */
typedef struct {
    /**
     * Number of threads
     * How many threads should be used.
     * Default value is 0, which indicates detecting number of CPUs.
     * Maximum value is AFTEN_MAX_THREADS.
     */
    int n_threads;

    /**
     * Available SIMD instruction sets; shouldn't be modified
     */
    AftenSimdInstructions available_simd_instructions;

    /**
     * Wanted SIMD instruction sets
     */
    AftenSimdInstructions wanted_simd_instructions;
} AftenSystemParams;

/**
 * Parameters which affect encoded audio output
 */
typedef struct {
    /**
     * Bitrate selection mode.
     * AFTEN_ENC_MODE_CBR : constant bitrate
     * AFTEN_ENC_MODE_VBR : variable bitrate
     * default is CBR
     */
    AftenEncMode encoding_mode;

    /**
     * Stereo rematrixing option.
     * Set to 0 to disable stereo rematrixing, 1 to enable it.
     * default is 1
     */
    int use_rematrixing;

    /**
     * Block switching option.
     * Set to 0 to disable block switching, 1 to enable it.
     * default is 0
     */
    int use_block_switching;

    /**
     * DC high-pass filter option.
     * Set to 0 to disable the filter, 1 to enable it.
     * default is 0
     */
    int use_dc_filter;

    /**
     * Bandwidth low-pass filter option.
     * Set to 0 to disable the, 1 to enable it.
     * This option cannot be enabled with variable bandwidth mode (bwcode=-2)
     * default is 0
     */
    int use_bw_filter;

    /**
     * LFE low-pass filter option.
     * Set to 0 to disable the filter, 1 to enable it.
     * This limits the LFE bandwidth, and can only be used if the input audio
     * has an LFE channel.
     * default is 0
     */
    int use_lfe_filter;

    /**
     * Constant bitrate.
     * This option sets the bitrate for CBR encoding mode.
     * It can also be used to set the maximum bitrate for VBR mode.
     * It is specified in kbps. Only certain bitrates are valid:
     *   0,  32,  40,  48,  56,  64,  80,  96, 112, 128,
     * 160, 192, 224, 256, 320, 384, 448, 512, 576, 640
     * default is 0
     * For CBR mode, this selects bitrate based on the number of channels.
     * For VBR mode, this sets the maximum bitrate to 640 kbps.
     */
    int bitrate;

    /**
     * VBR Quality.
     * This option sets the target quality for VBR encoding mode.
     * The range is 0 to 1023 and corresponds to the SNR offset.
     * default is 240
     */
    int quality;

    /**
     * Bandwidth code.
     * This option determines the cutoff frequency for encoded bandwidth.
     * 0 to 60 corresponds to a cutoff of 28.5% to 98.8% of the full bandwidth.
     * -1 is used for constant adaptive bandwidth. Aften selects a good value
     *    based on the quality or bitrate parameters.
     * -2 is used for variable adaptive bandwidth. Aften selects a value for
     *    each frame based on the encoding quality level for that frame.
     * default is -1
     */
    int bwcode;

    /**
     * Bit Allocation speed/accuracy
     * This determines how accurate the bit allocation search method is.
     * Set to 0 for better quality
     * Set to 1 for faster encoding
     * default is 0
     */
    int bitalloc_fast;

    /**
     * Exponent Strategy speed/quality
     * This determines whether to use a fixed or adaptive exponent strategy.
     * Set to 0 for adaptive strategy (better quality, slower)
     * Set to 1 for fixed strategy (lower quality, faster)
     */
    int expstr_fast;

    /**
     * Dynamic Range Compression profile
     * This determines which DRC profile to use.
     * Film Light:     DYNRNG_PROFILE_FILM_LIGHT
     * Film Standard:  DYNRNG_PROFILE_FILM_STANDARD
     * Music Light:    DYNRNG_PROFILE_MUSIC_LIGHT
     * Music Standard: DYNRNG_PROFILE_MUSIC_STANDARD
     * Speech:         DYNRNG_PROFILE_SPEECH,
     * None:           DYNRNG_PROFILE_NONE
     * default is None
     */
    DynRngProfile dynrng_profile;

    /**
     * Minimum bandwidth code.
     * For use with variable bandwidth mode, this option determines the
     * minimum value for the bandwidth code.
     * default is 0.
     */
    int min_bwcode;

    /**
     * Maximum bandwidth code.
     * For use with variable bandwidth mode, this option determines the
     * maximum value for the bandwidth code.
     * default is 60.
     */
    int max_bwcode;

} AftenEncParams;

/**
 * Metadata parameters
 * See the A/52 specification for details regarding the metadata.
 */
typedef struct {
    /** Center mix level */
    int cmixlev;

    /** Surround mix level */
    int surmixlev;

    /** Dolby(R) Surround mode */
    int dsurmod;

    /** Dialog normalization */
    int dialnorm;

    /** Extended bit stream info 1 exists */
    int xbsi1e;

    /** Preferred downmix mode */
    int dmixmod;

    /** LtRt center mix level */
    int ltrtcmixlev;

    /** LtRt surround mix level */
    int ltrtsmixlev;

    /** LoRo center mix level */
    int lorocmixlev;

    /** LoRo surround mix level */
    int lorosmixlev;

    /** Extended bit stream info 2 exists */
    int xbsi2e;

    /** Dolby(R) Surround EX mode */
    int dsurexmod;

    /** Dolby(R) Headphone mode */
    int dheadphonmod;

    /** A/D converter type */
    int adconvtyp;

} AftenMetadata;

/**
 * Values in this structure are updated by Aften during encoding.
 * They give information about the previously encoded frame.
 */
typedef struct {
    int quality;
    int bit_rate;
    int bwcode;
} AftenStatus;

/**
 * libaften public encoding context
 */
typedef struct {
    AftenEncParams params;
    AftenMetadata meta;
    AftenStatus status;
    AftenSystemParams system;

    /**
     * Verbosity level.
     * 0 is quiet mode. 1 and 2 are more verbose.
     * default is 1
     */
    int verbose;

    /**
     * Total number of channels in the input stream.
     */
    int channels;

    /**
     * Audio coding mode (channel configuration).
     * There are utility functions to set this if you don't know the proper
     * value.
     */
    int acmod;

    /**
     * Indicates that there is an LFE channel present.
     * There are utility functions to set this if you don't know the proper
     * value.
     */
    int lfe;

    /**
     * Audio sample rate in Hz
     */
    int samplerate;

    /**
     * Audio sample format
     * default: A52_SAMPLE_FMT_S16
     */
    A52SampleFormat sample_format;

    /**
     * Used internally by the encoder. The user should leave this alone.
     * It is allocated in aften_encode_init and free'd in aften_encode_close.
     */
    void *private_context;
} AftenContext;

#if defined(__cplusplus)
}
#endif

#endif /* AFTEN_TYPES_H */
