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
 * libaften public header for function declarations
 */

#ifndef AFTEN_H
#define AFTEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "aften-types.h"

#if defined(_WIN32) && !defined(_XBOX)
 #if defined(AFTEN_BUILD_LIBRARY)
  #define AFTEN_API __declspec(dllexport)
 #else
  #define AFTEN_API __declspec(dllimport)
 #endif
#else
 #if defined(AFTEN_BUILD_LIBRARY) && defined(HAVE_GCC_VISIBILITY)
  #define AFTEN_API __attribute__((visibility("default")))
 #else
  #define AFTEN_API extern
 #endif
#endif

/**
 * @defgroup encoding Main encoding functions
 * @{
 */

/**
 * Gets the libaften version string.
 */
AFTEN_API const char *aften_get_version(void);

/**
 * Sets the parameters for an encoding context to their default values.
 * @param s The encoding context
 */
AFTEN_API void aften_set_defaults(AftenContext *s);

/**
 * Initializes an encoding context.
 * This must be called before any calls to @c aften_encode_frame
 * @param s The encoding context
 * @return Returns 0 on success, non-zero on failure.
 */
AFTEN_API int aften_encode_init(AftenContext *s);

/**
 * Encodes a single AC-3 frame.
 * @param s    The encoding context
 * @param[out] frame_buffer Pointer to output frame data
 * @param[in]  samples      Pointer to input audio samples
 * @return Returns the number of bytes written to @p frame_buffer, or returns
 * a negative value on error.
 */
AFTEN_API int aften_encode_frame(AftenContext *s, unsigned char *frame_buffer,
                                 const void *samples);

/**
 * Sets the parameters in the context @p s to their default values.
 * @param s The encoding context
 */
AFTEN_API void aften_encode_close(AftenContext *s);

/** @} end encoding functions */

/**
 * @defgroup utility Utility functions
 * @{
 */

/**
 * Determines the proper A/52 acmod and lfe parameters based on the
 * number of channels and the WAVE_FORMAT_EXTENSIBLE channel mask.  If the
 * chmask value has the high bit set to 1 (e.g. 0xFFFFFFFF), then the default
 * plain WAVE channel selection is assumed.
 * @param[in]  ch      number of channels
 * @param[in]  chmask  channel mask
 * @param[out] acmod   pointer to audio coding mode
 * @param[out] lfe     pointer to LFE flag
 * @return On error, the @p acmod and @p lfe output params are set to -1 and
 * the function returns -1;  on success, the @p acmod and @p lfe params are set
 * to appropriate values and the function returns 0.
 */
AFTEN_API int aften_wav_channels_to_acmod(int ch, unsigned int chmask,
                                          int *acmod, int *lfe);

/**
 * Takes a channel-interleaved array of audio samples, where the channel order
 * is the default WAV order. The samples are rearranged to the proper A/52
 * channel order based on the @p acmod and @p lfe parameters.
 * @param     samples  array of interleaved audio samples
 * @param[in] n        number of samples in the array
 * @param[in] ch       number of channels
 * @param[in] fmt      sample format
 * @param[in] acmod    audio coding mode
 */
AFTEN_API void aften_remap_wav_to_a52(void *samples, int n, int ch,
                                      A52SampleFormat fmt, int acmod);

/**
 * Takes a channel-interleaved array of audio samples, where the channels are
 * in MPEG order. The samples are rearranged to the proper A/52 channel order
 * based on the @p acmod parameter.
 * @param     samples  array of interleaved audio samples
 * @param[in] n        number of samples in the array
 * @param[in] ch       number of channels
 * @param[in] fmt      sample format
 * @param[in] acmod    audio coding mode
 */
AFTEN_API void aften_remap_mpeg_to_a52(void *samples, int n, int ch,
                                       A52SampleFormat fmt, int acmod);

/**
 * Tells whether libaften was configured to use floats or doubles
 */
AFTEN_API FloatType aften_get_float_type(void);

/** @} end utility functions */

#undef AFTEN_API

#if defined(__cplusplus)
}
#endif

#endif /* AFTEN_H */
