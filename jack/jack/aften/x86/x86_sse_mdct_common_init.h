/*********************************************************************
 * Copyright (C) 2005-2007 by Prakash Punnoor                        *
 * prakash@punnoor.de                                                *
 *                                                                   *
 * This library is free software; you can redistribute it and/or     *
 * modify it under the terms of the GNU Library General Public       *
 * License as published by the Free Software Foundation;             *
 * version 2 of the License                                          *
 *                                                                   *
 * This library is distributed in the hope that it will be useful,   *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of    *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU *
 * Library General Public License for more details.                  *
 *                                                                   *
 * You should have received a copy of the GNU Library General Public *
 * License along with this library; if not, write to the             *
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,      *
 * Boston, MA  02110-1301, USA.                                      *
 *********************************************************************/
#ifndef X86_SSE_MDCT_COMMON_INIT_H
#define X86_SSE_MDCT_COMMON_INIT_H

#include "common.h"
#include "mdct.h"

void sse_mdct_ctx_init(MDCTContext *mdct, int n);
void sse_mdct_ctx_close(MDCTContext *mdct);
void sse_mdct_tctx_init(MDCTThreadContext *mdct, int n);
void sse_mdct_tctx_close(MDCTThreadContext *tmdct);

#endif /* X86_SSE_MDCT_COMMON_INIT_H */
