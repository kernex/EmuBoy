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
#include <string.h>
#include "gb.h"

static unsigned int clk;
static unsigned int mode;
static char line_color_cache[DISPLAY_WIDTH];
static char bg_priority_cache[DISPLAY_WIDTH];

void scanline()
{
    int x, i, j, k;
    int x0, y0;
    int has_w = 0;
    int w_y, bg_y;

    if(mmu.ly + mmu.scy >= 256)
        bg_y = mmu.ly + mmu.scy - 256;
    else
        bg_y = mmu.ly + mmu.scy;

    if((mmu.lcdc & LCDC_WINDOW_ENABLE_BIT) && (mmu.ly >= mmu.wy))
    {
        w_y = mmu.ly - mmu.wy;
        has_w = 1;
    }

    //if(mmu.lcdc & LCDC_BG_ENABLE_BIT)//somente para dmg
    {
        for(x = 0; x < DISPLAY_WIDTH; x++)
        {
            if(x + mmu.scx >= 256)
                x0 = x + mmu.scx - 256;
            else
                x0 = x + mmu.scx;

            int tile_n, pal_n, tileset, flip_x, flip_y, bg_priority;
            if(has_w && (x >= mmu.wx - 7))
            {
                x0 = x - mmu.wx + 7;
                y0 = w_y;
                int strt_pos = (y0 >> 3) << 5;//(mmu.ly / 8) * 32
                int index = strt_pos + (x0 >> 3);//x / 8;
                if(mmu.lcdc & LCDC_WINDOW_TILEMAP_BIT)
                {
                    tile_n = mmu.vram.vram_s.map1[index];
                    pal_n = mmu.vram2.vram_s.map1_attr[index] & 0x7;
                    tileset = (mmu.vram2.vram_s.map1_attr[index] >> 3) & 0x1;
                    flip_x = mmu.vram2.vram_s.map1_attr[index] & 0x20;
                    flip_y = mmu.vram2.vram_s.map1_attr[index] & 0x40;
                    bg_priority = mmu.vram2.vram_s.map1_attr[index] & 0x80;
                }
                else
                {
                    tile_n = mmu.vram.vram_s.map0[index];
                    pal_n = mmu.vram2.vram_s.map0_attr[index] & 0x7;
                    tileset = (mmu.vram2.vram_s.map0_attr[index] >> 3) & 0x1;
                    flip_x = mmu.vram2.vram_s.map0_attr[index] & 0x20;
                    flip_y = mmu.vram2.vram_s.map0_attr[index] & 0x40;
                    bg_priority = mmu.vram2.vram_s.map0_attr[index] & 0x80;
                }

            }
            else
            {
                y0 = bg_y;
                int strt_pos = (y0 >> 3) << 5;//(mmu.ly / 8) * 32
                int index = strt_pos + (x0 >> 3);//x / 8;
                if(mmu.lcdc & LCDC_BG_TILEMAP_BIT)
                {
                    tile_n = mmu.vram.vram_s.map1[index];
                    pal_n = mmu.vram2.vram_s.map1_attr[index] & 0x7;
                    tileset = (mmu.vram2.vram_s.map1_attr[index] >> 3) & 0x1;
                    flip_x = mmu.vram2.vram_s.map1_attr[index] & 0x20;
                    flip_y = mmu.vram2.vram_s.map1_attr[index] & 0x40;
                    bg_priority = mmu.vram2.vram_s.map1_attr[index] & 0x80;
                }
                else
                {
                    tile_n = mmu.vram.vram_s.map0[index];
                    pal_n = mmu.vram2.vram_s.map0_attr[index] & 0x7;
                    tileset = (mmu.vram2.vram_s.map0_attr[index] >> 3) & 0x1;
                    flip_x = mmu.vram2.vram_s.map0_attr[index] & 0x20;
                    flip_y = mmu.vram2.vram_s.map0_attr[index] & 0x40;
                    bg_priority = mmu.vram2.vram_s.map0_attr[index] & 0x80;
                }

            }

            i = tile_n * TILE_SIZE;
            /* verifica se o tileset começa em 0x8800 */
            if(!(mmu.lcdc & LCDC_BG_TILESET_BIT))
            {
                //0x8800 selecionado

                /* pula para a região 0x9000
                caso o tile seja do tileset0.
                tile_addr = 0x9000 +  tile_n * TILE_SIZE
                */
                if(tile_n < 128)
                    i += 4096;
            }

            int tile_x = x0 & 0x7;//x % 8
            int tile_y;
            if(flip_y)
                tile_y = (7 - (y0 & 0x7)) << 1;
            else
                tile_y = (y0 & 0x7) << 1;//(mmu.ly % 8) * 2
            i += tile_y;
             int xx;
            if(flip_x)
                xx = (tile_x);
            else
                xx = (7 - tile_x);

            int c;
            if(tileset)
                c = (mmu.vram2.vram_s.tileset[i] >> xx & 0x1) | ((mmu.vram2.vram_s.tileset[i + 1] >> xx & 0x1) << 1);
            else
                c = (mmu.vram.vram_s.tileset[i] >> xx & 0x1) | ((mmu.vram.vram_s.tileset[i + 1] >> xx & 0x1) << 1);
            //fb[mmu.ly * DISPLAY_WIDTH  + x] = palette[(mmu.bgp >> (c << 1)) & 0x3];
            //if(mmu.bg_pal_ram.bg_pal[pal_n][c] == 575)
                //break;
            fb[mmu.ly * DISPLAY_WIDTH  + x] = mmu.bg_pal_ram.bg_pal[pal_n][c];
            if(bg_priority && c)
                bg_priority_cache[x] = 1;
            else
                bg_priority_cache[x] = 0;
            line_color_cache[x] = c;
        }
    }
    //habilitar somente quando estiver emulando
    if(!(mmu.lcdc & LCDC_SPRITE_ENABLE_BIT))
        return;

    int sprite_h = (mmu.lcdc & LCDC_SPRITE_SIZE_BIT ? 16 : 8);
    for(j = MAX_SPRITE-1; j >= 0 ; j--)
    {
        /*
        todos os sprites que tiverem x < 8 ou y < 16 estão fora
        da tela e não serão desenhados
        */

        //if((mmu.oam.oam_s[j].x <  8) || (mmu.oam.oam_s[j].y < 16))
            //continue;

        int sprite_y = mmu.oam.oam_s[j].y - 16;
        int sprint_x = mmu.oam.oam_s[j].x - 8;

        if(!((sprite_y <=  mmu.ly) && (sprite_y + sprite_h > mmu.ly)))
            continue;

        int n = mmu.oam.oam_s[j].tile_n;
        /* caso for 8x16 alinha o numero do tile */
        if(mmu.lcdc & LCDC_SPRITE_SIZE_BIT)
            n &= 0xFE;

        y0 = mmu.ly;
        i = n * TILE_SIZE;
        if((mmu.oam.oam_s[j].options & OAM_Y_FLIP_BIT))
            i += ((sprite_h - 1 - (mmu.ly - sprite_y)) << 1);
        else
            i += ((mmu.ly - sprite_y) << 1);

        int pal_n, tileset;
        pal_n = mmu.oam.oam_s[j].options & 0x7;
        tileset = (mmu.oam.oam_s[j].options >> 3) & 0x1;
        for(k = 0; k < 8; k++)
        {
            int obj_x;
            if(mmu.oam.oam_s[j].options & OAM_X_FLIP_BIT)
                obj_x =  sprint_x + (7 - k);
            else
                obj_x =  sprint_x + k;

            if(obj_x < 0 || obj_x > DISPLAY_WIDTH)
                continue;

            if(bg_priority_cache[obj_x])
                continue;

            int c;

            if(tileset)
                c = (mmu.vram2.vram_s.tileset[i] >> (7 - k) & 0x1) | ((mmu.vram2.vram_s.tileset[i + 1] >> (7 - k) & 0x1) << 1);
            else
                c = (mmu.vram.vram_s.tileset[i] >> (7 - k) & 0x1) | ((mmu.vram.vram_s.tileset[i + 1] >> (7 - k) & 0x1) << 1);

            if(!c)
                continue;

            if((mmu.oam.oam_s[j].options & OAM_PRIORITY_BIT) && line_color_cache[obj_x])
                continue;


            fb[(y0 * DISPLAY_WIDTH) + obj_x] = mmu.obj_pal_ram.obj_pal[pal_n][c];
        }

    }
}

