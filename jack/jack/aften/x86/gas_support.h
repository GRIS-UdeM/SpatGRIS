/*********************************************************************
 * Copyright (C) 2007 by Prakash Punnoor                             *
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
#ifndef GAS_SUPPORT_H
#define GAS_SUPPORT_H

#define _st(x)		#x"\n\t"
#define __st(x,y)	#x","#y"\n\t"

#define _mov(x, y)	__st(mov x, y)
#define _xor(x, y)	__st(xor x, y)
#define _test(x, y)	__st(test x, y)

#define _(x)		$##x
#define _l(x)		#x":\n\t"

#define _eax		%%eax
#define _ebx		%%ebx
#define _ecx		%%ecx
#define _edx		%%edx
#define _esi		%%esi

#include "asm_common.h"

#ifdef __LP64__
#define _a			%%rax
#define _b			%%rbx
#define _c			%%rcx
#define _d			%%rdx
#define _s			%%rsi
#else
#define _a			_eax
#define _b			_ebx
#define _c			_ecx
#define _d			_edx
#define _s			_esi
#endif

#endif /* GAS_SUPPORT_H */
