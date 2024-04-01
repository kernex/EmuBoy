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

extern char audio_b[];

void apu_wb(unsigned short addr, unsigned char byte);
struct APU
{
    unsigned int clk;
    unsigned int ch1_len_clk;
    unsigned int ch1_env_clk;
    unsigned int ch1_frq_clk;
    unsigned int ch1_swp_clk;
    unsigned int ch1_cur_vol;
    unsigned int ch1_len_cnt;
    signed   int ch1_env_cnt;
    signed   int ch1_swp_cnt;
    signed   int ch1_shadow_freq;
    unsigned int ch1_swp_shift;;
    signed   int ch1_sample;
    unsigned int ch1_frequency;
    unsigned int ch1_trigger;
    unsigned int ch1_enable;

    unsigned int ch2_len_clk;
    unsigned int ch2_env_clk;
    unsigned int ch2_frq_clk;
    unsigned int ch2_cur_vol;
    unsigned int ch2_len_cnt;
    signed   int ch2_env_cnt;
    signed   int ch2_sample;
    unsigned int ch2_frequency;
    unsigned int ch2_trigger;
    unsigned int ch2_enable;

    unsigned int ch3_len_clk;
    unsigned int ch3_frq_clk;
    unsigned int ch3_len_cnt;
    unsigned int ch3_wav_pos;
    signed       ch3_sample;
    unsigned int ch3_frequency;
    unsigned int ch3_trigger;
    unsigned int ch3_enable;

    unsigned int ch4_len_clk;
    unsigned int ch4_env_clk;
    unsigned int ch4_frq_clk;
    unsigned int ch4_cur_vol;
    unsigned int ch4_len_cnt;
    signed   int ch4_env_cnt;
    signed       ch4_sample;
    unsigned int ch4_frequency;
    unsigned int ch4_trigger;
    unsigned int ch4_enable;

    unsigned short lfsr;
};

struct APU apu;

void apu_reset()
{
    apu.clk = 0;

    apu.ch1_len_clk = 0;
    apu.ch1_env_clk = 0;
    apu.ch1_frq_clk = 0;
    apu.ch1_cur_vol = 0;
    apu.ch1_len_cnt = 64;
    apu.ch1_env_cnt = 7;
    apu.ch1_swp_cnt = 0;
    apu.ch1_shadow_freq = 0;
    apu.ch1_swp_clk = 0;

    apu.ch2_len_clk = 0;
    apu.ch2_env_clk = 0;
    apu.ch2_frq_clk = 0;
    apu.ch2_cur_vol = 0;
    apu.ch2_len_cnt = 64;
    apu.ch2_env_cnt = 7;

    apu.ch3_len_clk = 0;
    apu.ch3_frq_clk = 0;
    apu.ch3_len_cnt = 256;
    apu.ch3_wav_pos = 0;

    apu.ch4_len_clk = 0;
    apu.ch4_env_clk = 0;
    apu.ch4_frq_clk = 0;
    apu.ch4_cur_vol = 0;
    apu.ch4_len_cnt = 64;
    apu.ch4_env_cnt = 7;
    apu.ch4_sample = 0;

    apu.lfsr = 0x7FFF;
}

static const float  duty_rates[] = { 0.125, 0.25, 0.5, 0.75 };
static const int    div_rates [] = { 8, 16, 32, 48, 64, 80, 96, 112};

#define APU_ENABLE_BIT     0x80
#define CH1_SND_ENABLE_BIT 0x01
#define CH2_SND_ENABLE_BIT 0x02
#define CH3_SND_ENABLE_BIT 0x04
#define CH4_SND_ENABLE_BIT 0x08
#define CH4_SND_STEP_BIT   0x08
#define CH3_APU_ENABLE_BIT 0x80
#define CH_INIT_BIT        0x80
#define CH_ENV_INC_BIT     0x08
#define CH_SWP_INC_BIT     0x08
#define CH_LEN_CONT_BIT    0x40
#define CH_LEN_256HZ       16384
#define CH_SWP_128HZ       32768
#define CH_ENV_64HZ        65536

