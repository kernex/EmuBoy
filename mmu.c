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

#include "mmu.h"
#include "gb.h"

static const unsigned char bios[] =
{
  
};

MMU mmu;

static int bank_base_addr = 0x4000;
static int ram_base_addr = 0x1000;
static void oam_dma_transfer(unsigned short source)
{
    int i;
    for(i = 0; i < OAM_SIZE; i++)
    {
        mmu.oam.oam[i] = mmu_rb(source + i);
    }

}

void h_dma_transfer()
{
    if(mmu.hdma_enable)
    {
        if(mmu.hdma_lines > 0)
        {
            int i;
            int offset = (((mmu.hdma5 & 0x7F) + 1) - mmu.hdma_lines) << 4;
            unsigned short src = (mmu.hdma1 << 8) | (mmu.hdma2 & 0xF0);
            unsigned short dst = 0x8000 + (((mmu.hdma3 & 0x1F) << 8 ) | (mmu.hdma4 & 0xF0));
            src += offset;
            dst += offset;
            for(i = 0; i < 16; i++)
            {
                mmu_wb(dst + i, mmu_rb(src + i));
            }
            mmu.hdma_lines--;
        }
        else
        {
            mmu.hdma_enable = false;
            SET_BIT(mmu.hdma5, 0x80);
        }
    }


}

static void dma_transfer()
{
    int stat = (mmu.stat & 0x3);
    if(stat == 2 || stat == 3)
        return;
    int i;
    int length = ((mmu.hdma5 & 0x7F) + 1) << 4;
    unsigned short src = (mmu.hdma1 << 8) | (mmu.hdma2 & 0xF0);
    unsigned short dst = ((mmu.hdma3 & 0x1F) << 8 ) | (mmu.hdma4 & 0xF0);
    unsigned short tmp = src + length;
    mmu.hdma2 = tmp & 0xFF;
    mmu.hdma1 = tmp >> 8;
    tmp = dst + length;
    mmu.hdma4 = tmp & 0xFF;
    mmu.hdma3 = tmp >> 8;
    dst += 0x8000;
    //mmu.hdma5=0x80;
    //printf("src:0x%x -> dst:0x%x\n", src, dst);
    for(i = 0; i < length; i++)
    {
        mmu_wb(dst + i, mmu_rb(src + i));
    }
}

unsigned char mmu_rb(unsigned short addr)
{
    switch(addr & 0xF000)
    {
        case 0x0000:
            if(addr < 0x0100 || addr > 0x0200)
                if(mmu.in_bios)
                    return bios[addr];
        case 0x1000:
        case 0x2000:
        case 0x3000:
            return mmu.rom[addr];

        case 0x4000:
        case 0x5000:
        case 0x6000:
        case 0x7000:
            return mmu.rom[mbc.rom_base_addr + (addr & 0x3fff)];

        case 0x8000:
        case 0x9000:
            if(mmu.vbk)
                return mmu.vram2.vram[addr & 0x1FFF];
            else
                return mmu.vram.vram[addr & 0x1FFF];

        case 0xA000:
        case 0xB000:
            if(mbc.ext_ram_enabled)
                return mbc.ext_ram[mbc.ext_ram_base_addr + (addr & 0x1FFF)];
            break;
        ///*
        case 0xC000:
            return mmu.ram[addr & 0xFFF];
        case 0xD000:
            return mmu.ram[ram_base_addr + (addr & 0xFFF)];
        //*/

        case 0xE000:
            //printf("0xE000\n");
            return 0;

        case 0xF000:
            switch(addr & 0xFF00)
            {
                //case 0xFD00:
                    //return mmu.ram[addr & 0x1FFF];

                case 0xFE00:
                {
                    int adr0 = addr & 0xFF;
                    if(adr0 < OAM_SIZE)
                        return mmu.oam.oam[adr0];
                }


                case 0xFF00:
                {
                    //if(addr >= 0xff10 && addr <= 0xff26)
                        //printf("0x%x\n", addr);

                    if(addr == 0xFF69)
                        return mmu.bg_pal_ram.ram[mmu.bgpi & 0x3F];
                    else
                    if(addr == 0xFF6B)
                        return mmu.obj_pal_ram.ram[mmu.obpi & 0x3F];
                    else
                        return *(&mmu.p1 + (addr & 0x00FF));
                }
            }

    }

    return 0;
}

INLINE unsigned short mmu_rw(unsigned short addr)
{
    return mmu_rb(addr) | (mmu_rb(addr + 1) << 8);
}

