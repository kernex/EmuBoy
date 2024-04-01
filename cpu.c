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
#include <stdio.h>

static CPU cpu;
static int halted = 0;

static void int_jmp(unsigned short addr)
{
    /* salva o pc na pilha */
    cpu.sp--;
    cpu.sp--;
    mmu_ww(cpu.sp, cpu.pc);
    /* pula para o handler */
    cpu.pc = addr;
    /* clocks consumido */
    clock.t_clock += 12;
}

static void nop(){}

/*
    Convencao de nomes:
    ld_BC_a -> LD(BC), A
    ld_bc_nn -> LD BC, 20
*/

static void ld_bc_nn(){ cpu.BC = mmu_rw(cpu.pc); cpu.pc+=2; }
static void ld_de_nn(){ cpu.DE = mmu_rw(cpu.pc); cpu.pc+=2; }
static void ld_hl_nn(){ cpu.HL = mmu_rw(cpu.pc); cpu.pc+=2; }
static void ld_sp_nn(){ cpu.sp = mmu_rw(cpu.pc); cpu.pc+=2; }
static void ld_sp_hl(){ cpu.sp = cpu.HL; }

//static void ld_hl_sp_n(){ cpu.HL = cpu.sp + ((signed char)mmu_rb(cpu.pc++)); }
static void ld_hl_sp_n(){ unsigned char n = mmu_rb(cpu.pc++); unsigned char sres = (cpu.sp & 0x00FF) + (signed char)n; int ures = (cpu.sp & 0x00FF) + n; cpu.HL = (cpu.sp & 0xFF00) + sres; cpu.af.flags = HFLAG8(cpu.sp, n, ures) | CFLAG_ADD8(ures); }


static void ld_BC_a(){ mmu_wb(cpu.BC, cpu.af.A); }
static void ld_DE_a(){ mmu_wb(cpu.DE, cpu.af.A); }

static void ld_HL_n(){ mmu_wb(cpu.HL, mmu_rb(cpu.pc++)); }

static void ld_HL_a(){ mmu_wb(cpu.HL, cpu.af.A); }
static void ld_HL_b(){ mmu_wb(cpu.HL, cpu.bc.B); }
static void ld_HL_c(){ mmu_wb(cpu.HL, cpu.bc.C); }
static void ld_HL_d(){ mmu_wb(cpu.HL, cpu.de.D); }
static void ld_HL_e(){ mmu_wb(cpu.HL, cpu.de.E); }
static void ld_HL_h(){ mmu_wb(cpu.HL, cpu.hl.H); }
static void ld_HL_l(){ mmu_wb(cpu.HL, cpu.hl.L); }

static void ld_HLi_a(){ mmu_wb(cpu.HL++, cpu.af.A); }
static void ld_a_HLi(){ cpu.af.A = mmu_rb(cpu.HL++); }

static void ld_HLd_a(){ mmu_wb(cpu.HL--, cpu.af.A); }
static void ld_a_HLd(){ cpu.af.A = mmu_rb(cpu.HL--); }

static void ld_NN_sp(){ mmu_ww(mmu_rw(cpu.pc), cpu.sp); cpu.pc+=2; }
static void ldh_N_a() { mmu_wb(0xFF00 + mmu_rb(cpu.pc++), cpu.af.A); }
static void ldh_a_N() { cpu.af.A = mmu_rb(0xFF00 + mmu_rb(cpu.pc++)); }
static void ld_NN_a() { mmu_wb(mmu_rw(cpu.pc), cpu.af.A); cpu.pc+=2; }
static void ld_a_NN() { cpu.af.A = mmu_rw(mmu_rw(cpu.pc)); cpu.pc+=2; }
static void ld_C_a() { mmu_wb(0xFF00 + cpu.bc.C, cpu.af.A); }
static void ld_a_C() { cpu.af.A = mmu_rb(0xFF00 + cpu.bc.C); }

static void ld_a_BC(){ cpu.af.A = mmu_rb(cpu.BC); }
static void ld_a_DE(){ cpu.af.A = mmu_rb(cpu.DE); }

static void ld_a_n(){ cpu.af.A = mmu_rb(cpu.pc++); }
static void ld_b_n(){ cpu.bc.B = mmu_rb(cpu.pc++); }
static void ld_c_n(){ cpu.bc.C = mmu_rb(cpu.pc++); }
static void ld_d_n(){ cpu.de.D = mmu_rb(cpu.pc++); }
static void ld_e_n(){ cpu.de.E = mmu_rb(cpu.pc++); }
static void ld_h_n(){ cpu.hl.H = mmu_rb(cpu.pc++); }
static void ld_l_n(){ cpu.hl.L = mmu_rb(cpu.pc++); }

static void ld_b_b(){ }
static void ld_b_c(){ cpu.bc.B = cpu.bc.C; }
static void ld_b_d(){ cpu.bc.B = cpu.de.D; }
static void ld_b_e(){ cpu.bc.B = cpu.de.E; }
static void ld_b_h(){ cpu.bc.B = cpu.hl.H; }
static void ld_b_l(){ cpu.bc.B = cpu.hl.L; }
static void ld_b_a(){ cpu.bc.B = cpu.af.A; }
static void ld_b_HL(){ cpu.bc.B = mmu_rb(cpu.HL); }

static void ld_c_c(){ }
static void ld_c_b(){ cpu.bc.C = cpu.bc.B; }
static void ld_c_d(){ cpu.bc.C = cpu.de.D; }
static void ld_c_e(){ cpu.bc.C = cpu.de.E; }
static void ld_c_h(){ cpu.bc.C = cpu.hl.H; }
static void ld_c_l(){ cpu.bc.C = cpu.hl.L; }
static void ld_c_a(){ cpu.bc.C = cpu.af.A; }
static void ld_c_HL(){ cpu.bc.C = mmu_rb(cpu.HL); }

static void ld_d_d(){ }
static void ld_d_b(){ cpu.de.D = cpu.bc.B; }
static void ld_d_c(){ cpu.de.D = cpu.bc.C; }
static void ld_d_e(){ cpu.de.D = cpu.de.E; }
static void ld_d_h(){ cpu.de.D = cpu.hl.H; }
static void ld_d_l(){ cpu.de.D = cpu.hl.L; }
static void ld_d_a(){ cpu.de.D = cpu.af.A; }
static void ld_d_HL(){ cpu.de.D = mmu_rb(cpu.HL); }



static void ld_e_b(){ cpu.de.E = cpu.bc.B; }
static void ld_e_c(){ cpu.de.E = cpu.bc.C; }
static void ld_e_d(){ cpu.de.E = cpu.de.D; }
static void ld_e_e(){ }
static void ld_e_h(){ cpu.de.E = cpu.hl.H; }
static void ld_e_l(){ cpu.de.E = cpu.hl.L; }
static void ld_e_a(){ cpu.de.E = cpu.af.A; }
static void ld_e_HL(){ cpu.de.E = mmu_rb(cpu.HL); }

static void ld_h_b(){ cpu.hl.H = cpu.bc.B; }
static void ld_h_c(){ cpu.hl.H = cpu.bc.C; }
static void ld_h_d(){ cpu.hl.H = cpu.de.D; }
static void ld_h_e(){ cpu.hl.H = cpu.de.E; }
static void ld_h_h(){ }
static void ld_h_l(){ cpu.hl.H = cpu.hl.L; }
static void ld_h_a(){ cpu.hl.H = cpu.af.A; }
static void ld_h_HL(){ cpu.hl.H = mmu_rb(cpu.HL); }

static void ld_l_b(){ cpu.hl.L = cpu.bc.B; }
static void ld_l_c(){ cpu.hl.L = cpu.bc.C; }
static void ld_l_d(){ cpu.hl.L = cpu.de.D; }
static void ld_l_e(){ cpu.hl.L = cpu.de.E; }
static void ld_l_h(){ cpu.hl.L = cpu.hl.H; }
static void ld_l_l(){ }
static void ld_l_a(){ cpu.hl.L = cpu.af.A; }
static void ld_l_HL(){ cpu.hl.L = mmu_rb(cpu.HL); }

static void ld_a_b(){ cpu.af.A = cpu.bc.B; }
static void ld_a_c(){ cpu.af.A = cpu.bc.C; }
static void ld_a_d(){ cpu.af.A = cpu.de.D; }
static void ld_a_e(){ cpu.af.A = cpu.de.E; }
static void ld_a_h(){ cpu.af.A = cpu.hl.H; }
static void ld_a_l(){ cpu.af.A = cpu.hl.L; }
static void ld_a_HL(){ cpu.af.A  = mmu_rb(cpu.HL); }
static void ld_a_a(){ }

static void inc_bc(){ cpu.BC++; }
static void inc_de(){ cpu.DE++; }
static void inc_hl(){ cpu.HL++; }
static void inc_sp(){ cpu.sp++; }

static void inc_HL() { int value = mmu_rb(cpu.HL) + 1; mmu_wb(cpu.HL, value); cpu.af.flags = ZFLAG(value) | HFLAG_INC(value) | KEEP(C_FLAG); }