INLINE static unsigned short calc_sweep_freq(void)
{
    unsigned short new_freq;
    int sweep_freq = apu.ch1_shadow_freq >> apu.ch1_swp_shift;
    new_freq = apu.ch1_shadow_freq + (mmu.nr10 & CH_SWP_INC_BIT ? -sweep_freq : sweep_freq);
    if(new_freq > 2047)
        CLEAR_BIT(mmu.nr52, CH1_SND_ENABLE_BIT);
    return new_freq;
}

INLINE static void ch1_step(void)
{

    int ch1_freq;
     //trigger
    if(apu.ch1_trigger)
    {
        CLEAR_BIT(mmu.nr14, CH_INIT_BIT);
        apu.ch1_trigger = 0;
        SET_BIT(mmu.nr52, CH1_SND_ENABLE_BIT);
        apu.ch1_enable = 1;
        apu.ch1_cur_vol = (mmu.nr12 & 0xF0) >> 4;
        //if(apu.ch1_len_cnt == 0)
            apu.ch1_len_cnt = 64 - (mmu.nr11 & 0x3F);
        apu.ch1_env_cnt = mmu.nr12 & 0x07;
        apu.ch1_len_clk = clock.t_clock;
        apu.ch1_env_clk = clock.t_clock;
        apu.ch1_frq_clk = clock.t_clock;
        apu.ch1_swp_clk = clock.t_clock;
        apu.ch1_shadow_freq = apu.ch1_frequency;
        if(apu.ch1_swp_shift > 0)
            calc_sweep_freq();
        apu.ch1_swp_cnt = 0;
    }
    else
    {
        apu.ch1_len_clk += clock.t_clock;
        apu.ch1_env_clk += clock.t_clock;
        apu.ch1_frq_clk += clock.t_clock;
        apu.ch1_swp_clk += clock.t_clock;
    }

    if(!apu.ch1_enable)
    {
        //(mmu.nr52 & CH1_SND_ENABLE_BIT)
        apu.ch1_sample = 0;
        return;
    }

    //int step_clk = (mmu.nr12 & 0x7) * CH_ENV_64HZ;
    if(apu.ch1_env_clk >= CH_ENV_64HZ)
    {
        apu.ch1_env_clk -= CH_ENV_64HZ;

        if((mmu.nr12 & 0x07) > 0)
        {
            if(apu.ch1_env_cnt > 0)
                apu.ch1_env_cnt--;

            if(apu.ch1_env_cnt == 0)
            {
                if(mmu.nr12 & CH_ENV_INC_BIT)
                {
                    if(apu.ch1_cur_vol < 16)
                        apu.ch1_cur_vol++;
                }
                else
                {
                    if(apu.ch1_cur_vol > 0)
                        apu.ch1_cur_vol--;
                }
                apu.ch1_env_cnt = mmu.nr12 & 0x07;
            }
        }
    }

    if(apu.ch1_len_clk >= CH_LEN_256HZ)
    {
       apu.ch1_len_clk -= CH_LEN_256HZ;
       if(apu.ch1_len_cnt > 0)
       {
           apu.ch1_len_cnt--;
       }
       else
       {
           //apu.ch1_len_cnt = mmu.nr11 & 0x3F;
           if((mmu.nr14 & CH_LEN_CONT_BIT))
           {
                CLEAR_BIT(mmu.nr52, CH1_SND_ENABLE_BIT);
                apu.ch1_enable = 0;
           }
       }
    }

    if(apu.ch1_swp_clk >= CH_SWP_128HZ)
    {
        apu.ch1_swp_clk -= CH_SWP_128HZ;

        if((mmu.nr10 >> 4) > 0  && apu.ch1_swp_shift > 0)
        {
            if(apu.ch1_swp_cnt > 0)
                apu.ch1_swp_cnt--;

            if(apu.ch1_swp_cnt == 0 )
            {
                int new_freq = calc_sweep_freq();
                if(new_freq < 2048)
                {
                    apu.ch1_shadow_freq = new_freq;
                    mmu.nr13 = new_freq & 0xFF;
                    mmu.nr14 = (mmu.nr14 & 0xF8) | ((new_freq >> 8) & 0x7);
                    calc_sweep_freq();
                }

                apu.ch1_swp_cnt = 16-(mmu.nr10 >> 4);
            }
        }
    }

    if(apu.ch1_swp_shift)
        ch1_freq = apu.ch1_shadow_freq;
    else
        ch1_freq = apu.ch1_frequency;

    ch1_freq = ((2048 - ch1_freq) << 5);

    if(apu.ch1_frq_clk >= (duty_rates[mmu.nr11 >> 6] * ch1_freq))
        apu.ch1_sample = -apu.ch1_cur_vol >> 1;
    else
        apu.ch1_sample = apu.ch1_cur_vol >> 1;

    if(apu.ch1_frq_clk >= ch1_freq)
        apu.ch1_frq_clk -= ch1_freq;
}

