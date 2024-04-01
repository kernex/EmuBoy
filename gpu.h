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

#ifndef __GPU_H__
#define __GPU_H__

typedef unsigned short fb_word_t;

#define WHITE       0xFFFFFFFF
#define LIGHT_GREY  0xFFC0C0C0
#define DARK_GREY   0xFF606060
#define BLACK       0xFF000000

#define DISPLAY_HEIGHT 144
#define DISPLAY_WIDTH  160

extern fb_word_t fb[];
extern fb_word_t pallete[];

void gpu_reset();
void gpu_step(void);
#endif