static void inc_a(){ cpu.af.A++; cpu.af.flags = ZFLAG(cpu.af.A) | HFLAG_INC(cpu.af.A) | KEEP(C_FLAG); }
static void inc_b(){ cpu.bc.B++; cpu.af.flags = ZFLAG(cpu.bc.B) | HFLAG_INC(cpu.bc.B) | KEEP(C_FLAG); }
static void inc_c(){ cpu.bc.C++; cpu.af.flags = ZFLAG(cpu.bc.C) | HFLAG_INC(cpu.bc.C) | KEEP(C_FLAG); }
static void inc_d(){ cpu.de.D++; cpu.af.flags = ZFLAG(cpu.de.D) | HFLAG_INC(cpu.de.D) | KEEP(C_FLAG); }
static void inc_e(){ cpu.de.E++; cpu.af.flags = ZFLAG(cpu.de.E) | HFLAG_INC(cpu.de.E) | KEEP(C_FLAG); }
static void inc_h(){ cpu.hl.H++; cpu.af.flags = ZFLAG(cpu.hl.H) | HFLAG_INC(cpu.hl.H) | KEEP(C_FLAG); }
static void inc_l(){ cpu.hl.L++; cpu.af.flags = ZFLAG(cpu.hl.L) | HFLAG_INC(cpu.hl.L) | KEEP(C_FLAG); }

static void dec_a(){ cpu.af.A--; cpu.af.flags = ZFLAG(cpu.af.A) | N_FLAG | HFLAG_DEC(cpu.af.A) | KEEP(C_FLAG); }
static void dec_b(){ cpu.bc.B--; cpu.af.flags = ZFLAG(cpu.bc.B) | N_FLAG | HFLAG_DEC(cpu.bc.B) | KEEP(C_FLAG); }
static void dec_c(){ cpu.bc.C--; cpu.af.flags = ZFLAG(cpu.bc.C) | N_FLAG | HFLAG_DEC(cpu.bc.C) | KEEP(C_FLAG); }
static void dec_d(){ cpu.de.D--; cpu.af.flags = ZFLAG(cpu.de.D) | N_FLAG | HFLAG_DEC(cpu.de.D) | KEEP(C_FLAG); }
static void dec_e(){ cpu.de.E--; cpu.af.flags = ZFLAG(cpu.de.E) | N_FLAG | HFLAG_DEC(cpu.de.E) | KEEP(C_FLAG); }
static void dec_h(){ cpu.hl.H--; cpu.af.flags = ZFLAG(cpu.hl.H) | N_FLAG | HFLAG_DEC(cpu.hl.H) | KEEP(C_FLAG); }
static void dec_l(){ cpu.hl.L--; cpu.af.flags = ZFLAG(cpu.hl.L) | N_FLAG | HFLAG_DEC(cpu.hl.L) | KEEP(C_FLAG); }

static void dec_bc(){ cpu.BC--; }
static void dec_de(){ cpu.DE--; }
static void dec_hl(){ cpu.HL--; }
static void dec_sp(){ cpu.sp--; }

static void dec_HL() { int value = mmu_rb(cpu.HL) - 1; mmu_wb(cpu.HL, value); cpu.af.flags = ZFLAG(value) | N_FLAG | HFLAG_DEC(value) | KEEP(C_FLAG); }

static void rlca(){ cpu.af.A = (cpu.af.A << 1) | (cpu.af.A >> 7); cpu.af.flags = (cpu.af.A & 0x01) ? C_FLAG : 0; }
static void rrca(){ cpu.af.A = (cpu.af.A >> 1) | (cpu.af.A << 7); cpu.af.flags = (cpu.af.A & 0x80) ? C_FLAG : 0; }

static void rla(){ int carry = cpu.af.flags & C_FLAG ? 0x01 : 0x00; cpu.af.flags = (cpu.af.A & 0x80) ? C_FLAG : 0x00; cpu.af.A = (cpu.af.A << 1) | carry; }
static void rra(){ int carry = cpu.af.flags & C_FLAG ? 0x80 : 0x00; cpu.af.flags = (cpu.af.A & 0x01) ? C_FLAG : 0x00; cpu.af.A = carry | (cpu.af.A >> 1); }



static void add_hl_bc(){ int res = cpu.HL + cpu.BC; cpu.af.flags = KEEP(Z_FLAG) | HFLAG_ADD16(cpu.HL, cpu.BC) | CFLAG_ADD16(res); cpu.HL = res & 0xFFFF; }
static void add_hl_de(){ int res = cpu.HL + cpu.DE; cpu.af.flags = KEEP(Z_FLAG) | HFLAG_ADD16(cpu.HL, cpu.DE) | CFLAG_ADD16(res); cpu.HL = res & 0xFFFF; }
static void add_hl_hl(){ int res = cpu.HL + cpu.HL; cpu.af.flags = KEEP(Z_FLAG) | HFLAG_ADD16(cpu.HL, cpu.HL) | CFLAG_ADD16(res); cpu.HL = res & 0xFFFF; }
static void add_hl_sp(){ int res = cpu.HL + cpu.sp; cpu.af.flags = KEEP(Z_FLAG) | HFLAG_ADD16(cpu.HL, cpu.sp) | CFLAG_ADD16(res); cpu.HL = res & 0xFFFF; }
static void add_sp_n(){ int n = mmu_rb(cpu.pc++); int lres = (cpu.sp & 0x00FF) + n; int res = (cpu.sp & 0xFF00) + (lres & 0xFF); cpu.af.flags = HFLAG8(cpu.sp, n, lres) | CFLAG_ADD8(lres); cpu.sp = res & 0xFFFF; }

static void add_a_b(){ int res = cpu.af.A + cpu.bc.B; cpu.af.flags = ZFLAG(res) | HFLAG8(res, cpu.af.A, cpu.bc.B) | CFLAG_ADD8(res); cpu.af.A = res & 0xFF; }
static void add_a_c(){ int res = cpu.af.A + cpu.bc.C; cpu.af.flags = ZFLAG(res) | HFLAG8(res, cpu.af.A, cpu.bc.C) | CFLAG_ADD8(res); cpu.af.A = res & 0xFF; }
static void add_a_d(){ int res = cpu.af.A + cpu.de.D; cpu.af.flags = ZFLAG(res) | HFLAG8(res, cpu.af.A, cpu.de.D) | CFLAG_ADD8(res); cpu.af.A = res & 0xFF; }
static void add_a_e(){ int res = cpu.af.A + cpu.de.E; cpu.af.flags = ZFLAG(res) | HFLAG8(res, cpu.af.A, cpu.de.E) | CFLAG_ADD8(res); cpu.af.A = res & 0xFF; }
static void add_a_h(){ int res = cpu.af.A + cpu.hl.H; cpu.af.flags = ZFLAG(res) | HFLAG8(res, cpu.af.A, cpu.hl.H) | CFLAG_ADD8(res); cpu.af.A = res & 0xFF; }
static void add_a_l(){ int res = cpu.af.A + cpu.hl.L; cpu.af.flags = ZFLAG(res) | HFLAG8(res, cpu.af.A, cpu.hl.L) | CFLAG_ADD8(res); cpu.af.A = res & 0xFF; }
static void add_a_HL(){ int tmp = mmu_rb(cpu.HL); int res = cpu.af.A + tmp; cpu.af.flags = ZFLAG(res) | HFLAG8(res, cpu.af.A, tmp) | CFLAG_ADD8(res); cpu.af.A = res & 0xFF; }
static void add_a_a(){ int res = cpu.af.A + cpu.af.A; cpu.af.flags = ZFLAG(res) | HFLAG8(res, cpu.af.A, cpu.af.A) | CFLAG_ADD8(res); cpu.af.A = res & 0xFF; }
static void add_a_n(){ int n = mmu_rb(cpu.pc++); int res = cpu.af.A + n; cpu.af.flags = ZFLAG(res) | HFLAG8(res, cpu.af.A, n) | CFLAG_ADD8(res); cpu.af.A = res & 0xFF; }

static void adc_a_b(){ int c = cpu.af.flags & C_FLAG ? 1 : 0; int res = cpu.af.A + cpu.bc.B + c; cpu.af.flags = ZFLAG(res) | HFLAG8(res, cpu.af.A, cpu.bc.B) | CFLAG_ADD8(res); cpu.af.A = res & 0xFF; }
static void adc_a_c(){ int c = cpu.af.flags & C_FLAG ? 1 : 0; int res = cpu.af.A + cpu.bc.C + c; cpu.af.flags = ZFLAG(res) | HFLAG8(res, cpu.af.A, cpu.bc.C) | CFLAG_ADD8(res); cpu.af.A = res & 0xFF; }
static void adc_a_d(){ int c = cpu.af.flags & C_FLAG ? 1 : 0; int res = cpu.af.A + cpu.de.D + c; cpu.af.flags = ZFLAG(res) | HFLAG8(res, cpu.af.A, cpu.de.D) | CFLAG_ADD8(res); cpu.af.A = res & 0xFF; }
static void adc_a_e(){ int c = cpu.af.flags & C_FLAG ? 1 : 0; int res = cpu.af.A + cpu.de.E + c; cpu.af.flags = ZFLAG(res) | HFLAG8(res, cpu.af.A, cpu.de.E) | CFLAG_ADD8(res); cpu.af.A = res & 0xFF; }
static void adc_a_h(){ int c = cpu.af.flags & C_FLAG ? 1 : 0; int res = cpu.af.A + cpu.hl.H + c; cpu.af.flags = ZFLAG(res) | HFLAG8(res, cpu.af.A, cpu.hl.H) | CFLAG_ADD8(res); cpu.af.A = res & 0xFF; }
static void adc_a_l(){ int c = cpu.af.flags & C_FLAG ? 1 : 0; int res = cpu.af.A + cpu.hl.L + c; cpu.af.flags = ZFLAG(res) | HFLAG8(res, cpu.af.A, cpu.hl.L) | CFLAG_ADD8(res); cpu.af.A = res & 0xFF; }
static void adc_a_HL(){ int tmp = mmu_rb(cpu.HL) ; int res = cpu.af.A + tmp + (cpu.af.flags & C_FLAG ? 1 : 0); cpu.af.flags = ZFLAG(res) | HFLAG8(res, cpu.af.A, tmp) | CFLAG_ADD8(res); cpu.af.A = res & 0xFF; }
static void adc_a_a(){ int c = cpu.af.flags & C_FLAG ? 1 : 0; int res = cpu.af.A + cpu.af.A + c; cpu.af.flags = ZFLAG(res) | HFLAG8(res, cpu.af.A, cpu.af.A) | CFLAG_ADD8(res); cpu.af.A = res & 0xFF; }
static void adc_a_n(){ int c = mmu_rb(cpu.pc++); int res = cpu.af.A + c + (cpu.af.flags & C_FLAG ? 1 : 0); cpu.af.flags = ZFLAG(res) | HFLAG8(res, cpu.af.A, c) | CFLAG_ADD8(res); cpu.af.A = res & 0xFF; }