INLINE static void ch2_step(void)
{
     //trigger
    if(apu.ch2_trigger)
    {
        CLEAR_BIT(mmu.nr24, CH_INIT_BIT);
        apu.ch2_trigger = 0;
        SET_BIT(mmu.nr52, CH2_SND_ENABLE_BIT);
        apu.ch2_enable = 1;
        apu.ch2_cur_vol = (mmu.nr22 & 0xF0) >> 4;
        //if(apu.ch2_len_cnt == 0)
            apu.ch2_len_cnt = 64 - (mmu.nr21 & 0x3F);
        apu.ch2_env_cnt = 0;
        apu.ch2_len_clk = clock.t_clock;
        apu.ch2_env_clk = clock.t_clock;
        apu.ch2_frq_clk = clock.t_clock;
    }
    else
    {
        apu.ch2_len_clk += clock.t_clock;
        apu.ch2_env_clk += clock.t_clock;
        apu.ch2_frq_clk += clock.t_clock;
    }

    if(!apu.ch2_enable)
    {
        apu.ch2_sample = 0;
        return;
    }

    //int step_clk = (mmu.nr22 & 0x7) * CH_ENV_64HZ;
    if(apu.ch2_env_clk >= CH_ENV_64HZ)
    {
        apu.ch2_env_clk -= CH_ENV_64HZ;
        if((mmu.nr22 & 0x07) > 0)
        {
            if(apu.ch2_env_cnt > 0)
                apu.ch2_env_cnt--;

            if(apu.ch2_env_cnt == 0)
            {
                if(mmu.nr22 & CH_ENV_INC_BIT)
                {
                    if(apu.ch2_cur_vol < 16)
                        apu.ch2_cur_vol++;
                }
                else
                {
                    if(apu.ch2_cur_vol > 0)
                        apu.ch2_cur_vol--;
                }
                apu.ch2_env_cnt = mmu.nr22 & 0x07;
            }
        }
    }

    //int len_clk = (64-(mmu.nr21 & 0x3F)) * CH_LEN_256HZ;
    if(apu.ch2_len_clk >= CH_LEN_256HZ)
   {
       apu.ch2_len_clk -= CH_LEN_256HZ;
       if(apu.ch2_len_cnt > 0)
       {
           apu.ch2_len_cnt--;
       }
       else
       {
            //apu.ch2_len_cnt = mmu.nr11 & 0x3F;
           if((mmu.nr24 & CH_LEN_CONT_BIT))
           {
               CLEAR_BIT(mmu.nr52, CH2_SND_ENABLE_BIT);
               apu.ch2_enable = 0;
           }

       }
    }

    if(apu.ch2_frq_clk >= (duty_rates[mmu.nr21 >> 6] * apu.ch2_frequency))
        apu.ch2_sample = -apu.ch2_cur_vol >> 1;
    else
        apu.ch2_sample = apu.ch2_cur_vol >> 1;

    if(apu.ch2_frq_clk >= apu.ch2_frequency)
        apu.ch2_frq_clk -= apu.ch2_frequency;
}

