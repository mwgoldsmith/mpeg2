/*
 * idct_altivec.c
 * Copyright (C) 1999-2001 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#ifdef ARCH_PPC

#include <inttypes.h>

#include "mpeg2_internal.h"

#if 0 /* C version of the altivec idct, requires compiler extensions */

#define vector_t vector signed short

#define IDCT_HALF					\
    /* 1st stage */					\
    t1 = vec_mradds (a1, vx7, vx1 );			\
    t8 = vec_mradds (a1, vx1, vec_subs (zero, vx7));	\
    t7 = vec_mradds (a2, vx5, vx3);			\
    t3 = vec_mradds (ma2, vx3, vx5);			\
							\
    /* 2nd stage */					\
    t5 = vec_adds (vx0, vx4);				\
    t0 = vec_subs (vx0, vx4);				\
    t2 = vec_mradds (a0, vx6, vx2);			\
    t4 = vec_mradds (a0, vx2, vec_subs (zero,vx6));	\
    t6 = vec_adds (t8, t3);				\
    t3 = vec_subs (t8, t3);				\
    t8 = vec_subs (t1, t7);				\
    t1 = vec_adds (t1, t7);				\
							\
    /* 3rd stage */					\
    t7 = vec_adds (t5, t2);				\
    t2 = vec_subs (t5, t2);				\
    t5 = vec_adds (t0, t4);				\
    t0 = vec_subs (t0, t4);				\
    t4 = vec_subs (t8, t3);				\
    t3 = vec_adds (t8, t3);				\
							\
    /* 4th stage */					\
    vy0 = vec_adds (t7, t1);				\
    vy7 = vec_subs (t7, t1);				\
    vy1 = vec_mradds (c4, t3, t5);			\
    vy6 = vec_mradds (mc4, t3, t5);			\
    vy2 = vec_mradds (c4, t4, t0);			\
    vy5 = vec_mradds (mc4, t4, t0);			\
    vy3 = vec_adds (t2, t6);				\
    vy4 = vec_subs (t2, t6);

#define IDCT								\
    vector_t vx0, vx1, vx2, vx3, vx4, vx5, vx6, vx7;			\
    vector_t vy0, vy1, vy2, vy3, vy4, vy5, vy6, vy7;			\
    vector_t a0, a1, a2, ma2, c4, mc4, zero, bias;			\
    vector_t t0, t1, t2, t3, t4, t5, t6, t7, t8;			\
    vector unsigned short shift;					\
									\
    c4 = vec_splat (constants[0], 0);					\
    a0 = vec_splat (constants[0], 1);					\
    a1 = vec_splat (constants[0], 2);					\
    a2 = vec_splat (constants[0], 3);					\
    mc4 = vec_splat (constants[0], 4);					\
    ma2 = vec_splat (constants[0], 5);					\
    bias = (vector_t)vec_splat ((vector signed int)constants[0], 3);	\
									\
    zero = vec_splat_s16 (0);						\
    shift = vec_splat_u16 (4);						\
									\
    vx0 = vec_mradds (vec_sl (block[0], shift), constants[1], zero);	\
    vx1 = vec_mradds (vec_sl (block[1], shift), constants[2], zero);	\
    vx2 = vec_mradds (vec_sl (block[2], shift), constants[3], zero);	\
    vx3 = vec_mradds (vec_sl (block[3], shift), constants[4], zero);	\
    vx4 = vec_mradds (vec_sl (block[4], shift), constants[1], zero);	\
    vx5 = vec_mradds (vec_sl (block[5], shift), constants[4], zero);	\
    vx6 = vec_mradds (vec_sl (block[6], shift), constants[3], zero);	\
    vx7 = vec_mradds (vec_sl (block[7], shift), constants[2], zero);	\
									\
    IDCT_HALF								\
									\
    vx0 = vec_mergeh (vy0, vy4);					\
    vx1 = vec_mergel (vy0, vy4);					\
    vx2 = vec_mergeh (vy1, vy5);					\
    vx3 = vec_mergel (vy1, vy5);					\
    vx4 = vec_mergeh (vy2, vy6);					\
    vx5 = vec_mergel (vy2, vy6);					\
    vx6 = vec_mergeh (vy3, vy7);					\
    vx7 = vec_mergel (vy3, vy7);					\
									\
    vy0 = vec_mergeh (vx0, vx4);					\
    vy1 = vec_mergel (vx0, vx4);					\
    vy2 = vec_mergeh (vx1, vx5);					\
    vy3 = vec_mergel (vx1, vx5);					\
    vy4 = vec_mergeh (vx2, vx6);					\
    vy5 = vec_mergel (vx2, vx6);					\
    vy6 = vec_mergeh (vx3, vx7);					\
    vy7 = vec_mergel (vx3, vx7);					\
									\
    vx0 = vec_adds (vec_mergeh (vy0, vy4), bias);			\
    vx1 = vec_mergel (vy0, vy4);					\
    vx2 = vec_mergeh (vy1, vy5);					\
    vx3 = vec_mergel (vy1, vy5);					\
    vx4 = vec_mergeh (vy2, vy6);					\
    vx5 = vec_mergel (vy2, vy6);					\
    vx6 = vec_mergeh (vy3, vy7);					\
    vx7 = vec_mergel (vy3, vy7);					\
									\
    IDCT_HALF								\
									\
    shift = vec_splat_u16 (6);						\
    vx0 = vec_sra (vy0, shift);						\
    vx1 = vec_sra (vy1, shift);						\
    vx2 = vec_sra (vy2, shift);						\
    vx3 = vec_sra (vy3, shift);						\
    vx4 = vec_sra (vy4, shift);						\
    vx5 = vec_sra (vy5, shift);						\
    vx6 = vec_sra (vy6, shift);						\
    vx7 = vec_sra (vy7, shift);