static void sub_b(){ int res = cpu.af.A - cpu.bc.B; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, cpu.bc.B) | CFLAG_SUB8(res); cpu.af.A = res & 0xFF; }
static void sub_c(){ int res = cpu.af.A - cpu.bc.C; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, cpu.bc.C) | CFLAG_SUB8(res); cpu.af.A = res & 0xFF; }
static void sub_d(){ int res = cpu.af.A - cpu.de.D; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, cpu.de.D) | CFLAG_SUB8(res); cpu.af.A = res & 0xFF; }
static void sub_e(){ int res = cpu.af.A - cpu.de.E; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, cpu.de.E) | CFLAG_SUB8(res); cpu.af.A = res & 0xFF; }
static void sub_h(){ int res = cpu.af.A - cpu.hl.H; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, cpu.hl.H) | CFLAG_SUB8(res); cpu.af.A = res & 0xFF; }
static void sub_l(){ int res = cpu.af.A - cpu.hl.L; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, cpu.hl.L) | CFLAG_SUB8(res); cpu.af.A = res & 0xFF; }
static void sub_HL(){ int tmp = mmu_rb(cpu.HL); int res = cpu.af.A - tmp; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, tmp) | CFLAG_SUB8(res); cpu.af.A = res & 0xFF; }
static void sub_a(){ int res = cpu.af.A - cpu.af.A; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, cpu.af.A) | CFLAG_SUB8(res); cpu.af.A = res & 0xFF; }
static void sub_n(){ int n = mmu_rb(cpu.pc++); int res = cpu.af.A - n; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, n) | CFLAG_SUB8(res); cpu.af.A = res & 0xFF; }

static void sbc_a_b(){ int c = cpu.af.flags & C_FLAG ? 1 : 0; int res = cpu.af.A - cpu.bc.B - c; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, cpu.bc.B) | CFLAG_SUB8(res); cpu.af.A = res & 0xFF; }
static void sbc_a_c(){ int c = cpu.af.flags & C_FLAG ? 1 : 0; int res = cpu.af.A - cpu.bc.C - c; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, cpu.bc.C) | CFLAG_SUB8(res); cpu.af.A = res & 0xFF; }
static void sbc_a_d(){ int c = cpu.af.flags & C_FLAG ? 1 : 0; int res = cpu.af.A - cpu.de.D - c; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, cpu.de.D) | CFLAG_SUB8(res); cpu.af.A = res & 0xFF; }
static void sbc_a_e(){ int c = cpu.af.flags & C_FLAG ? 1 : 0; int res = cpu.af.A - cpu.de.E - c; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, cpu.de.E) | CFLAG_SUB8(res); cpu.af.A = res & 0xFF; }
static void sbc_a_h(){ int c = cpu.af.flags & C_FLAG ? 1 : 0; int res = cpu.af.A - cpu.hl.H - c; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, cpu.hl.H) | CFLAG_SUB8(res); cpu.af.A = res & 0xFF; }
static void sbc_a_l(){ int c = cpu.af.flags & C_FLAG ? 1 : 0; int res = cpu.af.A - cpu.hl.L - c; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, cpu.hl.L) | CFLAG_SUB8(res); cpu.af.A = res & 0xFF; }
static void sbc_a_HL(){ int tmp = mmu_rb(cpu.HL); int res = cpu.af.A - tmp - (cpu.af.flags & C_FLAG ? 1 : 0); cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, tmp) | CFLAG_SUB8(res); cpu.af.A = res & 0xFF; }
static void sbc_a_a(){ int c = cpu.af.flags & C_FLAG ? 1 : 0; int res = cpu.af.A - cpu.af.A - c; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, cpu.af.A) | CFLAG_SUB8(res); cpu.af.A = res & 0xFF; }
static void sbc_a_n(){ int n = mmu_rb(cpu.pc++); int res = cpu.af.A - n - (cpu.af.flags & C_FLAG ? 1 : 0); cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, n) | CFLAG_SUB8(res); cpu.af.A = res & 0xFF; }

static void and_b(){ cpu.af.A &= cpu.bc.B; cpu.af.flags = ZFLAG(cpu.af.A) | H_FLAG; }
static void and_c(){ cpu.af.A &= cpu.bc.C; cpu.af.flags = ZFLAG(cpu.af.A) | H_FLAG; }
static void and_d(){ cpu.af.A &= cpu.de.D; cpu.af.flags = ZFLAG(cpu.af.A) | H_FLAG; }
static void and_e(){ cpu.af.A &= cpu.de.E; cpu.af.flags = ZFLAG(cpu.af.A) | H_FLAG; }
static void and_h(){ cpu.af.A &= cpu.hl.H; cpu.af.flags = ZFLAG(cpu.af.A) | H_FLAG; }
static void and_l(){ cpu.af.A &= cpu.hl.L; cpu.af.flags = ZFLAG(cpu.af.A) | H_FLAG; }
static void and_HL(){ int tmp = mmu_rb(cpu.HL); cpu.af.A &= tmp; cpu.af.flags = ZFLAG(cpu.af.A) | H_FLAG; }
static void and_a(){ cpu.af.A &= cpu.af.A; cpu.af.flags = ZFLAG(cpu.af.A) | H_FLAG; }
static void and_n(){ cpu.af.A &= mmu_rb(cpu.pc++); cpu.af.flags = ZFLAG(cpu.af.A) | H_FLAG; }

static void xor_b(){ cpu.af.A ^= cpu.bc.B; cpu.af.flags = ZFLAG(cpu.af.A); }
static void xor_c(){ cpu.af.A ^= cpu.bc.C; cpu.af.flags = ZFLAG(cpu.af.A); }
static void xor_d(){ cpu.af.A ^= cpu.de.D; cpu.af.flags = ZFLAG(cpu.af.A); }
static void xor_e(){ cpu.af.A ^= cpu.de.E; cpu.af.flags = ZFLAG(cpu.af.A); }
static void xor_h(){ cpu.af.A ^= cpu.hl.H; cpu.af.flags = ZFLAG(cpu.af.A); }
static void xor_l(){ cpu.af.A ^= cpu.hl.L; cpu.af.flags = ZFLAG(cpu.af.A); }
static void xor_HL(){ int tmp = mmu_rb(cpu.HL); cpu.af.A ^= tmp; cpu.af.flags = ZFLAG(cpu.af.A); }
static void xor_a(){ cpu.af.A ^= cpu.af.A; cpu.af.flags = ZFLAG(cpu.af.A); }
static void xor_n(){ cpu.af.A ^= mmu_rb(cpu.pc++); cpu.af.flags = ZFLAG(cpu.af.A); }

static void or_b(){ cpu.af.A |= cpu.bc.B; cpu.af.flags = ZFLAG(cpu.af.A); }
static void or_c(){ cpu.af.A |= cpu.bc.C; cpu.af.flags = ZFLAG(cpu.af.A); }
static void or_d(){ cpu.af.A |= cpu.de.D; cpu.af.flags = ZFLAG(cpu.af.A); }
static void or_e(){ cpu.af.A |= cpu.de.E; cpu.af.flags = ZFLAG(cpu.af.A); }
static void or_h(){ cpu.af.A |= cpu.hl.H; cpu.af.flags = ZFLAG(cpu.af.A); }
static void or_l(){ cpu.af.A |= cpu.hl.L; cpu.af.flags = ZFLAG(cpu.af.A); }
static void or_HL(){ int tmp = mmu_rb(cpu.HL); cpu.af.A |= tmp; cpu.af.flags = ZFLAG(cpu.af.A); }
static void or_a(){ cpu.af.A |= cpu.af.A; cpu.af.flags = ZFLAG(cpu.af.A); }
static void or_n(){ cpu.af.A |= mmu_rb(cpu.pc++); cpu.af.flags = ZFLAG(cpu.af.A); }

static void cp_b(){ int res = cpu.af.A - cpu.bc.B; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, cpu.bc.B) | CFLAG_SUB8(res); }
static void cp_c(){ int res = cpu.af.A - cpu.bc.C; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, cpu.bc.C) | CFLAG_SUB8(res); }
static void cp_d(){ int res = cpu.af.A - cpu.de.D; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, cpu.de.D) | CFLAG_SUB8(res); }
static void cp_e(){ int res = cpu.af.A - cpu.de.E; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, cpu.de.E) | CFLAG_SUB8(res); }
static void cp_h(){ int res = cpu.af.A - cpu.hl.H; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, cpu.hl.H) | CFLAG_SUB8(res); }
static void cp_l(){ int res = cpu.af.A - cpu.hl.L; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, cpu.hl.L) | CFLAG_SUB8(res); }
static void cp_HL(){ int tmp = mmu_rb(cpu.HL); int res = cpu.af.A - tmp; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, tmp) | CFLAG_SUB8(res); }
static void cp_a(){ int res = cpu.af.A - cpu.af.A; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, cpu.af.A) | CFLAG_SUB8(res); }
static void cp_n(){ int n = mmu_rb(cpu.pc++); int res = cpu.af.A - n; cpu.af.flags = ZFLAG(res) | N_FLAG | HFLAG8(res, cpu.af.A, n) | CFLAG_SUB8(res); }