INLINE static void ch3_step(void)
{

    if(!(mmu.nr30 & CH3_APU_ENABLE_BIT))
    {
        apu.ch3_sample = 0;;
        return;
    }

     //trigger
    if(apu.ch3_trigger)
    {
        CLEAR_BIT(mmu.nr34, CH_INIT_BIT);
        SET_BIT(mmu.nr52, CH3_SND_ENABLE_BIT);
        apu.ch3_wav_pos = 0;
        apu.ch3_trigger = 0;
        apu.ch3_enable = 1;
        apu.ch3_frq_clk = 0;
        apu.ch3_len_clk = 0;
        //if(apu.ch3_len_cnt == 0)
            apu.ch3_len_cnt = 256 - mmu.nr31;
    }
    else
    {
        apu.ch3_len_clk += clock.t_clock;
        apu.ch3_frq_clk += clock.t_clock;
    }

    if(!apu.ch3_enable)
    {
        apu.ch3_sample = 0;
        return;
    }


    if(apu.ch3_len_clk >= CH_LEN_256HZ)
   {
       apu.ch3_len_clk -= CH_LEN_256HZ;
       if(apu.ch3_len_cnt > 0)
       {
           apu.ch3_len_cnt--;
       }
       else
       {
           if((mmu.nr34 & CH_LEN_CONT_BIT))
           {
                CLEAR_BIT(mmu.nr52, CH3_SND_ENABLE_BIT);
                apu.ch3_enable = 0;
           }
       }
    }


    if(apu.ch3_frq_clk >= apu.ch3_frequency)
    {
        apu.ch3_frq_clk -= apu.ch3_frequency;

        int s = apu.ch3_wav_pos & 0x01 ? mmu.waveram[apu.ch3_wav_pos >> 1] & 0x0F : mmu.waveram[apu.ch3_wav_pos >> 1] >> 4;
        int out_lvl = (mmu.nr32 >> 5) & 0x3;
        /*
        if(out_lvl > 0)
            s = s >> (out_lvl - 1);
        else
            s = 8;
            */
        if(out_lvl > 0)
            s = s >> (out_lvl - 1);
        else
            s = 0;

        //apu.ch3_sample = (float)(s - 8) / (float)8.0;
        apu.ch3_sample = s >> 1;
        apu.ch3_wav_pos++;
        if(apu.ch3_wav_pos > 31)
            apu.ch3_wav_pos = 0;
    }
}

INLINE static void ch4_step(void)
{
     //trigger
    if(apu.ch4_trigger)
    {
        CLEAR_BIT(mmu.nr44, CH_INIT_BIT);
        apu.ch4_trigger = 0;
        apu.ch4_enable = 1;
        SET_BIT(mmu.nr52, CH4_SND_ENABLE_BIT);
        apu.ch4_cur_vol = (mmu.nr42 & 0xF0) >> 4;
        //if(apu.ch4_len_cnt == 0)
            apu.ch4_len_cnt = 64 - (mmu.nr41 & 0x3f);
        apu.ch4_env_cnt = mmu.nr42 & 0x07;;
        apu.ch4_len_clk = clock.t_clock;
        apu.ch4_env_clk = clock.t_clock;
        apu.ch4_frq_clk = clock.t_clock;
        //apu.lfsr = 0x7FFF;
    }
    else
    {
        apu.ch4_len_clk += clock.t_clock;
        apu.ch4_env_clk += clock.t_clock;
        apu.ch4_frq_clk += clock.t_clock;
    }

    if(!apu.ch4_enable)
    {
        apu.ch4_sample = 0;
        return;
    }

    //int step_clk = (mmu.nr22 & 0x7) * CH_ENV_64HZ;
    if(apu.ch4_env_clk >= CH_ENV_64HZ)
    {
        apu.ch4_env_clk -= CH_ENV_64HZ;
        if((mmu.nr42 & 0x07) > 0)
        {
            if(apu.ch4_env_cnt > 0)
                apu.ch4_env_cnt--;

            if(apu.ch4_env_cnt == 0)
            {
                if(mmu.nr42 & CH_ENV_INC_BIT)
                {
                    if(apu.ch4_cur_vol < 16)
                        apu.ch4_cur_vol++;
                }
                else
                {
                    if(apu.ch4_cur_vol > 0)
                        apu.ch4_cur_vol--;
                }
                apu.ch4_env_cnt = mmu.nr42 & 0x07;
            }
        }
    }

    if(apu.ch4_len_clk >= CH_LEN_256HZ)
   {
       apu.ch4_len_clk -= CH_LEN_256HZ;
       if(apu.ch4_len_cnt > 0)
       {
           apu.ch4_len_cnt--;
       }
       else
       {
            if((mmu.nr44 & CH_LEN_CONT_BIT))
            {
                CLEAR_BIT(mmu.nr52, CH4_SND_ENABLE_BIT);
                apu.ch4_enable = 0;
            }
       }
    }



    if(apu.ch4_frq_clk >= apu.ch4_frequency)
    {
        apu.ch4_frq_clk -= apu.ch4_frequency;

        int res = ((apu.lfsr >> 1) ^ apu.lfsr) & 0x01;
        apu.lfsr >>= 1;
        if(mmu.nr43 & CH4_SND_STEP_BIT)
        {
            CLEAR_BIT(apu.lfsr, 0x40);
            SET_BIT(apu.lfsr, res << 6);
        }
        else
        {
            SET_BIT(apu.lfsr, res << 14);
        }

        if(apu.lfsr & 0x01)
            apu.ch4_sample = -apu.ch4_cur_vol >> 1;
        else
            apu.ch4_sample = apu.ch4_cur_vol >> 1;
    }
}

