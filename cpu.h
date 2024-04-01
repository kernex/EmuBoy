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

#ifndef __CPU_H__
#define __CPU_H__

/* Zero Flag*/
#define Z_FLAG 0x80
/* Subtract Flag  */
#define N_FLAG 0x40
/* Half Carry Flag  */
#define H_FLAG 0x20
/* Carry Flag  */
#define C_FLAG 0x10

enum
{
    NOP,

};

typedef  struct _INSTR_
{
	void (*exec)();
	int t_clock;
}CPU_OPS;

typedef struct _REGS_
{
	union
	{
	    unsigned short AF;
	    struct
	    {
	        unsigned char flags;
	        unsigned char A;
	    }af;
	};
	union
	{
	    unsigned short BC;
	    struct
	    {
	        unsigned char C;
	        unsigned char B;
	    }bc;
	};
	union
	{
	    unsigned short DE;
	    struct
	    {
            unsigned char E;
	        unsigned char D;
	    }de;
	};
	union
	{
	    unsigned short HL;
	    struct
	    {
	        unsigned char L;
	        unsigned char H;
	    }hl;
	};

	/* Stack Pointer */
	unsigned short sp;
	/* Program Counter */
	unsigned short pc;
    /* Interrupt Master Enable */
	unsigned int ime;

}CPU;

void cpu_reset();
void cpu_step(void);

#endif