static void stop(){ cpu.pc++; }
static void halt(){ halted = 1; }

static void rst_00h(){ int_jmp(0x0000); }
static void rst_08h(){ int_jmp(0x0008); }
static void rst_10h(){ int_jmp(0x0010); }
static void rst_18h(){ int_jmp(0x0018); }
static void rst_20h(){ int_jmp(0x0020); }
static void rst_28h(){ int_jmp(0x0028); }
static void rst_30h(){ int_jmp(0x0030); }
static void rst_38h(){ int_jmp(0x0038); }

static void jr_n(){ cpu.pc += ((signed char)mmu_rb(cpu.pc)); cpu.pc++; }
static void jr_nz_n(){ if(!(cpu.af.flags & Z_FLAG)){ cpu.pc += ((signed char)mmu_rb(cpu.pc)); clock.t_clock += 4; } cpu.pc++; }
static void jr_z_n(){ if(cpu.af.flags & Z_FLAG){ cpu.pc += ((signed char)mmu_rb(cpu.pc)); clock.t_clock += 4; } cpu.pc++; }
static void jr_c_n(){ if(cpu.af.flags & C_FLAG){ cpu.pc += ((signed char)mmu_rb(cpu.pc)); clock.t_clock += 4; } cpu.pc++; }
static void jr_nc_n(){ if(!(cpu.af.flags & C_FLAG)){ cpu.pc += ((signed char)mmu_rb(cpu.pc)); clock.t_clock += 4; } cpu.pc++; }

static void jp_nn(){ cpu.pc = mmu_rw(cpu.pc); }
static void jp_HL(){ cpu.pc = cpu.HL; }
static void jp_nz_nn(){ if(!(cpu.af.flags & Z_FLAG)){ cpu.pc = mmu_rw(cpu.pc); clock.t_clock += 4; } else { cpu.pc+=2; } }
static void jp_z_nn(){ if(cpu.af.flags & Z_FLAG){ cpu.pc = mmu_rw(cpu.pc); clock.t_clock += 4; } else { cpu.pc+=2; } }
static void jp_c_nn(){ if(cpu.af.flags & C_FLAG){ cpu.pc = mmu_rw(cpu.pc); clock.t_clock += 4; } else { cpu.pc+=2; } }
static void jp_nc_nn(){ if(!(cpu.af.flags & C_FLAG)){ cpu.pc = mmu_rw(cpu.pc); clock.t_clock += 4; } else { cpu.pc+=2; } }

static void call_nn(){ cpu.sp-=2; mmu_ww(cpu.sp, cpu.pc + 2); cpu.pc = mmu_rw(cpu.pc); }
static void call_z_nn(){ if(cpu.af.flags & Z_FLAG){ cpu.sp-=2; mmu_ww(cpu.sp, cpu.pc + 2); cpu.pc = mmu_rw(cpu.pc); clock.t_clock += 12; } else { cpu.pc+=2; } }
static void call_nz_nn(){ if(!(cpu.af.flags & Z_FLAG)){ cpu.sp-=2; mmu_ww(cpu.sp, cpu.pc + 2); cpu.pc = mmu_rw(cpu.pc); clock.t_clock += 12; } else { cpu.pc+=2; } }
static void call_c_nn(){ if(cpu.af.flags & C_FLAG){ cpu.sp-=2; mmu_ww(cpu.sp, cpu.pc + 2); cpu.pc = mmu_rw(cpu.pc); clock.t_clock += 12; } else { cpu.pc+=2; } }
static void call_nc_nn(){ if(!(cpu.af.flags & C_FLAG)){ cpu.sp-=2; mmu_ww(cpu.sp, cpu.pc + 2); cpu.pc = mmu_rw(cpu.pc); clock.t_clock += 12; } else { cpu.pc+=2; } }

static void ret(){ cpu.pc = mmu_rw(cpu.sp); cpu.sp+=2; }
static void reti(){ cpu.pc = mmu_rw(cpu.sp); cpu.sp+=2; cpu.ime = 1; }
static void ret_z(){ if(cpu.af.flags & Z_FLAG){ cpu.pc = mmu_rw(cpu.sp); cpu.sp+=2; clock.t_clock+=12; } }
static void ret_nz(){ if(!(cpu.af.flags & Z_FLAG)){ cpu.pc = mmu_rw(cpu.sp); cpu.sp+=2; clock.t_clock+=12; } }
static void ret_c(){ if(cpu.af.flags & C_FLAG){ cpu.pc = mmu_rw(cpu.sp); cpu.sp+=2; clock.t_clock+=12; } }
static void ret_nc(){ if(!(cpu.af.flags & C_FLAG)){ cpu.pc = mmu_rw(cpu.sp); cpu.sp+=2; clock.t_clock+=12; } }

static void push_bc(){ cpu.sp -= 2; mmu_ww(cpu.sp, cpu.BC); }
static void push_de(){ cpu.sp -= 2; mmu_ww(cpu.sp, cpu.DE); }
static void push_hl(){ cpu.sp -= 2; mmu_ww(cpu.sp, cpu.HL); }
static void push_af(){ cpu.sp -= 2; mmu_ww(cpu.sp, cpu.AF); }

static void pop_bc(){ cpu.BC = mmu_rw(cpu.sp); cpu.sp += 2; }
static void pop_de(){ cpu.DE = mmu_rw(cpu.sp); cpu.sp += 2; }
static void pop_hl(){ cpu.HL = mmu_rw(cpu.sp); cpu.sp += 2; }
static void pop_af(){ cpu.AF = mmu_rw(cpu.sp) & 0xFFF0; cpu.sp += 2; }

static void daa()
{
    int flag = 0x00;
    if(cpu.af.flags & N_FLAG)
    {
        if((cpu.af.A & 0x0F) > 9 || (cpu.af.flags & H_FLAG))
        {
            cpu.af.A -= 0x06;
        }
        if(cpu.af.A > 0x9F || (cpu.af.flags & C_FLAG))
        {
            cpu.af.A -= 0x60;
            flag = C_FLAG;
        }
    }
    else
    {
        if((cpu.af.A & 0x0F) > 9 || (cpu.af.flags & H_FLAG))
        {
            cpu.af.A += 0x06;
        }
        if(cpu.af.A > 0x9F || (cpu.af.flags & C_FLAG))
        {
            cpu.af.A += 0x60;
            flag = C_FLAG;
        }
    }
    cpu.af.flags = ZFLAG(cpu.af.A) | flag | KEEP(N_FLAG);

    /*
    int a = cpu.af.A;
    if((cpu.af.A & 0x0F) > 9 || (cpu.af.flags & H_FLAG))
    {
        cpu.af.A += 0x06;

    }
    CLEAR_BIT(cpu.af.flags, C_FLAG);
    if(a > 0x99 || (cpu.af.flags & H_FLAG))
    {
        cpu.af.A += 0x60;
        cpu.af.flags |= C_FLAG;
    }
*/
}

static void di(){ cpu.ime = 0; }
static void ei(){ cpu.ime = 1; }

static void cpl(){ cpu.af.A = ~cpu.af.A; cpu.af.flags = KEEP(Z_FLAG) | N_FLAG | H_FLAG | KEEP(C_FLAG); }

static void scf(){ cpu.af.flags = KEEP(Z_FLAG) | C_FLAG; }
static void ccf(){ cpu.af.flags = KEEP(Z_FLAG) | (C_FLAG & ~cpu.af.flags); }

static void rlc_a(){ cpu.af.A = (cpu.af.A << 1) | (cpu.af.A >> 7); cpu.af.flags = ZFLAG(cpu.af.A) | ((cpu.af.A & 0x01) ? C_FLAG : 0); }
static void rlc_b(){ cpu.bc.B = (cpu.bc.B << 1) | (cpu.bc.B >> 7); cpu.af.flags = ZFLAG(cpu.bc.B) | ((cpu.bc.B & 0x01) ? C_FLAG : 0); }
static void rlc_c(){ cpu.bc.C = (cpu.bc.C << 1) | (cpu.bc.C >> 7); cpu.af.flags = ZFLAG(cpu.bc.C) | ((cpu.bc.C & 0x01) ? C_FLAG : 0); }
static void rlc_d(){ cpu.de.D = (cpu.de.D << 1) | (cpu.de.D >> 7); cpu.af.flags = ZFLAG(cpu.de.D) | ((cpu.de.D & 0x01) ? C_FLAG : 0); }
static void rlc_e(){ cpu.de.E = (cpu.de.E << 1) | (cpu.de.E >> 7); cpu.af.flags = ZFLAG(cpu.de.E) | ((cpu.de.E & 0x01) ? C_FLAG : 0); }
static void rlc_l(){ cpu.hl.L = (cpu.hl.L << 1) | (cpu.hl.L >> 7); cpu.af.flags = ZFLAG(cpu.hl.L) | ((cpu.hl.L & 0x01) ? C_FLAG : 0); }
static void rlc_h(){ cpu.hl.H = (cpu.hl.H << 1) | (cpu.hl.H >> 7); cpu.af.flags = ZFLAG(cpu.hl.H) | ((cpu.hl.H & 0x01) ? C_FLAG : 0); }
static void rlc_HL(){ int value = mmu_rb(cpu.HL); value = (value << 1) | (value >> 7); cpu.af.flags = ZFLAG(value) | ((value & 0x01) ? C_FLAG : 0); mmu_wb(cpu.HL, value); }

