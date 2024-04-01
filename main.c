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
 
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <SDL.h>
#include "mmu.h"
#include "gpu.h"
#include "gb.h"

static bool refresh = false;
static bool flush = false;

fb_word_t fb[DISPLAY_WIDTH * DISPLAY_HEIGHT];
char audio_b[1024];

void key_check()
{
    SDL_Event event;
    SDL_PollEvent(&event);
    switch (event.type)
    {
        case SDL_KEYDOWN:
            switch( event.key.keysym.sym )
            {
                case SDLK_LEFT:
                    gb_key_down(GB_LEFT_KEY);
                    break;
                case SDLK_RIGHT:
                    gb_key_down(GB_RIGHT_KEY);
                    break;
                case SDLK_UP:
                    gb_key_down(GB_UP_KEY);
                    break;
                case SDLK_DOWN:
                    gb_key_down(GB_DOWN_KEY);
                    break;
                case SDLK_RETURN:
                    gb_key_down(GB_START_KEY);
                    break;
                case SDLK_p:
                    gb_key_down(GB_SELECT_KEY);
                    break;
                case SDLK_a:
                    gb_key_down(GB_B_KEY);
                    break;
                case SDLK_s:
                    gb_key_down(GB_A_KEY);
                    break;
                default:
                    break;
            }
            break;

            case SDL_KEYUP:
                switch( event.key.keysym.sym )
                {
                    case SDLK_LEFT:
                        gb_key_up(GB_LEFT_KEY);
                        break;
                    case SDLK_RIGHT:
                        gb_key_up(GB_RIGHT_KEY);
                        break;
                    case SDLK_UP:
                        gb_key_up(GB_UP_KEY);
                        break;
                    case SDLK_DOWN:
                        gb_key_up(GB_DOWN_KEY);
                        break;
                    case SDLK_RETURN:
                         gb_key_up(GB_START_KEY);
                        break;
                    case SDLK_p:
                        gb_key_up(GB_SELECT_KEY);
                        break;
                    case SDLK_a:
                        gb_key_up(GB_B_KEY);
                        break;
                    case SDLK_s:
                         gb_key_up(GB_A_KEY);
                        break;
                    default:
                        break;
                }
                break;
    }
}

void repaint()
{
    refresh = true;
}

void audio_flush()
{
    flush = true;
}

int main (int argc, char** argv)
{

    SDL_Window* window;
    SDL_Surface* screen;
    SDL_Surface *surface;
    SDL_AudioSpec want, have;
    SDL_AudioDeviceID dev;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0 )
    {
        printf("Falha na SDL: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow( "Kernex's EmuBoy", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, DISPLAY_WIDTH, DISPLAY_HEIGHT, SDL_WINDOW_SHOWN );
    if(!window)
    {
        printf("Falha na SDL: %s\n", SDL_GetError());
        return 1;
    }

    screen = SDL_GetWindowSurface(window);

    surface = SDL_CreateRGBSurfaceFrom((void*)fb,
                DISPLAY_WIDTH,
                DISPLAY_HEIGHT,
                16,
                DISPLAY_WIDTH * 2,
                0x001F,
                0x03E0,
                0x7C00,
                0);

    if(!surface)
    {
        printf("Falha na SDL: %s\n", SDL_GetError());
        return 1;
    }

    if (!surface)
    {
        printf("Falha na SDL: %s\n", SDL_GetError());
        return 1;
    }
    /*
    int j;
    for(j = 0; j < DISPLAY_WIDTH * DISPLAY_HEIGHT; j++)
        fb[j] = 0x54c0;

    SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 0, 0, 0));
            SDL_BlitSurface(surface, 0, screen, NULL);
            SDL_UpdateWindowSurface(window);
            while(1);
*/
    SDL_memset(&want, 0, sizeof(want));
    want.freq = 44100;
    want.format = AUDIO_S8;
    want.channels = 1;
    want.samples = 1024;
    want.callback = NULL;
    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
    if (!dev)
    {
        printf("Falha na SDL: %s\n", SDL_GetError());
        return 1;
    }
    SDL_PauseAudioDevice(dev, 0);
	
	if(gb_load(argv[1]))
        return;
	
    gb_init();
	
	int start_time = SDL_GetPerformanceCounter();
    while(1)
    {
        gb_step();
        if(flush)
        {
            flush = false;
            SDL_QueueAudio(dev, audio_b, 735);
        }

        if(refresh)
        {
            //SDL_QueueAudio(dev, audio_b, 2940);
            SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 0, 0, 0));
            SDL_BlitSurface(surface, 0, screen, NULL);
            SDL_UpdateWindowSurface(window);
            key_check();
            refresh = false;
            int freq = SDL_GetPerformanceFrequency();
            int end_time = SDL_GetPerformanceCounter();
            int t = (end_time - start_time);
            float elapsed_time = t / (float)freq * 1000.0f;
            int sleep_time = floor(16.72f - elapsed_time);
            SDL_Delay(sleep_time > 0 ? sleep_time : 0);
            start_time = SDL_GetPerformanceCounter();
        }
    }

    SDL_FreeSurface(surface);

    return 0;
}

