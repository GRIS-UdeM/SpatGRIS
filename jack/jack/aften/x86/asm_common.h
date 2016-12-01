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
#ifndef ASM_COMMON_H
#define ASM_COMMON_H

#define _pushf		_st(pushf)
#define _popf		_st(popf)
#define _push(x)	_st(push x)
#define _pop(x) 	_st(pop x)
#define _jz(x)		_st(jz x)
#define _cpuid		_st(cpuid)

#endif /* ASM_COMMON_H */