static void rrc_a(){ cpu.af.A = (cpu.af.A >> 1) | (cpu.af.A << 7); cpu.af.flags = ZFLAG(cpu.af.A) | ((cpu.af.A & 0x80) ? C_FLAG : 0); }
static void rrc_b(){ cpu.bc.B = (cpu.bc.B >> 1) | (cpu.bc.B << 7); cpu.af.flags = ZFLAG(cpu.bc.B) | ((cpu.bc.B & 0x80) ? C_FLAG : 0); }
static void rrc_c(){ cpu.bc.C = (cpu.bc.C >> 1) | (cpu.bc.C << 7); cpu.af.flags = ZFLAG(cpu.bc.C) | ((cpu.bc.C & 0x80) ? C_FLAG : 0); }
static void rrc_d(){ cpu.de.D = (cpu.de.D >> 1) | (cpu.de.D << 7); cpu.af.flags = ZFLAG(cpu.de.D) | ((cpu.de.D & 0x80) ? C_FLAG : 0); }
static void rrc_e(){ cpu.de.E = (cpu.de.E >> 1) | (cpu.de.E << 7); cpu.af.flags = ZFLAG(cpu.de.E) | ((cpu.de.E & 0x80) ? C_FLAG : 0); }
static void rrc_h(){ cpu.hl.H = (cpu.hl.H >> 1) | (cpu.hl.H << 7); cpu.af.flags = ZFLAG(cpu.hl.H) | ((cpu.hl.H & 0x80) ? C_FLAG : 0); }
static void rrc_l(){ cpu.hl.L = (cpu.hl.L >> 1) | (cpu.hl.L << 7); cpu.af.flags = ZFLAG(cpu.hl.L) | ((cpu.hl.L & 0x80) ? C_FLAG : 0); }
static void rrc_HL(){ int value = mmu_rb(cpu.HL); value = (value >> 1) | (value << 7); cpu.af.flags = ZFLAG(value) | ((value & 0x80) ? C_FLAG : 0); mmu_wb(cpu.HL, value); }

static void rl_a(){ int carry = cpu.af.flags & C_FLAG ? 0x01 : 0x00; cpu.af.flags = (cpu.af.A & 0x80) ? C_FLAG : 0x00; cpu.af.A = (cpu.af.A << 1) | carry; cpu.af.flags |= ZFLAG(cpu.af.A); }
static void rl_b(){ int carry = cpu.af.flags & C_FLAG ? 0x01 : 0x00; cpu.af.flags = (cpu.bc.B & 0x80) ? C_FLAG : 0x00; cpu.bc.B = (cpu.bc.B << 1) | carry; cpu.af.flags |= ZFLAG(cpu.bc.B); }
static void rl_c(){ int carry = cpu.af.flags & C_FLAG ? 0x01 : 0x00; cpu.af.flags = (cpu.bc.C & 0x80) ? C_FLAG : 0x00; cpu.bc.C = (cpu.bc.C << 1) | carry; cpu.af.flags |= ZFLAG(cpu.bc.C); }
static void rl_d(){ int carry = cpu.af.flags & C_FLAG ? 0x01 : 0x00; cpu.af.flags = (cpu.de.D & 0x80) ? C_FLAG : 0x00; cpu.de.D = (cpu.de.D << 1) | carry; cpu.af.flags |= ZFLAG(cpu.de.D); }
static void rl_e(){ int carry = cpu.af.flags & C_FLAG ? 0x01 : 0x00; cpu.af.flags = (cpu.de.E & 0x80) ? C_FLAG : 0x00; cpu.de.E = (cpu.de.E << 1) | carry; cpu.af.flags |= ZFLAG(cpu.de.E); }
static void rl_h(){ int carry = cpu.af.flags & C_FLAG ? 0x01 : 0x00; cpu.af.flags = (cpu.hl.H & 0x80) ? C_FLAG : 0x00; cpu.hl.H = (cpu.hl.H << 1) | carry; cpu.af.flags |= ZFLAG(cpu.hl.H); }
static void rl_l(){ int carry = cpu.af.flags & C_FLAG ? 0x01 : 0x00; cpu.af.flags = (cpu.hl.L & 0x80) ? C_FLAG : 0x00; cpu.hl.L = (cpu.hl.L << 1) | carry; cpu.af.flags |= ZFLAG(cpu.hl.L); }
static void rl_HL(){ int value = mmu_rb(cpu.HL); int carry = cpu.af.flags & C_FLAG ? 0x01 : 0x00; cpu.af.flags = (value & 0x80) ? C_FLAG : 0x00; value = (value << 1) | carry; cpu.af.flags |= ZFLAG(value); mmu_wb(cpu.HL, value); }

static void rr_a(){ int carry = cpu.af.flags & C_FLAG ? 0x80 : 0x00; cpu.af.flags = (cpu.af.A & 0x01) ? C_FLAG : 0x00; cpu.af.A = carry | (cpu.af.A >> 1); cpu.af.flags |= ZFLAG(cpu.af.A); }
static void rr_b(){ int carry = cpu.af.flags & C_FLAG ? 0x80 : 0x00; cpu.af.flags = (cpu.bc.B & 0x01) ? C_FLAG : 0x00; cpu.bc.B = carry | (cpu.bc.B >> 1); cpu.af.flags |= ZFLAG(cpu.bc.B); }
static void rr_c(){ int carry = cpu.af.flags & C_FLAG ? 0x80 : 0x00; cpu.af.flags = (cpu.bc.C & 0x01) ? C_FLAG : 0x00; cpu.bc.C = carry | (cpu.bc.C >> 1); cpu.af.flags |= ZFLAG(cpu.bc.C); }
static void rr_d(){ int carry = cpu.af.flags & C_FLAG ? 0x80 : 0x00; cpu.af.flags = (cpu.de.D & 0x01) ? C_FLAG : 0x00; cpu.de.D = carry | (cpu.de.D >> 1); cpu.af.flags |= ZFLAG(cpu.de.D); }
static void rr_e(){ int carry = cpu.af.flags & C_FLAG ? 0x80 : 0x00; cpu.af.flags = (cpu.de.E & 0x01) ? C_FLAG : 0x00; cpu.de.E = carry | (cpu.de.E >> 1); cpu.af.flags |= ZFLAG(cpu.de.E); }
static void rr_h(){ int carry = cpu.af.flags & C_FLAG ? 0x80 : 0x00; cpu.af.flags = (cpu.hl.H & 0x01) ? C_FLAG : 0x00; cpu.hl.H = carry | (cpu.hl.H >> 1); cpu.af.flags |= ZFLAG(cpu.hl.H); }
static void rr_l(){ int carry = cpu.af.flags & C_FLAG ? 0x80 : 0x00; cpu.af.flags = (cpu.hl.L & 0x01) ? C_FLAG : 0x00; cpu.hl.L = carry | (cpu.hl.L >> 1); cpu.af.flags |= ZFLAG(cpu.hl.L); }
static void rr_HL(){ int value = mmu_rb(cpu.HL); int carry = cpu.af.flags & C_FLAG ? 0x80 : 0x00; cpu.af.flags = (value & 0x01) ? C_FLAG : 0x00; value = carry | (value >> 1); cpu.af.flags |= ZFLAG(value); mmu_wb(cpu.HL, value); }

static void sla_b(){ int carry = cpu.bc.B & 0x80; cpu.bc.B <<= 1; cpu.af.flags = ZFLAG(cpu.bc.B) | (carry ? C_FLAG : 0); }
static void sla_c(){ int carry = cpu.bc.C & 0x80; cpu.bc.C <<= 1; cpu.af.flags = ZFLAG(cpu.bc.C) | (carry ? C_FLAG : 0); }
static void sla_d(){ int carry = cpu.de.D & 0x80; cpu.de.D <<= 1; cpu.af.flags = ZFLAG(cpu.de.D) | (carry ? C_FLAG : 0); }
static void sla_e(){ int carry = cpu.de.E & 0x80; cpu.de.E <<= 1; cpu.af.flags = ZFLAG(cpu.de.E) | (carry ? C_FLAG : 0); }
static void sla_h(){ int carry = cpu.hl.H & 0x80; cpu.hl.H <<= 1; cpu.af.flags = ZFLAG(cpu.hl.H) | (carry ? C_FLAG : 0); }
static void sla_l(){ int carry = cpu.hl.L & 0x80; cpu.hl.L <<= 1; cpu.af.flags = ZFLAG(cpu.hl.L) | (carry ? C_FLAG : 0); }
static void sla_HL(){ int value = mmu_rb(cpu.HL); int carry = value & 0x80; value <<= 1; cpu.af.flags = ZFLAG(value) | (carry ? C_FLAG : 0); mmu_wb(cpu.HL, value); }
static void sla_a(){ int carry = cpu.af.A & 0x80; cpu.af.A <<= 1; cpu.af.flags = ZFLAG(cpu.af.A) | (carry ? C_FLAG : 0); }

