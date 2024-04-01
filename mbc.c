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

#include "gb.h"
#include "mbc.h"

MBC mbc = { 0x4000, 0, 0, 0, 0, 0, 0, 0 };

unsigned int cart_ram_size()
{
    switch(cart_header->ram_size)
    {
        case 0x1:
            return 2048;
        case 0x2:
            return 8192;
        case 0x3:
            return 32768;
        case 0x4:
            return 131072;
        case 0x5:
            return 65536;
    }
    return 0;
}

static void mbc1_wb(unsigned short addr, unsigned char byte)
{
    switch(addr)
    {
        case 0x0000:
        case 0x1000:
            if(byte == 0x0A)
            {
                mbc.ext_ram_enabled = 1;
                if(!mbc.ext_ram)
                    mbc.ext_ram = malloc(cart_ram_size());
            }
            else
            {
                mbc.ext_ram_enabled = 0;
            }
            break;

        case 0x2000:
        case 0x3000:
            mbc.rom_bk_select &= 0xE0;
            if(!byte)
                mbc.rom_bk_select |= 1;
            else
                mbc.rom_bk_select |= (byte & 0x1f);
            mbc.rom_bk_select &= mbc.bk_cnt;
            mbc.rom_base_addr = 0x4000 * mbc.rom_bk_select;
            break;

        case 0x4000:
        case 0x5000:
            if(mbc.mode_select)
            {
                mbc.ext_ram_base_addr = 0x2000 * (byte & 0x3);
            }
            else
            {
                mbc.rom_bk_select &= 0x1f;
                mbc.rom_bk_select |= ((byte & 0x3) << 5);
                mbc.rom_bk_select &= mbc.bk_cnt;
                mbc.rom_base_addr = 0x4000 * mbc.rom_bk_select;
            }
            break;

        case 0x6000:
        case 0x7000:
            mbc.mode_select = byte & 0x01;
            break;
    }
}

static void mbc3_wb(unsigned short addr, unsigned char byte)
{
    switch(addr)
    {
        case 0x0000:
        case 0x1000:
            if(byte == 0x0A)
            {
                mbc.ext_ram_enabled = 1;
                if(!mbc.ext_ram)
                    mbc.ext_ram = malloc(cart_ram_size());
            }
            else
            {
                mbc.ext_ram_enabled = 0;
            }
            break;

        case 0x2000:
        case 0x3000:
            if(!byte)
                mbc.rom_bk_select = 1;
            else
                mbc.rom_bk_select = (byte & 0x7F);
            mbc.rom_bk_select &= mbc.bk_cnt;
            mbc.rom_base_addr = 0x4000 * mbc.rom_bk_select;
            break;

        case 0x4000:
        case 0x5000:
                mbc.ext_ram_base_addr = 0x2000 * (byte & 0x3);
            break;

        case 0x6000:
        case 0x7000:

            break;
    }

}

static void mbc5_wb(unsigned short addr, unsigned char byte)
{
    switch(addr)
    {
        case 0x0000:
        case 0x1000:
            if(byte == 0x0A)
            {
                mbc.ext_ram_enabled = 1;
                if(!mbc.ext_ram)
                    mbc.ext_ram = malloc(cart_ram_size());
            }
            else
            {
                mbc.ext_ram_enabled = 0;
            }
            break;

        case 0x2000:
            mbc.rom_bk_select &= 0x100;
            if(!byte)
                mbc.rom_bk_select |= 1;
            else
                mbc.rom_bk_select |= byte;
            mbc.rom_bk_select &= mbc.bk_cnt;
            mbc.rom_base_addr = 0x4000 * mbc.rom_bk_select;
            break;

        case 0x3000:
            mbc.rom_bk_select &= 0xFF;
            mbc.rom_bk_select |= ((byte & 0x1) << 8);
            mbc.rom_bk_select &= mbc.bk_cnt;
            mbc.rom_base_addr = 0x4000 * mbc.rom_bk_select;

            break;

        case 0x4000:
        case 0x5000:
            mbc.ext_ram_base_addr = 0x2000 * (byte & 0xF);
            break;
    }
}

INLINE void mbc_wb(unsigned short addr, unsigned char byte)
{
    switch(cart_header->cart_type)
    {
        case 0x01:
        case 0x02:
        case 0x03:
            mbc1_wb(addr, byte);
        break;

        case 0x19:
        case 0x1A:
        case 0x1B:
        case 0x1C:
        case 0x1D:
        case 0x1E:
            mbc5_wb(addr, byte);
        break;

        case 0x0F:
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
            mbc3_wb(addr, byte);
        break;

    }
}
