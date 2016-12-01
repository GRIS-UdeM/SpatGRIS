/********************************************************************************
 * Copyright (C) 2005-2007 by Prakash Punnoor                                   *
 * prakash@punnoor.de                                                           *
 *                                                                              *
 * This library is free software; you can redistribute it and/or                *
 * modify it under the terms of the GNU Lesser General Public                   *
 * License as published by the Free Software Foundation; either                 *
 * version 2 of the License                                                     *
 *                                                                              *
 * This library is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            *
 * Lesser General Public License for more details.                              *
 *                                                                              *
 * You should have received a copy of the GNU Lesser General Public             *
 * License along with this library; if not, write to the Free Software          *
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA *
 ********************************************************************************/
#include "x86_sse_mdct_common_init.h"
#include "x86_sse_mdct_common.c"

static void
sse3_mdct_close(A52Context *ctx)
{
    sse_mdct_ctx_close(&ctx->mdct_ctx_512);
    sse_mdct_ctx_close(&ctx->mdct_ctx_256);
}

static void
sse3_mdct_thread_close(A52ThreadContext *tctx)
{
    sse_mdct_tctx_close(&tctx->mdct_tctx_512);
    sse_mdct_tctx_close(&tctx->mdct_tctx_256);

    aligned_free(tctx->frame.blocks[0].input_samples[0]);
}

void
sse3_mdct_init(A52Context *ctx)
{
    sse_mdct_ctx_init(&ctx->mdct_ctx_512, 512);
    sse_mdct_ctx_init(&ctx->mdct_ctx_256, 256);

    ctx->mdct_ctx_512.mdct = mdct_512;
    ctx->mdct_ctx_256.mdct = mdct_256;

    ctx->mdct_ctx_512.mdct_close = sse3_mdct_close;
    ctx->mdct_ctx_256.mdct_close = sse3_mdct_close;
}

void
sse3_mdct_thread_init(A52ThreadContext *tctx)
{
    sse_mdct_tctx_init(&tctx->mdct_tctx_512, 512);
    sse_mdct_tctx_init(&tctx->mdct_tctx_256, 256);

    tctx->mdct_tctx_512.mdct_thread_close = sse3_mdct_thread_close;
    tctx->mdct_tctx_256.mdct_thread_close = sse3_mdct_thread_close;

    tctx->mdct_tctx_512.mdct = &tctx->ctx->mdct_ctx_512;
    tctx->mdct_tctx_256.mdct = &tctx->ctx->mdct_ctx_256;

    tctx->frame.blocks[0].input_samples[0] =
        aligned_malloc(A52_NUM_BLOCKS * A52_MAX_CHANNELS * (256 + 512) * sizeof(FLOAT));
    alloc_block_buffers(tctx);
}