static void sra_b(){ int carry = cpu.bc.B & 0x01; int tmp = cpu.bc.B & 0x80; cpu.bc.B = tmp | (cpu.bc.B >> 1); cpu.af.flags = ZFLAG(cpu.bc.B) | (carry ? C_FLAG : 0); }
static void sra_c(){ int carry = cpu.bc.C & 0x01; int tmp = cpu.bc.C & 0x80; cpu.bc.C = tmp | (cpu.bc.C >> 1); cpu.af.flags = ZFLAG(cpu.bc.C) | (carry ? C_FLAG : 0); }
static void sra_d(){ int carry = cpu.de.D & 0x01; int tmp = cpu.de.D & 0x80; cpu.de.D = tmp | (cpu.de.D >> 1); cpu.af.flags = ZFLAG(cpu.de.D) | (carry ? C_FLAG : 0); }
static void sra_e(){ int carry = cpu.de.E & 0x01; int tmp = cpu.de.E & 0x80; cpu.de.E = tmp | (cpu.de.E >> 1); cpu.af.flags = ZFLAG(cpu.de.E) | (carry ? C_FLAG : 0); }
static void sra_h(){ int carry = cpu.hl.H & 0x01; int tmp = cpu.hl.H & 0x80; cpu.hl.H = tmp | (cpu.hl.H >> 1); cpu.af.flags = ZFLAG(cpu.hl.H) | (carry ? C_FLAG : 0); }
static void sra_l(){ int carry = cpu.hl.L & 0x01; int tmp = cpu.hl.L & 0x80; cpu.hl.L = tmp | (cpu.hl.L >> 1); cpu.af.flags = ZFLAG(cpu.hl.L) | (carry ? C_FLAG : 0); }
static void sra_HL(){ int value = mmu_rb(cpu.HL); int carry = value & 0x01; int tmp = value & 0x80; value = tmp | (value >> 1); cpu.af.flags = ZFLAG(value) | (carry ? C_FLAG : 0); mmu_wb(cpu.HL, value); }
static void sra_a(){ int carry = cpu.af.A & 0x01; int tmp = cpu.af.A & 0x80; cpu.af.A = tmp | (cpu.af.A >> 1); cpu.af.flags = ZFLAG(cpu.af.A) | (carry ? C_FLAG : 0); }

static void srl_b(){ int carry = cpu.bc.B & 0x01; cpu.bc.B = (cpu.bc.B >> 1); cpu.af.flags = ZFLAG(cpu.bc.B) | (carry ? C_FLAG : 0); }
static void srl_c(){ int carry = cpu.bc.C & 0x01; cpu.bc.C = (cpu.bc.C >> 1); cpu.af.flags = ZFLAG(cpu.bc.C) | (carry ? C_FLAG : 0); }
static void srl_d(){ int carry = cpu.de.D & 0x01; cpu.de.D = (cpu.de.D >> 1); cpu.af.flags = ZFLAG(cpu.de.D) | (carry ? C_FLAG : 0); }
static void srl_e(){ int carry = cpu.de.E & 0x01; cpu.de.E = (cpu.de.E >> 1); cpu.af.flags = ZFLAG(cpu.de.E) | (carry ? C_FLAG : 0); }
static void srl_h(){ int carry = cpu.hl.H & 0x01; cpu.hl.H = (cpu.hl.H >> 1); cpu.af.flags = ZFLAG(cpu.hl.H) | (carry ? C_FLAG : 0); }
static void srl_l(){ int carry = cpu.hl.L & 0x01; cpu.hl.L = (cpu.hl.L >> 1); cpu.af.flags = ZFLAG(cpu.hl.L) | (carry ? C_FLAG : 0); }
static void srl_HL(){ int value = mmu_rb(cpu.HL); int carry = value & 0x01; value = (value >> 1); cpu.af.flags = ZFLAG(value) | (carry ? C_FLAG : 0); mmu_wb(cpu.HL, value); }
static void srl_a(){ int carry = cpu.af.A & 0x01; cpu.af.A = (cpu.af.A >> 1); cpu.af.flags = ZFLAG(cpu.af.A) | (carry ? C_FLAG : 0); }


static void swap_b(){ cpu.bc.B = (cpu.bc.B >> 4) | (cpu.bc.B << 4); cpu.af.flags = ZFLAG(cpu.bc.B); }
static void swap_c(){ cpu.bc.C = (cpu.bc.C >> 4) | (cpu.bc.C << 4); cpu.af.flags = ZFLAG(cpu.bc.C); }
static void swap_h(){ cpu.hl.H = (cpu.hl.H >> 4) | (cpu.hl.H << 4); cpu.af.flags = ZFLAG(cpu.hl.H); }
static void swap_l(){ cpu.hl.L = (cpu.hl.L >> 4) | (cpu.hl.L << 4); cpu.af.flags = ZFLAG(cpu.hl.L); }
static void swap_d(){ cpu.de.D = (cpu.de.D >> 4) | (cpu.de.D << 4); cpu.af.flags = ZFLAG(cpu.de.D); }
static void swap_e(){ cpu.de.E = (cpu.de.E >> 4) | (cpu.de.E << 4); cpu.af.flags = ZFLAG(cpu.de.E); }
static void swap_HL(){ int value = mmu_rb(cpu.HL); value = (value >> 4) | (value << 4); cpu.af.flags = ZFLAG(value); mmu_wb(cpu.HL, value); }
static void swap_a(){ cpu.af.A = (cpu.af.A >> 4) | (cpu.af.A << 4); cpu.af.flags = ZFLAG(cpu.af.A); }

static void bit_bbb_b(){ int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; cpu.af.flags = H_FLAG | ((cpu.bc.B & (1 << b)) ? 0 : Z_FLAG) | KEEP(C_FLAG); }
static void bit_bbb_c(){ int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; cpu.af.flags = H_FLAG | ((cpu.bc.C & (1 << b)) ? 0 : Z_FLAG) | KEEP(C_FLAG); }
static void bit_bbb_d(){ int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; cpu.af.flags = H_FLAG | ((cpu.de.D & (1 << b)) ? 0 : Z_FLAG) | KEEP(C_FLAG); }
static void bit_bbb_e(){ int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; cpu.af.flags = H_FLAG | ((cpu.de.E & (1 << b)) ? 0 : Z_FLAG) | KEEP(C_FLAG); }
static void bit_bbb_h(){ int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; cpu.af.flags = H_FLAG | ((cpu.hl.H & (1 << b)) ? 0 : Z_FLAG) | KEEP(C_FLAG); }
static void bit_bbb_l(){ int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; cpu.af.flags = H_FLAG | ((cpu.hl.L & (1 << b)) ? 0 : Z_FLAG) | KEEP(C_FLAG); }
static void bit_bbb_HL(){ int value = mmu_rb(cpu.HL); int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; cpu.af.flags = H_FLAG | ((value & (1 << b)) ? 0 : Z_FLAG) | KEEP(C_FLAG); }
static void bit_bbb_a(){ int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; cpu.af.flags = H_FLAG | ((cpu.af.A & (1 << b)) ? 0 : Z_FLAG) | KEEP(C_FLAG); }

static void res_bbb_b(){ int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; CLEAR_BIT(cpu.bc.B, (1 << b)); }
static void res_bbb_c(){ int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; CLEAR_BIT(cpu.bc.C, (1 << b)); }
static void res_bbb_d(){ int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; CLEAR_BIT(cpu.de.D, (1 << b)); }
static void res_bbb_e(){ int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; CLEAR_BIT(cpu.de.E, (1 << b)); }
static void res_bbb_h(){ int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; CLEAR_BIT(cpu.hl.H, (1 << b)); }
static void res_bbb_l(){ int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; CLEAR_BIT(cpu.hl.L, (1 << b)); }
static void res_bbb_HL(){ int value = mmu_rb(cpu.HL); int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; CLEAR_BIT(value, (1 << b)); mmu_wb(cpu.HL, value); }
static void res_bbb_a(){ int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; CLEAR_BIT(cpu.af.A, (1 << b)); }


static void set_bbb_b(){ int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; SET_BIT(cpu.bc.B, (1 << b)); }
static void set_bbb_c(){ int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; SET_BIT(cpu.bc.C, (1 << b)); }
static void set_bbb_d(){ int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; SET_BIT(cpu.de.D, (1 << b)); }
static void set_bbb_e(){ int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; SET_BIT(cpu.de.E, (1 << b)); }
static void set_bbb_h(){ int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; SET_BIT(cpu.hl.H, (1 << b)); }
static void set_bbb_l(){ int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; SET_BIT(cpu.hl.L, (1 << b)); }
static void set_bbb_HL(){ int value = mmu_rb(cpu.HL); int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; SET_BIT(value, (1 << b)); mmu_wb(cpu.HL, value); }
static void set_bbb_a(){ int b = (mmu_rb(cpu.pc-1) >> 3) & 0x07; SET_BIT(cpu.af.A, (1 << b)); }