void gpu_reset()
{
    /*
    mmu.lcdc = 0xe9;
    mmu.scy = 0xa7;
    mmu.scx = 0x34;
    mmu.bgp = 0xe4;
    mmu.obp0 = 0xe4;
    mmu.obp1 = 0xe4;
    mmu.wy = 0x70;
    mmu.wx = 0x07;
    */

    clk = 0;
    mode = 3;
    SET_BIT(mmu.stat, 3);
    mmu.ly = 0;
    mmu.lcdc = 0x91;
}

INLINE static void ly_compare(void)
{
    if(mmu.ly == mmu.lyc)
    {
        SET_BIT(mmu.stat, STAT_COINCIDENCE_FLAG);
        if(mmu.stat & STAT_INT_LCY)
            SET_BIT(mmu.iflg, LCDC_STAT_INT_FLAG);
    }
    else
    {
        CLEAR_BIT(mmu.stat, STAT_COINCIDENCE_FLAG);
    }
}

bool delay;
INLINE void gpu_step(void)
{
    if(!(mmu.lcdc & LCDC_DISPLAY_ENABLE))
    {
        clk=0;
        mode=3;
        CLEAR_BIT(mmu.stat, 3);
        mmu.ly=0;
        //delay = true;
        return;
    }

    if(clock.double_speed)
        clk += (clock.t_clock>>1);
    else
        clk += clock.t_clock;

    //somente gbc?
    /*
    if(delay)
    {
        if (clk <= 140448)
        {
            return;
        }
        else
        {
            clk -= 140448;
            delay = false;
        }
    }
    */

    switch(mode)
    {
        case HBLANK_FLAG:
            if(clk >= 204)
            {
                clk -= 204;
                mmu.ly++;
                ly_compare();
                if(mmu.ly == 144)
                {
                    mode = VBLANK_FLAG;
                    SET_BIT(mmu.iflg, VBLANK_INT_FLAG);
                    if(mmu.stat & STAT_INT_VBLANK)
                        SET_BIT(mmu.iflg, LCDC_STAT_INT_FLAG);
                }
                else
                {
                    mode = OAM_FLAG;
                    if(mmu.stat & STAT_INT_OAM)
                        SET_BIT(mmu.iflg, LCDC_STAT_INT_FLAG);
                }
                SET_BIT(mmu.stat, mode);
            }
        break;

        case VBLANK_FLAG:
            if(clk >= 456)
            {
                clk -= 456;
                mmu.ly++;

                ly_compare();

                if(mmu.ly == 154)
                {
                    mmu.ly = 0;
                    ly_compare();
                    mode = OAM_FLAG;
                    CLEAR_BIT(mmu.stat, STAT_INT_FLAG_MASK);
                    SET_BIT(mmu.stat, mode);
                    if(mmu.stat & STAT_INT_OAM)
                        SET_BIT(mmu.iflg, LCDC_STAT_INT_FLAG);
                    repaint();
                }
                /*
                if(mmu.ly==153)
                {
                    mmu.ly=0;
                    ly_compare();
                    mmu.ly=153;
                }
                */
            }
        break;

        case OAM_FLAG:
            if(clk >= 80)
            {
                clk -= 80;
                mode = RAM_FLAG;
                CLEAR_BIT(mmu.stat, STAT_INT_FLAG_MASK);
                SET_BIT(mmu.stat, RAM_FLAG);
            }
        break;

        case RAM_FLAG:
            if(clk >= 172)
            {
                clk -= 172;
                mode = HBLANK_FLAG;
                h_dma_transfer();
                scanline();
                CLEAR_BIT(mmu.stat, STAT_INT_FLAG_MASK);
                SET_BIT(mmu.stat, mode);
                if(mmu.stat & STAT_INT_HBLANK)
                    SET_BIT(mmu.iflg, LCDC_STAT_INT_FLAG);
            }
        break;
    }
}