static vector_t constants[5] = {
    (vector_t)(23170, 13573, 6518, 21895, -23170, -21895, 32, 31),
    (vector_t)(16384, 22725, 21407, 19266, 16384, 19266, 21407, 22725),
    (vector_t)(22725, 31521, 29692, 26722, 22725, 26722, 29692, 31521),
    (vector_t)(21407, 29692, 27969, 25172, 21407, 25172, 27969, 29692),
    (vector_t)(19266, 26722, 25172, 22654, 19266, 22654, 25172, 26722)
};

void idct_block_add_altivec (vector_t * block, uint8_t * dest, int stride)
{
    vector_t tmp, tmp2;

    IDCT

#define ADD(dest,src)					\
    *(uint64_t *)&tmp = *(uint64_t *)dest;		\
    tmp2 = vec_adds (vec_mergeh (zero, tmp), src);	\
    tmp = (vector_t)vec_packsu (tmp2, tmp2);		\
    *(uint64_t *)dest = *(uint64_t *)&tmp;

    ADD (dest, vx0)	dest += stride;
    ADD (dest, vx1)	dest += stride;
    ADD (dest, vx2)	dest += stride;
    ADD (dest, vx3)	dest += stride;
    ADD (dest, vx4)	dest += stride;
    ADD (dest, vx5)	dest += stride;
    ADD (dest, vx6)	dest += stride;
    ADD (dest, vx7)
}

void idct_block_copy_altivec (vector_t * block, uint8_t * dest, int stride)
{
    vector_t tmp;

    IDCT

#define COPY(dest,src)				\
    tmp = (vector_t)vec_packsu (src, src);	\
    *((uint64_t *)dest) = *(uint64_t *)&tmp;

    COPY (dest, vx0)	dest += stride;
    COPY (dest, vx1)	dest += stride;
    COPY (dest, vx2)	dest += stride;
    COPY (dest, vx3)	dest += stride;
    COPY (dest, vx4)	dest += stride;
    COPY (dest, vx5)	dest += stride;
    COPY (dest, vx6)	dest += stride;
    COPY (dest, vx7)
}

#else /* asm version of the altivec idct, converted from the above */

