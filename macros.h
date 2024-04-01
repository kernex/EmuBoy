/*
 * Copyright (C) 2019 Flávio N. Lima
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __MACROS_H__
#define __MACROS_H__

#define INLINE __attribute__((always_inline)) inline

typedef int bool;
#define false 0
#define true  1

#define SET_BIT(REG, BIT)     ((REG) |= (BIT))
#define CLEAR_BIT(REG, BIT)   ((REG) &= ~(BIT))
#define READ_BIT(REG, BIT)    ((REG) & (BIT))

#define WORD(a, b) ((a << 8) | b)

#define BREAKPOINT(addr) if(cpu.pc == addr) system("pause")

#define KEEP(x)     (cpu.af.flags & x)
#define ZFLAG(x)    ((x & 0xFF) == 0 ? Z_FLAG : 0)
#define ZFLAG16(x)    ((x & 0xFFFF) == 0 ? Z_FLAG : 0)
//#define HFLAG_ADD8(a,b)  ((((a) & 0x0F) + ((b) & 0x0F)) & 0x10 ? H_FLAG : 0)
#define HFLAG8(a,b,c)  (((a) ^ (b) ^ (c)) & 0x10 ? H_FLAG : 0)
#define HFLAG16(a,b,c)  (((a) ^ (b) ^ (c)) & 0x1000 ? H_FLAG : 0)
#define HFLAG_ADD16(a,b)  ((((a) & 0x0F00) + ((b) & 0x0F00)) & 0x1000 ? H_FLAG : 0)
#define HFLAG_SUB8(a,b)  ((((a) & 0x0F) - ((b) & 0x0F)) < 0  ? H_FLAG : 0)
#define CFLAG_ADD8(res)  (res > 0xFF ? C_FLAG : 0)
#define CFLAG_ADD16(res)  (res > 0xFFFF ? C_FLAG : 0)
#define CFLAG_SUB8(res)  (res < 0 ? C_FLAG : 0)
#define HFLAG_INC(x) ((x & 0xF) == 0 ? H_FLAG : 0)
#define HFLAG_DEC(x) ((x & 0xF) == 0xF ? H_FLAG : 0)

#endif