const CPU_OPS ext_map[] =
{
    {rlc_b, 4},
    {rlc_c, 4},
    {rlc_d, 4},
    {rlc_e, 4},
    {rlc_h, 4},
    {rlc_l, 4},
    {rlc_HL, 12},
    {rlc_a, 4},
    {rrc_b, 4},
    {rrc_c, 4},
    {rrc_d, 4},
    {rrc_e, 4},
    {rrc_h, 4},
    {rrc_l, 4},
    {rrc_HL, 12},
    {rrc_a, 4},

    {rl_b, 4},
    {rl_c, 4},
    {rl_d, 4},
    {rl_e, 4},
    {rl_h, 4},
    {rl_l, 4},
    {rl_HL, 12},
    {rl_a, 4},
    {rr_b, 4},
    {rr_c, 4},
    {rr_d, 4},
    {rr_e, 4},
    {rr_h, 4},
    {rr_l, 4},
    {rr_HL, 12},
    {rr_a, 4},

    {sla_b, 4},
    {sla_c, 4},
    {sla_d, 4},
    {sla_e, 4},
    {sla_h, 4},
    {sla_l, 4},
    {sla_HL, 12},
    {sla_a, 4},
    {sra_b, 4},
    {sra_c, 4},
    {sra_d, 4},
    {sra_e, 4},
    {sra_h, 4},
    {sra_l, 4},
    {sra_HL, 12},
    {sra_a, 4},

    {swap_b, 4},
    {swap_c, 4},
    {swap_d, 4},
    {swap_e, 4},
    {swap_h, 4},
    {swap_l, 4},
    {swap_HL, 12},
    {swap_a, 4},
    {srl_b, 4},
    {srl_c, 4},
    {srl_d, 4},
    {srl_e, 4},
    {srl_h, 4},
    {srl_l, 4},
    {srl_HL, 12},
    {srl_a, 4},

    {bit_bbb_b, 4},
    {bit_bbb_c, 4},
    {bit_bbb_d, 4},
    {bit_bbb_e, 4},
    {bit_bbb_h, 4},
    {bit_bbb_l, 4},
    {bit_bbb_HL, 8},
    {bit_bbb_a, 4},
    {bit_bbb_b, 4},
    {bit_bbb_c, 4},
    {bit_bbb_d, 4},
    {bit_bbb_e, 4},
    {bit_bbb_h, 4},
    {bit_bbb_l, 4},
    {bit_bbb_HL, 8},
    {bit_bbb_a, 4},

    {bit_bbb_b, 4},
    {bit_bbb_c, 4},
    {bit_bbb_d, 4},
    {bit_bbb_e, 4},
    {bit_bbb_h, 4},
    {bit_bbb_l, 4},
    {bit_bbb_HL, 8},
    {bit_bbb_a, 4},
    {bit_bbb_b, 4},
    {bit_bbb_c, 4},
    {bit_bbb_d, 4},
    {bit_bbb_e, 4},
    {bit_bbb_h, 4},
    {bit_bbb_l, 4},
    {bit_bbb_HL, 8},
    {bit_bbb_a, 4},

    {bit_bbb_b, 4},
    {bit_bbb_c, 4},
    {bit_bbb_d, 4},
    {bit_bbb_e, 4},
    {bit_bbb_h, 4},
    {bit_bbb_l, 4},
    {bit_bbb_HL, 8},
    {bit_bbb_a, 4},
    {bit_bbb_b, 4},
    {bit_bbb_c, 4},
    {bit_bbb_d, 4},
    {bit_bbb_e, 4},
    {bit_bbb_h, 4},
    {bit_bbb_l, 4},
    {bit_bbb_HL, 8},
    {bit_bbb_a, 4},

    {bit_bbb_b, 4},
    {bit_bbb_c, 4},
    {bit_bbb_d, 4},
    {bit_bbb_e, 4},
    {bit_bbb_h, 4},
    {bit_bbb_l, 4},
    {bit_bbb_HL, 8},
    {bit_bbb_a, 4},
    {bit_bbb_b, 4},
    {bit_bbb_c, 4},
    {bit_bbb_d, 4},
    {bit_bbb_e, 4},
    {bit_bbb_h, 4},
    {bit_bbb_l, 4},
    {bit_bbb_HL, 8},
    {bit_bbb_a, 4},

    {res_bbb_b, 4},
    {res_bbb_c, 4},
    {res_bbb_d, 4},
    {res_bbb_e, 4},
    {res_bbb_h, 4},
    {res_bbb_l, 4},
    {res_bbb_HL, 12},
    {res_bbb_a, 4},
    {res_bbb_b, 4},
    {res_bbb_c, 4},
    {res_bbb_d, 4},
    {res_bbb_e, 4},
    {res_bbb_h, 4},
    {res_bbb_l, 4},
    {res_bbb_HL, 12},
    {res_bbb_a, 4},

    {res_bbb_b, 4},
    {res_bbb_c, 4},
    {res_bbb_d, 4},
    {res_bbb_e, 4},
    {res_bbb_h, 4},
    {res_bbb_l, 4},
    {res_bbb_HL, 12},
    {res_bbb_a, 4},
    {res_bbb_b, 4},
    {res_bbb_c, 4},
    {res_bbb_d, 4},
    {res_bbb_e, 4},
    {res_bbb_h, 4},
    {res_bbb_l, 4},
    {res_bbb_HL, 12},
    {res_bbb_a, 4},

    {res_bbb_b, 4},
    {res_bbb_c, 4},
    {res_bbb_d, 4},
    {res_bbb_e, 4},
    {res_bbb_h, 4},
    {res_bbb_l, 4},
    {res_bbb_HL, 12},
    {res_bbb_a, 4},
    {res_bbb_b, 4},
    {res_bbb_c, 4},
    {res_bbb_d, 4},
    {res_bbb_e, 4},
    {res_bbb_h, 4},
    {res_bbb_l, 4},
    {res_bbb_HL, 12},
    {res_bbb_a, 4},

    {res_bbb_b, 4},
    {res_bbb_c, 4},
    {res_bbb_d, 4},
    {res_bbb_e, 4},
    {res_bbb_h, 4},
    {res_bbb_l, 4},
    {res_bbb_HL, 12},
    {res_bbb_a, 4},
    {res_bbb_b, 4},
    {res_bbb_c, 4},
    {res_bbb_d, 4},
    {res_bbb_e, 4},
    {res_bbb_h, 4},
    {res_bbb_l, 4},
    {res_bbb_HL, 12},
    {res_bbb_a, 4},

    {set_bbb_b, 4},
    {set_bbb_c, 4},
    {set_bbb_d, 4},
    {set_bbb_e, 4},
    {set_bbb_h, 4},
    {set_bbb_l, 4},
    {set_bbb_HL, 12},
    {set_bbb_a, 4},
    {set_bbb_b, 4},
    {set_bbb_c, 4},
    {set_bbb_d, 4},
    {set_bbb_e, 4},
    {set_bbb_h, 4},
    {set_bbb_l, 4},
    {set_bbb_HL, 12},
    {set_bbb_a, 4},

    {set_bbb_b, 4},
    {set_bbb_c, 4},
    {set_bbb_d, 4},
    {set_bbb_e, 4},
    {set_bbb_h, 4},
    {set_bbb_l, 4},
    {set_bbb_HL, 12},
    {set_bbb_a, 4},
    {set_bbb_b, 4},
    {set_bbb_c, 4},
    {set_bbb_d, 4},
    {set_bbb_e, 4},
    {set_bbb_h, 4},
    {set_bbb_l, 4},
    {set_bbb_HL, 12},
    {set_bbb_a, 4},

    {set_bbb_b, 4},
    {set_bbb_c, 4},
    {set_bbb_d, 4},
    {set_bbb_e, 4},
    {set_bbb_h, 4},
    {set_bbb_l, 4},
    {set_bbb_HL, 12},
    {set_bbb_a, 4},
    {set_bbb_b, 4},
    {set_bbb_c, 4},
    {set_bbb_d, 4},
    {set_bbb_e, 4},
    {set_bbb_h, 4},
    {set_bbb_l, 4},
    {set_bbb_HL, 12},
    {set_bbb_a, 4},

    {set_bbb_b, 4},
    {set_bbb_c, 4},
    {set_bbb_d, 4},
    {set_bbb_e, 4},
    {set_bbb_h, 4},
    {set_bbb_l, 4},
    {set_bbb_HL, 12},
    {set_bbb_a, 4},
    {set_bbb_b, 4},
    {set_bbb_c, 4},
    {set_bbb_d, 4},
    {set_bbb_e, 4},
    {set_bbb_h, 4},
    {set_bbb_l, 4},
    {set_bbb_HL, 12},
    {set_bbb_a, 4},

};

static void prefix_cb()
{
    unsigned char opcode;
    /* lê uma instrução */
    opcode = mmu_rb(cpu.pc++);
    /* duração da instrução */
    clock.t_clock += ext_map[opcode].t_clock;
    /* executa a instrução */
    ext_map[opcode].exec();
}