asm ("									\n"
"	.section	\".data\"					\n"
"	.align 4							\n"
"	.type	 prescale,@object					\n"
"	.size	 prescale,64						\n"
"prescale:								\n"
"	.long 1073764549						\n"
"	.long 1402948418						\n"
"	.long 1073761090						\n"
"	.long 1402951877						\n"
"	.long 1489337121						\n"
"	.long 1945921634						\n"
"	.long 1489332322						\n"
"	.long 1945926433						\n"
"	.long 1402958844						\n"
"	.long 1833001556						\n"
"	.long 1402954324						\n"
"	.long 1833006076						\n"
"	.long 1262643298						\n"
"	.long 1649694846						\n"
"	.long 1262639230						\n"
"	.long 1649698914						\n"
"	.align 4							\n"
"	.type	 constants,@object					\n"
"	.size	 constants,16						\n"
"constants:								\n"
"	.long 1518482693						\n"
"	.long 427185543							\n"
"	.long -1518425479						\n"
"	.long 2097183							\n"
"	.section	\".text\"					\n"
"	.align 2							\n"
"	.globl idct_block_add_altivec					\n"
"	.type	 idct_block_add_altivec,@function			\n"
"idct_block_add_altivec:						\n"
"	.extern _savev23						\n"
"	.extern _restv23						\n"
"	stwu 1,-176(1)							\n"
"	mflr 0								\n"
"	stw 0,180(1)							\n"
"	addi 0,1,176							\n"
"	bl _savev23							\n"
"	addi 9,3,112							\n"
"	lvx 0,0,3							\n"
"	vspltish 25,4							\n"
"	lvx 12,0,9							\n"
"	lis 10,prescale@ha						\n"
"	vspltisw 13,0							\n"
"	addi 9,3,16							\n"
"	lvx 10,0,9							\n"
"	la 10,prescale@l(10)						\n"
"	lvx 5,0,10							\n"
"	addi 11,10,16							\n"
"	vslh 0,0,25							\n"
"	addi 8,3,48							\n"
"	lvx 7,0,11							\n"
"	vslh 12,12,25							\n"
"	addi 9,3,80							\n"
"	lvx 11,0,8							\n"
"	lvx 1,0,9							\n"
"	addi 11,10,48							\n"
"	vslh 10,10,25							\n"
"	lvx 8,0,11							\n"
"	vmhraddshs 18,0,5,13						\n"
"	addi 8,3,96							\n"
"	addi 11,3,32							\n"
"	lvx 0,0,8							\n"
"	vmhraddshs 27,12,7,13						\n"
"	lis 9,constants@ha						\n"
"	lvx 9,0,11							\n"
"	vslh 11,11,25							\n"
"	vslh 1,1,25							\n"
"	la 9,constants@l(9)						\n"
"	lvx 12,0,9							\n"
"	vmhraddshs 14,10,7,13						\n"
"	addi 10,10,32							\n"
"	lvx 10,0,10							\n"
"	vmhraddshs 23,1,8,13						\n"
"	addi 3,3,64							\n"
"	vmhraddshs 24,11,8,13						\n"
"	lvx 1,0,3							\n"
"	lfd 0,0(4)							\n"
"	vslh 9,9,25							\n"
"	addi 9,1,16							\n"
"	vslh 0,0,25							\n"
"	vsplth 8,12,2							\n"
"	stfd 0,16(1)							\n"
"	vsubshs 11,13,27						\n"
"	vsplth 3,12,5							\n"
"	lvx 6,0,9							\n"
"	vmhraddshs 19,0,10,13						\n"
"	vsplth 4,12,3							\n"
"	vmhraddshs 28,9,10,13						\n"
"	vsplth 7,12,1							\n"
"	vmhraddshs 31,8,14,11						\n"
"	vsplth 10,12,0							\n"
"	vmhraddshs 2,3,24,23						\n"
"	vsplth 11,12,4							\n"
"	vslh 1,1,25							\n"
"	vspltw 12,12,3							\n"
"	vmhraddshs 26,4,23,24						\n"
"	vspltish 25,6							\n"
"	vmhraddshs 9,1,5,13						\n"
"	vmrghh 6,13,6							\n"
"	vmhraddshs 27,8,27,14						\n"
"	vsubshs 0,13,19							\n"
"	vmhraddshs 14,7,19,28						\n"
"	vaddshs 23,31,2							\n"
"	vmhraddshs 1,7,28,0						\n"
"	vsubshs 2,31,2							\n"
"	vaddshs 19,18,9							\n"
"	vsubshs 31,27,26						\n"
"	vaddshs 27,27,26						\n"
"	vsubshs 9,18,9							\n"
"	vaddshs 26,19,14						\n"
"	vsubshs 14,19,14						\n"
"	vaddshs 19,9,1							\n"
"	vsubshs 9,9,1							\n"
"	vsubshs 1,31,2							\n"
"	vaddshs 2,31,2							\n"
"	vmhraddshs 15,11,1,9						\n"
"	vmhraddshs 29,11,2,19						\n"
"	vmhraddshs 16,10,2,19						\n"
"	vmhraddshs 17,10,1,9						\n"
"	vaddshs 30,14,23						\n"
"	vsubshs 18,14,23						\n"
"	vaddshs 5,26,27							\n"
"	vsubshs 1,26,27							\n"
"	vmrglh 24,16,15							\n"
"	vmrglh 14,5,18							\n"
"	vmrglh 23,17,29							\n"
"	vmrglh 27,30,1							\n"
"	vmrghh 19,30,1							\n"
"	vmrghh 9,17,29							\n"
"	vmrghh 18,5,18							\n"
"	vmrglh 30,14,23							\n"
"	vmrglh 1,24,27							\n"
"	vmrghh 28,16,15							\n"
"	vmrglh 16,18,9							\n"
"	vmrghh 5,18,9							\n"
"	vmrghh 29,24,27							\n"
"	vmrglh 27,30,1							\n"
"	vmrghh 18,28,19							\n"
"	vmrglh 15,28,19							\n"
"	vsubshs 0,13,27							\n"
"	vmrghh 17,14,23							\n"
"	vmrglh 14,5,18							\n"
"	vmrglh 23,17,29							\n"
"	vmrglh 24,16,15							\n"
"	vmhraddshs 31,8,14,0						\n"
"	vmrghh 19,30,1							\n"
"	vmhraddshs 27,8,27,14						\n"
"	vmhraddshs 2,3,24,23						\n"
"	vmrghh 28,16,15							\n"
"	vmhraddshs 26,4,23,24						\n"
"	vmrghh 0,5,18							\n"
"	vmhraddshs 14,7,19,28						\n"
"	vmrghh 9,17,29							\n"
"	vaddshs 18,0,12							\n"
"	vsubshs 1,13,19							\n"
"	vaddshs 23,31,2							\n"
"	vsubshs 2,31,2							\n"
"	vaddshs 19,18,9							\n"
"	vsubshs 31,27,26						\n"
"	vaddshs 27,27,26						\n"
"	vaddshs 26,19,14						\n"
"	vsubshs 9,18,9							\n"
"	vaddshs 5,26,27							\n"
"	vmhraddshs 1,7,28,1						\n"
"	vsrah 18,5,25							\n"
"	vsubshs 14,19,14						\n"
"	vaddshs 0,6,18							\n"
"	vaddshs 30,14,23						\n"
"	vpkshus 0,0,0							\n"
"	vaddshs 19,9,1							\n"
"	stvx 0,0,9							\n"
"	vsubshs 9,9,1							\n"
"	lfd 0,16(1)							\n"
"	vsubshs 1,31,2							\n"
"	stfd 0,0(4)							\n"
"	vaddshs 2,31,2							\n"
"	add 4,4,5							\n"
"	vsubshs 18,14,23						\n"
"	lfd 0,0(4)							\n"
"	vmhraddshs 16,10,2,19						\n"
"	stfd 0,16(1)							\n"
"	vmhraddshs 17,10,1,9						\n"
"	lvx 0,0,9							\n"
"	vmhraddshs 15,11,1,9						\n"
"	vsubshs 1,26,27							\n"
"	vsrah 24,30,25							\n"
"	vsrah 14,16,25							\n"
"	vmrghh 0,13,0							\n"
"	vsrah 28,17,25							\n"
"	vsrah 27,1,25							\n"
"	vaddshs 0,0,14							\n"
"	vsrah 9,18,25							\n"
"	vpkshus 0,0,0							\n"
"	vsrah 23,15,25							\n"
"	stvx 0,0,9							\n"
"	vmhraddshs 29,11,2,19						\n"
"	lfd 0,16(1)							\n"
"	stfd 0,0(4)							\n"
"	add 4,4,5							\n"
"	lfd 0,0(4)							\n"
"	stfd 0,16(1)							\n"
"	vsrah 19,29,25							\n"
"	lvx 0,0,9							\n"
"	vmrghh 0,13,0							\n"
"	vaddshs 0,0,28							\n"
"	vpkshus 1,0,0							\n"
"	stvx 1,0,9							\n"
"	lfd 0,16(1)							\n"
"	stfd 0,0(4)							\n"
"	add 4,4,5							\n"
"	lfd 0,0(4)							\n"
"	stfd 0,16(1)							\n"
"	lvx 0,0,9							\n"
"	vmrghh 0,13,0							\n"
"	vaddshs 0,0,24							\n"
"	vpkshus 1,0,0							\n"
"	stvx 1,0,9							\n"
"	lfd 0,16(1)							\n"
"	stfd 0,0(4)							\n"
"	add 4,4,5							\n"
"	lfd 0,0(4)							\n"
"	stfd 0,16(1)							\n"
"	lvx 0,0,9							\n"
"	vmrghh 0,13,0							\n"
"	vaddshs 0,0,9							\n"
"	vpkshus 1,0,0							\n"
"	stvx 1,0,9							\n"
"	lfd 0,16(1)							\n"
"	stfd 0,0(4)							\n"
"	add 4,4,5							\n"
"	lfd 0,0(4)							\n"
"	stfd 0,16(1)							\n"
"	lvx 0,0,9							\n"
"	vmrghh 0,13,0							\n"
"	vaddshs 0,0,23							\n"
"	vpkshus 1,0,0							\n"
"	stvx 1,0,9							\n"
"	lfd 0,16(1)							\n"
"	stfd 0,0(4)							\n"
"	add 4,4,5							\n"
"	lfd 0,0(4)							\n"
"	stfd 0,16(1)							\n"
"	lvx 0,0,9							\n"
"	vmrghh 0,13,0							\n"
"	vaddshs 0,0,19							\n"
"	vpkshus 1,0,0							\n"
"	stvx 1,0,9							\n"
"	lfd 0,16(1)							\n"
"	stfd 0,0(4)							\n"
"	add 4,4,5							\n"
"	lfd 0,0(4)							\n"
"	stfd 0,16(1)							\n"
"	lvx 0,0,9							\n"
"	vmrghh 13,13,0							\n"
"	vaddshs 0,13,27							\n"
"	vpkshus 0,0,0							\n"
"	stvx 0,0,9							\n"
"	lfd 0,16(1)							\n"
"	stfd 0,0(4)							\n"
"	addi 0,1,176							\n"
"	bl _restv23							\n"
"	lwz 0,180(1)							\n"
"	mtlr 0								\n"
"	la 1,176(1)							\n"
"	blr								\n"
".Lfe2:									\n"
"	.size	 idct_block_add_altivec,.Lfe2-idct_block_add_altivec	\n"
"	.align 2							\n"
"	.globl idct_block_copy_altivec					\n"
"	.type	 idct_block_copy_altivec,@function			\n"
"idct_block_copy_altivec:						\n"
"	.extern _savev24						\n"
"	.extern _restv24						\n"
"	stwu 1,-160(1)							\n"
"	mflr 0								\n"
"	stw 0,164(1)							\n"
"	addi 0,1,160							\n"
"	bl _savev24							\n"
"	addi 9,3,112							\n"
"	lvx 0,0,3							\n"
"	vspltish 25,4							\n"
"	lvx 13,0,9							\n"
"	lis 10,prescale@ha						\n"
"	vspltisw 12,0							\n"
"	addi 9,3,16							\n"
"	lvx 10,0,9							\n"
"	la 10,prescale@l(10)						\n"
"	lvx 6,0,10							\n"
"	addi 11,10,16							\n"
"	vslh 0,0,25							\n"
"	addi 8,3,48							\n"
"	lvx 7,0,11							\n"
"	vslh 13,13,25							\n"
"	addi 9,3,80							\n"
"	lvx 11,0,8							\n"
"	lvx 1,0,9							\n"
"	addi 11,10,48							\n"
"	vslh 10,10,25							\n"
"	lvx 8,0,11							\n"
"	vmhraddshs 19,0,6,12						\n"
"	addi 8,3,96							\n"
"	addi 11,3,32							\n"
"	lvx 0,0,8							\n"
"	vmhraddshs 29,13,7,12						\n"
"	lis 9,constants@ha						\n"
"	lvx 9,0,11							\n"
"	vslh 11,11,25							\n"
"	vslh 1,1,25							\n"
"	la 9,constants@l(9)						\n"
"	lvx 13,0,9							\n"
"	vmhraddshs 31,10,7,12						\n"
"	addi 10,10,32							\n"
"	lvx 10,0,10							\n"
"	vmhraddshs 2,1,8,12						\n"
"	addi 3,3,64							\n"
"	vmhraddshs 17,11,8,12						\n"
"	lvx 1,0,3							\n"
"	addi 9,1,16							\n"
"	vslh 9,9,25							\n"
"	vslh 0,0,25							\n"
"	vsplth 8,13,2							\n"
"	vsubshs 11,12,29						\n"
"	vsplth 5,13,5							\n"
"	vmhraddshs 3,0,10,12						\n"
"	vsplth 4,13,3							\n"
"	vmhraddshs 18,9,10,12						\n"
"	vsplth 7,13,1							\n"
"	vmhraddshs 9,5,17,2						\n"
"	vsplth 10,13,0							\n"
"	vmhraddshs 16,8,31,11						\n"
"	vslh 1,1,25							\n"
"	vsplth 11,13,4							\n"
"	vmhraddshs 27,4,2,17						\n"
"	vspltw 13,13,3							\n"
"	vmhraddshs 6,1,6,12						\n"
"	vspltish 25,6							\n"
"	vmhraddshs 29,8,29,31						\n"
"	vmhraddshs 17,7,3,18						\n"
"	vsubshs 0,12,3							\n"
"	vaddshs 24,16,9							\n"
"	vmhraddshs 3,7,18,0						\n"
"	vsubshs 9,16,9							\n"
"	vaddshs 2,19,6							\n"
"	vsubshs 16,29,27						\n"
"	vaddshs 29,29,27						\n"
"	vsubshs 1,19,6							\n"
"	vaddshs 27,2,17							\n"
"	vsubshs 17,2,17							\n"
"	vaddshs 2,1,3							\n"
"	vsubshs 1,1,3							\n"
"	vsubshs 3,16,9							\n"
"	vaddshs 9,16,9							\n"
"	vmhraddshs 6,10,3,1						\n"
"	vmhraddshs 26,11,9,2						\n"
"	vmhraddshs 28,11,3,1						\n"
"	vmhraddshs 15,10,9,2						\n"
"	vsubshs 1,17,24							\n"
"	vaddshs 19,27,29						\n"
"	vaddshs 30,17,24						\n"
"	vmrglh 2,6,26							\n"
"	vsubshs 14,27,29						\n"
"	vmrglh 31,19,1							\n"
"	vmrglh 17,15,28							\n"
"	vmrghh 6,6,26							\n"
"	vmrglh 29,30,14							\n"
"	vmrghh 19,19,1							\n"
"	vmrghh 3,30,14							\n"
"	vmrghh 18,15,28							\n"
"	vmrglh 15,19,6							\n"
"	vmrglh 30,31,2							\n"
"	vmrglh 14,17,29							\n"
"	vmrghh 19,19,6							\n"
"	vmrghh 26,17,29							\n"
"	vmrghh 1,18,3							\n"
"	vmrglh 29,30,14							\n"
"	vmrglh 28,18,3							\n"
"	vmrghh 6,31,2							\n"
"	vsubshs 0,12,29							\n"
"	vmrglh 31,19,1							\n"
"	vmrglh 2,6,26							\n"
"	vmrglh 17,15,28							\n"
"	vmhraddshs 16,8,31,0						\n"
"	vmrghh 3,30,14							\n"
"	vmhraddshs 29,8,29,31						\n"
"	vmhraddshs 9,5,17,2						\n"
"	vmrghh 18,15,28							\n"
"	vmhraddshs 27,4,2,17						\n"
"	vmrghh 0,19,1							\n"
"	vmhraddshs 17,7,3,18						\n"
"	vmrghh 6,6,26							\n"
"	vsubshs 12,12,3							\n"
"	vaddshs 19,0,13							\n"
"	vaddshs 24,16,9							\n"
"	vmhraddshs 3,7,18,12						\n"
"	vsubshs 9,16,9							\n"
"	vaddshs 2,19,6							\n"
"	vsubshs 16,29,27						\n"
"	vaddshs 29,29,27						\n"
"	vsubshs 1,19,6							\n"
"	vaddshs 27,2,17							\n"
"	vsubshs 17,2,17							\n"
"	vaddshs 2,1,3							\n"
"	vsubshs 1,1,3							\n"
"	vsubshs 3,16,9							\n"
"	vaddshs 9,16,9							\n"
"	vaddshs 19,27,29						\n"
"	vmhraddshs 15,10,9,2						\n"
"	vmhraddshs 6,10,3,1						\n"
"	vsrah 19,19,25							\n"
"	vmhraddshs 28,11,3,1						\n"
"	vpkshus 0,19,19							\n"
"	vaddshs 30,17,24						\n"
"	stvx 0,0,9							\n"
"	vsrah 31,15,25							\n"
"	lfd 0,16(1)							\n"
"	vsrah 18,6,25							\n"
"	stfd 0,0(4)							\n"
"	vpkshus 0,31,31							\n"
"	vsubshs 1,17,24							\n"
"	stvx 0,0,9							\n"
"	vsrah 17,30,25							\n"
"	lfd 0,16(1)							\n"
"	vpkshus 0,18,18							\n"
"	vsrah 6,1,25							\n"
"	stvx 0,0,9							\n"
"	vpkshus 1,17,17							\n"
"	vmhraddshs 26,11,9,2						\n"
"	add 4,4,5							\n"
"	vsrah 2,28,25							\n"
"	vpkshus 0,6,6							\n"
"	stfd 0,0(4)							\n"
"	vsubshs 14,27,29						\n"
"	lfd 0,16(1)							\n"
"	vpkshus 13,2,2							\n"
"	stvx 1,0,9							\n"
"	vsrah 29,14,25							\n"
"	add 4,4,5							\n"
"	vsrah 3,26,25							\n"
"	stfd 0,0(4)							\n"
"	vpkshus 12,29,29						\n"
"	lfd 0,16(1)							\n"
"	vpkshus 1,3,3							\n"
"	stvx 0,0,9							\n"
"	add 4,4,5							\n"
"	stfd 0,0(4)							\n"
"	lfd 13,16(1)							\n"
"	stvx 13,0,9							\n"
"	add 4,4,5							\n"
"	stfd 13,0(4)							\n"
"	lfd 0,16(1)							\n"
"	add 4,4,5							\n"
"	stfd 0,0(4)							\n"
"	stvx 1,0,9							\n"
"	add 4,4,5							\n"
"	lfd 0,16(1)							\n"
"	stfd 0,0(4)							\n"
"	stvx 12,0,9							\n"
"	add 4,4,5							\n"
"	lfd 0,16(1)							\n"
"	stfd 0,0(4)							\n"
"	addi 0,1,160							\n"
"	bl _restv24							\n"
"	lwz 0,164(1)							\n"
"	mtlr 0								\n"
"	la 1,160(1)							\n"
"	blr								\n"
".Lfe3:									\n"
"	.size	 idct_block_copy_altivec,.Lfe3-idct_block_copy_altivec	\n"
);

#endif

void idct_mmx_init (void)
{
    extern uint8_t scan_norm[64];
    extern uint8_t scan_alt[64];
    int i, j;

    /* the altivec idct uses a transposed input, so we patch scan tables */
    for (i = 0; i < 64; i++) {
        j = scan_norm[i];
        scan_norm[i] = (j >> 3) | ((j & 7) << 3);
        j = scan_alt[i];
        scan_alt[i] = (j >> 3) | ((j & 7) << 3);
    }
}

#endif
