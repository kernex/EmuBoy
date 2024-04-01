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

#ifndef __MMU_H__
#define __MMU_H__

#include "macros.h"

#define RAM_SIZE        32768
#define VRAM_SIZE       8192
#define TILESET_SIZE    6144
#define TILE_SIZE       16
#define MAP_SIZE        1024
#define OAM_SIZE        160
#define MAX_SPRITE      40
#define PAL_N           8
#define PRAM_SIZE       64

typedef struct __attribute__ ((__packed__)) MMU_STRUCT
{
    unsigned char *rom;
    unsigned char ram[RAM_SIZE];
    union
    {
        unsigned char vram[VRAM_SIZE];
        struct
        {
            unsigned char tileset[TILESET_SIZE];
            unsigned char    map0[MAP_SIZE];
            unsigned char    map1[MAP_SIZE];
        }vram_s;
    }vram;

    union
    {
        unsigned char vram[VRAM_SIZE];
        struct
        {
            unsigned char tileset[TILESET_SIZE];
            unsigned char    map0_attr[MAP_SIZE];
            unsigned char    map1_attr[MAP_SIZE];
        }vram_s;
    }vram2;

    union
    {
        unsigned char oam[OAM_SIZE];
        struct
        {
            unsigned char y;
            unsigned char x;
            unsigned char tile_n;
            unsigned char options;
        }oam_s[MAX_SPRITE];
    }oam;


    union
    {
        unsigned short bg_pal[PAL_N][4];
        unsigned char  ram[PRAM_SIZE];
    }bg_pal_ram;

    union
    {
        unsigned short obj_pal[PAL_N][4];
        unsigned char  ram[PRAM_SIZE];
    }obj_pal_ram;

    unsigned char bank_select;
    unsigned char in_bios;
    unsigned int hdma_clk;
    unsigned int hdma_enable;
    unsigned int gdma_clk;
    unsigned int gdma_enable;
    unsigned int hdma_cycles;
    unsigned int hdma_lines;

    unsigned char p1;
    unsigned char sb;
    unsigned char sc;
    unsigned char reserv1;
    unsigned char div;
    unsigned char tima;
    unsigned char tma;
    unsigned char tac;
    unsigned char reserv2[7];
    unsigned char iflg;
    unsigned char nr10;
    unsigned char nr11;
    unsigned char nr12;
    unsigned char nr13;
    unsigned char nr14;
    unsigned char reserv3;
    unsigned char nr21;
    unsigned char nr22;
    unsigned char nr23;
    unsigned char nr24;
    unsigned char nr30;
    unsigned char nr31;
    unsigned char nr32;
    unsigned char nr33;
    unsigned char nr34;
    unsigned char reserv4;
    unsigned char nr41;
    unsigned char nr42;
    unsigned char nr43;
    unsigned char nr44;
    unsigned char nr50;
    unsigned char nr51;
    unsigned char nr52;
    unsigned char reserv5[9];
    unsigned char waveram[16];
    unsigned char lcdc;
    unsigned char stat;
    unsigned char scy;
    unsigned char scx;
    unsigned char ly;
    unsigned char lyc;
    unsigned char dma;
    unsigned char bgp;
    unsigned char obp0;
    unsigned char obp1;
    unsigned char wy;
    unsigned char wx;
    /* ====== GBC ====== */
    unsigned char reserv6;
    unsigned char key1;
    unsigned char reserv7;
    unsigned char vbk;
    unsigned char btr_lck;
    unsigned char hdma1;
    unsigned char hdma2;
    unsigned char hdma3;
    unsigned char hdma4;
    unsigned char hdma5;
    unsigned char rp;
    unsigned char reserv8[17];
    unsigned char bgpi;
    unsigned char bgpd;
    unsigned char obpi;
    unsigned char obpd;
    unsigned char reserv9[4];
    unsigned char svbk;
    /* ================= */
    unsigned char reserv10[15];
    unsigned char wram[127];
    unsigned char ie;
}MMU;

unsigned char mmu_rb(unsigned short addr);
unsigned short mmu_rw(unsigned short addr);
void mmu_wb(unsigned short addr, unsigned char byte);
void mmu_ww(unsigned short addr, unsigned short word);
void gdma_step();
#endif