const CPU_OPS map[] =
{
    {nop, 4},
    {ld_bc_nn, 12},
    {ld_BC_a, 8},
    {inc_bc, 8},
    {inc_b, 4},
    {dec_b, 4},
    {ld_b_n, 8},
    {rlca, 4},
    {ld_NN_sp, 20},
    {add_hl_bc, 8},
    {ld_a_BC, 8},
    {dec_bc, 8},
    {inc_c, 4},
    {dec_c, 4},
    {ld_c_n, 8},
    {rrca, 4},

    {stop, 4},
    {ld_de_nn, 12},
    {ld_DE_a, 8},
    {inc_de, 8},
    {inc_d, 4},
    {dec_d, 4},
    {ld_d_n, 8},
    {rla, 4},
    {jr_n, 12},
    {add_hl_de, 8},
    {ld_a_DE, 8},
    {dec_de, 8},
    {inc_e, 4},
    {dec_e, 4},
    {ld_e_n, 8},
    {rra, 4},

    {jr_nz_n, 8},
    {ld_hl_nn, 12},
    {ld_HLi_a, 8},
    {inc_hl, 8},
    {inc_h, 4},
    {dec_h, 4},
    {ld_h_n, 8},
    {daa, 4},
    {jr_z_n, 8},
    {add_hl_hl, 8},
    {ld_a_HLi, 8},
    {dec_hl, 8},
    {inc_l, 4},
    {dec_l, 4},
    {ld_l_n, 8},
    {cpl, 4},

    {jr_nc_n, 8},
    {ld_sp_nn, 12},
    {ld_HLd_a, 8},
    {inc_sp, 8},
    {inc_HL, 12},
    {dec_HL, 12},
    {ld_HL_n, 12},
    {scf, 4},
    {jr_c_n, 8},
    {add_hl_sp, 8},
    {ld_a_HLd, 8},
    {dec_sp, 8},
    {inc_a, 4},
    {dec_a, 4},
    {ld_a_n, 8},
    {ccf, 4},

    {ld_b_b, 4},
    {ld_b_c, 4},
    {ld_b_d, 4},
    {ld_b_e, 4},
    {ld_b_h, 4},
    {ld_b_l, 4},
    {ld_b_HL, 8},
    {ld_b_a, 4},
    {ld_c_b, 4},
    {ld_c_c, 4},
    {ld_c_d, 4},
    {ld_c_e, 4},
    {ld_c_h, 4},
    {ld_c_l, 4},
    {ld_c_HL, 8},
    {ld_c_a, 4},

    {ld_d_b, 4},
    {ld_d_c, 4},
    {ld_d_d, 4},
    {ld_d_e, 4},
    {ld_d_h, 4},
    {ld_d_l, 4},
    {ld_d_HL, 8},
    {ld_d_a, 4},
    {ld_e_b, 4},
    {ld_e_c, 4},
    {ld_e_d, 4},
    {ld_e_e, 4},
    {ld_e_h, 4},
    {ld_e_l, 4},
    {ld_e_HL, 8},
    {ld_e_a, 4},

    {ld_h_b, 4},
    {ld_h_c, 4},
    {ld_h_d, 4},
    {ld_h_e, 4},
    {ld_h_h, 4},
    {ld_h_l, 4},
    {ld_h_HL, 8},
    {ld_h_a, 4},
    {ld_l_b, 4},
    {ld_l_c, 4},
    {ld_l_d, 4},
    {ld_l_e, 4},
    {ld_l_h, 4},
    {ld_l_l, 4},
    {ld_l_HL, 8},
    {ld_l_a, 4},

    {ld_HL_b, 8},
    {ld_HL_c, 8},
    {ld_HL_d, 8},
    {ld_HL_e, 8},
    {ld_HL_h, 8},
    {ld_HL_l, 8},
    {halt, 4},
    {ld_HL_a, 8},
    {ld_a_b, 4},
    {ld_a_c, 4},
    {ld_a_d, 4},
    {ld_a_e, 4},
    {ld_a_h, 4},
    {ld_a_l, 4},
    {ld_a_HL, 8},
    {ld_a_a, 4},

    {add_a_b, 4},
    {add_a_c, 4},
    {add_a_d, 4},
    {add_a_e, 4},
    {add_a_h, 4},
    {add_a_l, 4},
    {add_a_HL, 8},
    {add_a_a, 4},
    {adc_a_b, 4},
    {adc_a_c, 4},
    {adc_a_d, 4},
    {adc_a_e, 4},
    {adc_a_h, 4},
    {adc_a_l, 4},
    {adc_a_HL, 8},
    {adc_a_a, 4},

    {sub_b, 4},
    {sub_c, 4},
    {sub_d, 4},
    {sub_e, 4},
    {sub_h, 4},
    {sub_l, 4},
    {sub_HL, 8},
    {sub_a, 4},
    {sbc_a_b, 4},
    {sbc_a_c, 4},
    {sbc_a_d, 4},
    {sbc_a_e, 4},
    {sbc_a_h, 4},
    {sbc_a_l, 4},
    {sbc_a_HL, 8},
    {sbc_a_a, 4},

    {and_b, 4},
    {and_c, 4},
    {and_d, 4},
    {and_e, 4},
    {and_h, 4},
    {and_l, 4},
    {and_HL, 8},
    {and_a, 4},
    {xor_b, 4},
    {xor_c, 4},
    {xor_d, 4},
    {xor_e, 4},
    {xor_h, 4},
    {xor_l, 4},
    {xor_HL, 8},
    {xor_a, 4},

    {or_b, 4},
    {or_c, 4},
    {or_d, 4},
    {or_e, 4},
    {or_h, 4},
    {or_l, 4},
    {or_HL, 8},
    {or_a, 4},
    {cp_b, 4},
    {cp_c, 4},
    {cp_d, 4},
    {cp_e, 4},
    {cp_h, 4},
    {cp_l, 4},
    {cp_HL, 8},
    {cp_a, 4},

    {ret_nz, 8},
    {pop_bc, 12},
    {jp_nz_nn, 12},
    {jp_nn, 16},
    {call_nz_nn, 12},
    {push_bc, 16},
    {add_a_n, 8},
    {rst_00h, 4},
    {ret_z, 8},
    {ret, 16},
    {jp_z_nn, 12},
    {prefix_cb, 4},
    {call_z_nn, 12},
    {call_nn, 24},
    {adc_a_n, 8},
    {rst_08h, 4},//clock=4+12

    {ret_nc, 8},
    {pop_de, 12},
    {jp_nc_nn, 12},
    {nop, 4},
    {call_nc_nn, 12},
    {push_de, 16},
    {sub_n, 8},
    {rst_10h, 4},
    {ret_c, 8},
    {reti, 16},
    {jp_c_nn, 12},
    {nop, 4},
    {call_c_nn, 12},
    {nop, 24},
    {sbc_a_n, 8},
    {rst_18h, 4},//clock=4+12

    {ldh_N_a, 12},
    {pop_hl, 12},
    {ld_C_a, 8},
    {nop, 4},
    {nop, 4},
    {push_hl, 16},
    {and_n, 8},
    {rst_20h, 4},
    {add_sp_n, 16},
    {jp_HL, 4},
    {ld_NN_a, 16},
    {nop, 4},
    {nop, 4},
    {nop, 4},
    {xor_n, 8},
    {rst_28h, 4},//clock=4+12

    {ldh_a_N, 12},
    {pop_af, 12},
    {ld_a_C, 8},
    {di, 4},
    {nop, 4},
    {push_af, 16},
    {or_n, 8},
    {rst_30h, 4},
    {ld_hl_sp_n, 12},
    {ld_sp_hl, 8},
    {ld_a_NN, 16},
    {ei, 4},
    {nop, 4},
    {nop, 4},
    {cp_n, 8},
    {rst_38h, 4},//clock=4+12
};

void test()
{
    //cpu.pc = 5;
    //jr_n();
    //cpu.HL = 0x60;
    //cpu.BC = 0x32;
    //add_hl_bc();
    //cpu.af.A = cpu.HL & 0xFF;
    //daa();
    cpu.af.flags = C_FLAG;
    cpu.de.E = 0x0;
    rl_e();
    printf("F=0x%x\n", cpu.af.flags);
    printf("E=0x%x\n", cpu.de.E);
    getchar();
}

void cpu_reset()
{
    cpu.pc = 0x0100;
    cpu.AF = 0x1100;
    cpu.sp = 0xFFFE;
    //cpu.BC = 0x0013;
    //cpu.DE = 0x00D8;
    //cpu.HL = 0x014D;
    //mmu.lcdc = 40;
    cpu.ime = 1;
}
int a = 0;
void debug(int opcode)
{
    if((cpu.pc-1 == 0x0434) || a)
    {
        a = 1;
        printf("opcode:0x%x\n", opcode);
        printf("AF=0x%x\n", cpu.AF);
        printf("BC=0x%x\n", cpu.BC);
        printf("DE=0x%x\n", cpu.DE);
        printf("HL=0x%x\n", cpu.HL);
        printf("SP=0x%x\n", cpu.sp);
        printf("PC=0x%x\n", cpu.pc-1);
        printf("ly=0x%x\n", mmu.ly);
        printf("0xffff=0x%x\n", mmu_rw(0xffff));
        printf("int=%d\n", cpu.ime);
        getchar();
    }
}

INLINE void cpu_step(void)
{
	unsigned char opcode;
	if(halted)
	{
	    clock.t_clock = 4;
	    goto INT;
	}

    if(cpu.pc == 0x0100)
        mmu.in_bios = 0;

    /* lê uma instrução */
    opcode = mmu_rb(cpu.pc++);
    /* duração da instrução */
    clock.t_clock = map[opcode].t_clock;
    /* executa a instrução */
    //debug(opcode);
    map[opcode].exec();
    /* verifica se houve alguma interrupção */
    INT:
    if(mmu.ie && mmu.iflg)
    {
        int flag = mmu.ie & mmu.iflg;

        if(flag)
            halted = 0;

        if(!cpu.ime)
            goto EXIT;

        if(flag & VBLANK_INT_FLAG)
        {
            cpu.ime = 0;
            int_jmp(0x0040);
            CLEAR_BIT(mmu.iflg, VBLANK_INT_FLAG);
        }
        else if(flag & LCDC_STAT_INT_FLAG)
        {
            cpu.ime = 0;
            int_jmp(0x0048);
            CLEAR_BIT(mmu.iflg, LCDC_STAT_INT_FLAG);
        }
        else if(flag & TIMER_OVLW_INT_FLAG)
        {
            cpu.ime = 0;
            int_jmp(0x0050);
            CLEAR_BIT(mmu.iflg, TIMER_OVLW_INT_FLAG);
        }
        else if(flag & SERIAL_INT_FLAG)
        {
            cpu.ime = 0;
            int_jmp(0x0058);
            CLEAR_BIT(mmu.iflg, SERIAL_INT_FLAG);
        }
        else if(flag & P10_P13_INT_FLAG)
        {
            cpu.ime = 0;
            int_jmp(0x0060);
            CLEAR_BIT(mmu.iflg, P10_P13_INT_FLAG);
        }
    }
    EXIT:;
}