void mmu_wb(unsigned short addr, unsigned char byte)
{
    switch(addr & 0xF000)
    {
        case 0x0000:
        case 0x1000:
        case 0x2000:
        case 0x3000:
        case 0x4000:
        case 0x5000:
        case 0x6000:
        case 0x7000:
            mbc_wb(addr & 0xF000, byte);
            break;

        case 0x8000:
        case 0x9000:
            if(mmu.vbk)
                mmu.vram2.vram[addr & 0x1FFF] = byte;
            else
                mmu.vram.vram[addr & 0x1FFF] = byte;
            break;

        case 0xA000:
        case 0xB000:
            if(mbc.ext_ram_enabled)
                mbc.ext_ram[mbc.ext_ram_base_addr + (addr & 0x1FFF)] = byte;
            break;

        ///*
         case 0xC000:
            mmu.ram[addr & 0xFFF] = byte;
            break;
        case 0xD000:
            mmu.ram[ram_base_addr + (addr & 0xFFF)] = byte;
            break;

        case 0xE000:
            //printf("0xE000: %d\n", byte);
            break;
       // */
        /*
        case 0xC000:
        case 0xD000:
            mmu.ram[addr & 0x1FFF] = byte;
            break;
*/
        case 0xF000:
            switch(addr & 0xFF00)
            {
               // case 0xFD00:
                  //  mmu.ram[addr & 0x1FFF] = byte;
                   // break;

                case 0xFE00:
                {
                    int adr0 = addr & 0xFF;
                    if(adr0 < OAM_SIZE)
                        mmu.oam.oam[adr0] = byte;
                }

                    //printf("oam:0x%x!!!; addr:0x%x\n", mmu.oam.oam[addr & 0x1FFF], (addr & 0x1FFF));
                    break;

                case 0xFF00:

                    *(&mmu.p1 + (addr & 0x00FF)) = byte;

                    if(addr == 0xff45)
                    {
                       // printf("LYC%d\n", byte);


                    }
                    apu_wb(addr, byte);
                    if(addr == 0xff1a)
                    {
                        //printf("%d\n", byte);
                        if(byte)
                            SET_BIT(mmu.nr52, 4);
                        else
                            CLEAR_BIT(mmu.nr52, 4);

                    }

                    if(addr == 0xff55)
                    {
                        //if(mmu.hdma5 & 0x80)
                            //printf("DMA-HBLANK!!!\n");
                        //else
                            //printf("NORMAL DMA!!!\n");
                        //printf("%d\n", byte);

                        if(mmu.hdma5 & 0x80)
                        {
                            //printf("DMA-HBLANK!!!\n");
                            mmu.hdma_lines = (mmu.hdma5 & 0x7F) + 1;
                            CLEAR_BIT(mmu.hdma5, 0x80);
                            mmu.hdma_enable = true;
                        }
                        else
                        {
                            dma_transfer();
                            SET_BIT(mmu.hdma5, 0x80);
                            mmu.hdma_enable = false;
                            //printf("DMA:%d\n", (mmu.hdma5 & 0x7F) + 1);
                            /*
                            if(clock.double_speed)
                                clock.t_clock += ((mmu.hdma5 + 1) * 64) + 461;
                            else
                                clock.t_clock += ((mmu.hdma5 + 1) * 32) + 923;
                                */
                            //clock.t_clock += ((mmu.hdma5 + 1) * 32);
                        }


                    }
                    else
                    if(addr == 0xff4d)
                    {
                        //printf("KEY1=%d:%d\n", mmu.key1, byte);
                        //printf("%d\n", byte);
                        if(clock.double_speed)
                            mmu.key1 = 0x80;
                        if(mmu.key1 & 0x1)
                        {
                            if(clock.double_speed)
                            {
                                clock.double_speed = false;
                                mmu.key1 = 0x00;
                            }
                            else
                            {
                                clock.double_speed = true;
                                mmu.key1 = 0x80;
                            }
                        }
                    }
                    else
                    if(addr == 0xFF69)
                    {
                        mmu.bg_pal_ram.ram[mmu.bgpi & 0x3F] = byte;
                        if(mmu.bgpi & 0x80)
                        {
                            mmu.bgpi++;
                            if(mmu.bgpi > 0xBF)
                                mmu.bgpi = 0x80;
                        }
                    }
                    else
                    if(addr == 0xFF6B)
                    {
                        mmu.obj_pal_ram.ram[mmu.obpi & 0x3F] = byte;
                        if(mmu.obpi & 0x80)
                        {
                            mmu.obpi++;
                            if(mmu.obpi > 0xBF)
                                mmu.obpi = 0x80;
                        }
                    }
                    else
                    if(addr == 0xFF70)
                    {
                        int bank = mmu.svbk & 0x7;
                        if(!bank)
                            bank = 1;
                        ram_base_addr = 0x1000 * bank;
                        //printf("bank=%d\n",  bank);
                    }
                    else
                    if(addr == 0xFF04)
                        mmu.div = 0;
                    else
                    if(addr == 0xFF46)
                        oam_dma_transfer(byte << 8);//multiplo de 0x100
                    break;
            }
            break;
    }
}

INLINE void mmu_ww(unsigned short addr, unsigned short word)
{
    mmu_wb(addr, word & 0xFF);
    mmu_wb(addr + 1, word >> 8);
}
