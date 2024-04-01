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

#ifndef __GB_H__
#define __GB_H__

#include <stdlib.h>
#include "macros.h"
#include "cpu.h"
#include "gpu.h"
#include "mmu.h"
#include "mbc.h"

#define LCDC_BG_ENABLE_BIT      0x01
#define LCDC_SPRITE_ENABLE_BIT  0x02
#define LCDC_SPRITE_SIZE_BIT    0x04
#define LCDC_BG_TILEMAP_BIT     0x08
#define LCDC_BG_TILESET_BIT     0x10
#define LCDC_WINDOW_ENABLE_BIT  0x20
#define LCDC_WINDOW_TILEMAP_BIT 0x40
#define LCDC_DISPLAY_ENABLE     0x80

#define OAM_PALETTE_BIT  0x10
#define OAM_X_FLIP_BIT   0x20
#define OAM_Y_FLIP_BIT   0x40
#define OAM_PRIORITY_BIT 0x80

#define TIMER_START     0x04
#define TIMER_4096_HZ   0x00
#define TIMER_262144_HZ 0x01
#define TIMER_65536_HZ  0x02
#define TIMER_16384_HZ  0x03

enum
{
    GB_A_KEY,
    GB_B_KEY,
    GB_SELECT_KEY,
    GB_START_KEY,
    GB_RIGHT_KEY,
    GB_LEFT_KEY,
    GB_UP_KEY,
    GB_DOWN_KEY,
};


enum
{
    HBLANK_FLAG,
    VBLANK_FLAG,
    OAM_FLAG,
    RAM_FLAG
};


#define STAT_INT_LCY          0x40
#define STAT_INT_OAM          0x20
#define STAT_INT_VBLANK       0x10
#define STAT_INT_HBLANK       0x08
#define STAT_INT_MASK         0x78

#define STAT_INT_FLAG_MASK    0x03
#define STAT_COINCIDENCE_FLAG 0x04

#define P10_P13_INT_FLAG    0x10
#define SERIAL_INT_FLAG     0x08
#define TIMER_OVLW_INT_FLAG 0x04
#define LCDC_STAT_INT_FLAG  0x02
#define VBLANK_INT_FLAG     0x01

typedef struct __attribute__ ((__packed__)) _HEADER_
{
	int entry;
	char nintendo_logo[48];
	char title[16];
	short new_lic_cod;
	char sgb_flag;
	char cart_type;
	char rom_size;
	char ram_size;
	char dest_code;
	char old_lic_cod;
	char mask_rom_ver_n;
	char header_chksum;
	short global_chksum;
}CartridgeHeader;

typedef struct _CLOCK_
{
    /* 1048576 Instr/s */
    int m_clock_total;
    int m_clock;
    /* 4194304 Hz*/
	int t_clock_total;
	int t_clock;
	bool double_speed;
}CLOCK;



extern CLOCK clock;
extern MMU mmu;
extern MBC mbc;
extern CartridgeHeader *cart_header;
extern void repaint();

void gb_key_down(unsigned int keycode);
void gb_key_up(unsigned int keycode);
void gb_init();
void gb_step(void);

#endif