INLINE void apu_wb(unsigned short addr, unsigned char byte)
{
    switch(addr)
    {

        case 0xFF10:
            apu.ch1_swp_shift = mmu.nr10 & 0x07;
            break;

        case 0xFF13:
            apu.ch1_frequency = ((mmu.nr14 & 0x07) << 8) | mmu.nr13;
            break;

        case 0xFF14:
            apu.ch1_frequency = ((mmu.nr14 & 0x07) << 8) | mmu.nr13;
            apu.ch1_trigger = mmu.nr14 & CH_INIT_BIT;
            break;



         case 0xFF18:
            apu.ch2_frequency = ((2048 - (((mmu.nr24 & 0x07) << 8) | mmu.nr23)) << 5);
            break;

        case 0xFF19:
            apu.ch2_frequency = ((2048 - (((mmu.nr24 & 0x07) << 8) | mmu.nr23)) << 5);
            apu.ch2_trigger = mmu.nr24 & CH_INIT_BIT;
            break;


        case 0xFF1D:
            apu.ch3_frequency = ((2048 - (((mmu.nr34 & 0x07) << 8) | mmu.nr33)) << 1);
            break;

        case 0xFF1E:
            apu.ch3_frequency = ((2048 - (((mmu.nr34 & 0x07) << 8) | mmu.nr33)) << 1);
            apu.ch3_trigger = mmu.nr34 & CH_INIT_BIT;
            break;

        case 0xFF22:
            apu.ch4_frequency = (div_rates[mmu.nr43 & 0x07] << (mmu.nr43 >> 4));
            apu.ch4_trigger = mmu.nr44 & CH_INIT_BIT;
            break;
    }
}

static int pos = 0;
void apu_step()
{
    if(!(mmu.nr52 & APU_ENABLE_BIT))
        return;

    int tmp = clock.t_clock;

    if(clock.double_speed)
        clock.t_clock = (clock.t_clock >> 1);

    apu.clk += clock.t_clock;

    ch1_step();
    ch2_step();
    ch3_step();
    ch4_step();

    int sample = ((apu.ch1_sample + apu.ch2_sample + apu.ch3_sample + apu.ch4_sample) * 8);
    if(sample > 127)
        sample = 127;
    else if(sample < -127)
        sample = -127;
    //44100hz
    if(apu.clk >= 95)
    {
        apu.clk -= 95;
        //audio_b[pos] = (float)sample/(float)127.0;
        audio_b[pos] = sample;
        pos++;
        if(pos >= 735)
        {
             pos = 0;
            audio_flush();
        }

    }
    clock.t_clock = tmp;
}
