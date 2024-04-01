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

#include <stdio.h>
#include <stdlib.h>
#include "gb.h"

CLOCK clock;
CartridgeHeader *cart_header;

void gb_reset()
{
    clock.m_clock = 0;
    clock.t_clock = 0;
    mmu.in_bios = 1;
    clock.double_speed = false;
}

int gb_load(char *file)
{
    FILE *rom;
    int size;
    rom = fopen(file, "rb");

    if(!file)
        return -1;

    fseek(rom, 0, SEEK_END);
    size = ftell(rom);
    rewind(rom);
    mmu.rom = malloc(size);
    cart_header = (CartridgeHeader *)(mmu.rom + 0x100);
    fread(mmu.rom, size, 1, rom);
    printf("mbc: 0x%x\n", cart_header->cart_type);
    mbc.bk_cnt = (size >> 14) - 1;
    fclose(rom);

    return 0;
}

static unsigned int p14 = 0xFF;
static unsigned int p15 = 0xFF;
void gb_key_down(unsigned int keycode)
{
    SET_BIT(mmu.iflg, P10_P13_INT_FLAG);
    switch(keycode)
    {
        case GB_A_KEY:
            CLEAR_BIT(p14, 0x01);
            break;

        case GB_B_KEY:
            CLEAR_BIT(p14, 0x02);
            break;

        case GB_SELECT_KEY:
            CLEAR_BIT(p14, 0x04);
            break;

        case GB_START_KEY:
            CLEAR_BIT(p14, 0x08);
            break;

        case GB_RIGHT_KEY:
            CLEAR_BIT(p15, 0x01);
            break;

        case GB_LEFT_KEY:
            CLEAR_BIT(p15, 0x02);
            break;

        case GB_UP_KEY:
            CLEAR_BIT(p15, 0x04);
            break;

        case GB_DOWN_KEY:
            CLEAR_BIT(p15, 0x08);
            break;
    }

}

void gb_key_up(unsigned int keycode)
{
    SET_BIT(mmu.iflg, P10_P13_INT_FLAG);
    switch(keycode)
    {
        case GB_A_KEY:
            SET_BIT(p14, 0x01);
            break;

        case GB_B_KEY:
            SET_BIT(p14, 0x02);
            break;

        case GB_SELECT_KEY:
            SET_BIT(p14, 0x04);
            break;

        case GB_START_KEY:
            SET_BIT(p14, 0x08);
            break;

        case GB_RIGHT_KEY:
            SET_BIT(p15, 0x01);
            break;

        case GB_LEFT_KEY:
            SET_BIT(p15, 0x02);
            break;

        case GB_UP_KEY:
            SET_BIT(p15, 0x04);
            break;

        case GB_DOWN_KEY:
            SET_BIT(p15, 0x08);
            break;
    }
}

INLINE static void key_step()
{
    int cmd = mmu.p1;
    mmu.p1 = 0x00;
    switch(cmd & 0x30)
    {
        /* P14 - A:0, B:1, SELECT:2, START:3 */
        case 0x10:

            SET_BIT(mmu.p1, 0x10);
            mmu.p1 |= p14 & 0xF;
            break;

        /* P15 - RIGHT:0, LEFT:1, UP:2, DOWN:3 */
        case 0x20:
            //...

            SET_BIT(mmu.p1, 0x20);
            mmu.p1 |= p15 & 0xF;
            break;

        /* RESET */
        case 0x30:
            mmu.p1 = 0xFF;
            break;
    }
}

static unsigned int div_clk = 0;
static unsigned int tim_clk = 0;
static unsigned int tima = 0;
static const unsigned int tim_clks[] = { 1024, 16, 64, 256 };
INLINE static void timer_step()
{
    div_clk += clock.t_clock;
    if(div_clk >= 256)
    {
        div_clk -= 256;
        mmu.div++;
    }

    if(mmu.tac & TIMER_START)
    {
        tim_clk += clock.t_clock;
        tima = mmu.tima;

        int clk_n = mmu.tac & 0x03;
        if(tim_clk >= tim_clks[clk_n])
        {
            int m = tim_clk / tim_clks[clk_n];
            tim_clk -= (tim_clks[clk_n] * m);
            tima += m;
        }

        if(tima > 255)
        {
            tima = mmu.tma;
            SET_BIT(mmu.iflg, TIMER_OVLW_INT_FLAG);
        }
        mmu.tima = tima;
    }
    else
    {
        tim_clk = 0;
    }
}

void gb_init()
{
    gb_reset();
    gpu_reset();
    cpu_reset();
    apu_reset();
}

INLINE void gb_step(void)
{
    key_step();
    cpu_step();
    gpu_step();
    timer_step();
    apu_step();
}
