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

#define vector_s16_t vector signed short
#define vector_u16_t vector unsigned short
#define vector_s8_t vector signed char
#define vector_u8_t vector unsigned char
#define vector_s32_t vector signed int
#define vector_u32_t vector unsigned int

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
    vector_s16_t vx0, vx1, vx2, vx3, vx4, vx5, vx6, vx7;		\
    vector_s16_t vy0, vy1, vy2, vy3, vy4, vy5, vy6, vy7;		\
    vector_s16_t a0, a1, a2, ma2, c4, mc4, zero, bias;			\
    vector_s16_t t0, t1, t2, t3, t4, t5, t6, t7, t8;			\
    vector_u16_t shift;							\
									\
    c4 = vec_splat (constants[0], 0);					\
    a0 = vec_splat (constants[0], 1);					\
    a1 = vec_splat (constants[0], 2);					\
    a2 = vec_splat (constants[0], 3);					\
    mc4 = vec_splat (constants[0], 4);					\
    ma2 = vec_splat (constants[0], 5);					\
    bias = (vector_s16_t)vec_splat ((vector_s32_t)constants[0], 3);	\
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

static vector_s16_t constants[5] = {
    (vector_s16_t)(23170, 13573, 6518, 21895, -23170, -21895, 32, 31),
    (vector_s16_t)(16384, 22725, 21407, 19266, 16384, 19266, 21407, 22725),
    (vector_s16_t)(22725, 31521, 29692, 26722, 22725, 26722, 29692, 31521),
    (vector_s16_t)(21407, 29692, 27969, 25172, 21407, 25172, 27969, 29692),
    (vector_s16_t)(19266, 26722, 25172, 22654, 19266, 22654, 25172, 26722)
};

void idct_block_add_altivec (vector_s16_t * block, uint8_t * dest, int stride)
{
    vector_u8_t tmp;
    vector_s16_t tmp2, tmp3;

    IDCT

#define ADD(dest,src)						\
    *(uint64_t *)&tmp = *(uint64_t *)dest;			\
    tmp2 = (vector_s16_t)vec_mergeh ((vector_u8_t)zero, tmp);	\
    tmp3 = vec_adds (tmp2, src);				\
    tmp = vec_packsu (tmp3, tmp3);				\
    vec_ste ((vector_u32_t)tmp, 0, (unsigned int *)dest);	\
    vec_ste ((vector_u32_t)tmp, 4, (unsigned int *)dest);

    ADD (dest, vx0)	dest += stride;
    ADD (dest, vx1)	dest += stride;
    ADD (dest, vx2)	dest += stride;
    ADD (dest, vx3)	dest += stride;
    ADD (dest, vx4)	dest += stride;
    ADD (dest, vx5)	dest += stride;
    ADD (dest, vx6)	dest += stride;
    ADD (dest, vx7)
}

void idct_block_copy_altivec (vector_s16_t * block, uint8_t * dest, int stride)
{
    vector_u8_t tmp;

    IDCT

#define COPY(dest,src)						\
    tmp = vec_packsu (src, src);				\
    vec_ste ((vector_u32_t)tmp, 0, (unsigned int *)dest);	\
    vec_ste ((vector_u32_t)tmp, 4, (unsigned int *)dest);

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
"	.type	 constants,@object					\n"
"	.size	 constants,80						\n"
"constants:								\n"
"	.long 1518482693						\n"
"	.long 427185543							\n"
"	.long -1518425479						\n"
"	.long 2097183							\n"
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
"	.section	\".text\"					\n"
"	.align 2							\n"
"	.globl idct_block_add_altivec					\n"
"	.type	 idct_block_add_altivec,@function			\n"
"idct_block_add_altivec:						\n"
"	.extern _savev22						\n"
"	.extern _restv22						\n"
"	stwu 1,-192(1)							\n"
"	mflr 0								\n"
"	stw 0,196(1)							\n"
"	addi 0,1,192							\n"
"	bl _savev22							\n"
"	addi 9,3,112							\n"
"	vspltish 25,4							\n"
"	vxor 13,13,13							\n"
"	lvx 1,0,9							\n"
"	lis 10,constants@ha						\n"
"	la 10,constants@l(10)						\n"
"	lvx 5,0,3							\n"
"	addi 9,3,16							\n"
"	lvx 8,0,10							\n"
"	lvx 12,0,9							\n"
"	addi 11,10,32							\n"
"	lvx 6,0,11							\n"
"	addi 8,3,48							\n"
"	vslh 1,1,25							\n"
"	addi 9,3,80							\n"
"	lvx 11,0,8							\n"
"	vslh 5,5,25							\n"
"	lvx 0,0,9							\n"
"	addi 11,10,64							\n"
"	vsplth 3,8,2							\n"
"	lvx 7,0,11							\n"
"	vslh 12,12,25							\n"
"	addi 9,3,96							\n"
"	vmhraddshs 26,1,6,13						\n"
"	addi 8,3,32							\n"
"	vsplth 19,8,5							\n"
"	lvx 1,0,9							\n"
"	vslh 11,11,25							\n"
"	addi 3,3,64							\n"
"	lvx 10,0,8							\n"
"	vslh 0,0,25							\n"
"	addi 9,10,48							\n"
"	vmhraddshs 14,12,6,13						\n"
"	lvx 4,0,9							\n"
"	addi 10,10,16							\n"
"	vmhraddshs 22,0,7,13						\n"
"	lvx 9,0,3							\n"
"	lfd 0,0(4)							\n"
"	vmhraddshs 24,11,7,13						\n"
"	lvx 6,0,10							\n"
"	addi 9,1,16							\n"
"	vsubshs 12,13,26						\n"
"	stfd 0,16(1)							\n"
"	vsplth 18,8,3							\n"
"	vslh 10,10,25							\n"
"	lvx 0,0,9							\n"
"	vsplth 11,8,1							\n"
"	vslh 1,1,25							\n"
"	vsplth 7,8,0							\n"
"	li 11,4								\n"
"	vmhraddshs 15,3,14,12						\n"
"	vsplth 2,8,4							\n"
"	vmhraddshs 23,1,4,13						\n"
"	vspltw 8,8,3							\n"
"	vmhraddshs 27,10,4,13						\n"
"	vmhraddshs 12,19,24,22						\n"
"	vmrghb 4,13,0							\n"
"	vslh 9,9,25							\n"
"	vmhraddshs 10,5,6,13						\n"
"	vspltish 25,6							\n"
"	vmhraddshs 28,18,22,24						\n"
"	vmhraddshs 26,3,26,14						\n"
"	vmhraddshs 9,9,6,13						\n"
"	vmhraddshs 14,11,23,27						\n"
"	vsubshs 0,13,23							\n"
"	vaddshs 23,15,12						\n"
"	vmhraddshs 1,11,27,0						\n"
"	vsubshs 12,15,12						\n"
"	vaddshs 6,10,9							\n"
"	vsubshs 15,26,28						\n"
"	vaddshs 26,26,28						\n"
"	vsubshs 9,10,9							\n"
"	vaddshs 28,6,14							\n"
"	vsubshs 14,6,14							\n"
"	vaddshs 6,9,1							\n"
"	vsubshs 9,9,1							\n"
"	vsubshs 1,15,12							\n"
"	vaddshs 12,15,12						\n"
"	vmhraddshs 17,2,1,9						\n"
"	vmhraddshs 16,7,1,9						\n"
"	vmhraddshs 29,2,12,6						\n"
"	vmhraddshs 31,7,12,6						\n"
"	vsubshs 5,14,23							\n"
"	vaddshs 10,28,26						\n"
"	vaddshs 30,14,23						\n"
"	vsubshs 1,28,26							\n"
"	vmrglh 14,10,5							\n"
"	vmrglh 24,31,17							\n"
"	vmrglh 22,16,29							\n"
"	vmrglh 26,30,1							\n"
"	vmrghh 23,30,1							\n"
"	vmrghh 9,16,29							\n"
"	vmrghh 10,10,5							\n"
"	vmrglh 30,14,22							\n"
"	vmrglh 1,24,26							\n"
"	vmrghh 27,31,17							\n"
"	vmrglh 31,10,9							\n"
"	vmrghh 29,24,26							\n"
"	vmrglh 26,30,1							\n"
"	vmrghh 10,10,9							\n"
"	vmrghh 5,27,23							\n"
"	vsubshs 0,13,26							\n"
"	vmrglh 17,27,23							\n"
"	vmrghh 16,14,22							\n"
"	vmrglh 14,10,5							\n"
"	vmrglh 24,31,17							\n"
"	vmrglh 22,16,29							\n"
"	vmhraddshs 15,3,14,0						\n"
"	vmrghh 23,30,1							\n"
"	vmhraddshs 26,3,26,14						\n"
"	vmhraddshs 12,19,24,22						\n"
"	vmrghh 27,31,17							\n"
"	vmhraddshs 28,18,22,24						\n"
"	vmrghh 0,10,5							\n"
"	vmhraddshs 14,11,23,27						\n"
"	vmrghh 9,16,29							\n"
"	vsubshs 1,13,23							\n"
"	vaddshs 10,0,8							\n"
"	vaddshs 23,15,12						\n"
"	vsubshs 12,15,12						\n"
"	vaddshs 6,10,9							\n"
"	vsubshs 15,26,28						\n"
"	vaddshs 26,26,28						\n"
"	vaddshs 28,6,14							\n"
"	vsubshs 9,10,9							\n"
"	vaddshs 10,28,26						\n"
"	vmhraddshs 1,11,27,1						\n"
"	vsrah 10,10,25							\n"
"	vsubshs 14,6,14							\n"
"	vaddshs 0,4,10							\n"
"	vaddshs 30,14,23						\n"
"	vpkshus 0,0,0							\n"
"	vaddshs 6,9,1							\n"
"	stvx 0,0,9							\n"
"	vsubshs 9,9,1							\n"
"	stvewx 0,0,4							\n"
"	vsubshs 1,15,12							\n"
"	lvx 0,0,9							\n"
"	vaddshs 12,15,12						\n"
"	stvewx 0,11,4							\n"
"	vsubshs 5,14,23							\n"
"	add 4,4,5							\n"
"	vmhraddshs 31,7,12,6						\n"
"	lfd 0,0(4)							\n"
"	vmhraddshs 16,7,1,9						\n"
"	stfd 0,16(1)							\n"
"	vmhraddshs 17,2,1,9						\n"
"	lvx 0,0,9							\n"
"	vsubshs 1,28,26							\n"
"	vsrah 24,30,25							\n"
"	vsrah 14,31,25							\n"
"	vsrah 27,16,25							\n"
"	vmrghb 4,13,0							\n"
"	vsrah 26,1,25							\n"
"	vsrah 9,5,25							\n"
"	vaddshs 0,4,14							\n"
"	vsrah 22,17,25							\n"
"	vpkshus 0,0,0							\n"
"	vmhraddshs 29,2,12,6						\n"
"	stvx 0,0,9							\n"
"	stvewx 0,0,4							\n"
"	lvx 0,0,9							\n"
"	stvewx 0,11,4							\n"
"	add 4,4,5							\n"
"	vsrah 23,29,25							\n"
"	lfd 0,0(4)							\n"
"	stfd 0,16(1)							\n"
"	lvx 0,0,9							\n"
"	vmrghb 4,13,0							\n"
"	vaddshs 0,4,27							\n"
"	vpkshus 1,0,0							\n"
"	stvx 1,0,9							\n"
"	stvewx 1,0,4							\n"
"	lvx 0,0,9							\n"
"	stvewx 0,11,4							\n"
"	add 4,4,5							\n"
"	lfd 0,0(4)							\n"
"	stfd 0,16(1)							\n"
"	lvx 0,0,9							\n"
"	vmrghb 4,13,0							\n"
"	vaddshs 0,4,24							\n"
"	vpkshus 1,0,0							\n"
"	stvx 1,0,9							\n"
"	stvewx 1,0,4							\n"
"	lvx 0,0,9							\n"
"	stvewx 0,11,4							\n"
"	add 4,4,5							\n"
"	lfd 0,0(4)							\n"
"	stfd 0,16(1)							\n"
"	lvx 0,0,9							\n"
"	vmrghb 4,13,0							\n"
"	vaddshs 0,4,9							\n"
"	vpkshus 1,0,0							\n"
"	stvx 1,0,9							\n"
"	stvewx 1,0,4							\n"
"	lvx 0,0,9							\n"
"	stvewx 0,11,4							\n"
"	add 4,4,5							\n"
"	lfd 0,0(4)							\n"
"	stfd 0,16(1)							\n"
"	lvx 0,0,9							\n"
"	vmrghb 4,13,0							\n"
"	vaddshs 0,4,22							\n"
"	vpkshus 1,0,0							\n"
"	stvx 1,0,9							\n"
"	stvewx 1,0,4							\n"
"	lvx 0,0,9							\n"
"	stvewx 0,11,4							\n"
"	add 4,4,5							\n"
"	lfd 0,0(4)							\n"
"	stfd 0,16(1)							\n"
"	lvx 0,0,9							\n"
"	vmrghb 4,13,0							\n"
"	vaddshs 0,4,23							\n"
"	vpkshus 1,0,0							\n"
"	stvx 1,0,9							\n"
"	stvewx 1,0,4							\n"
"	lvx 0,0,9							\n"
"	stvewx 0,11,4							\n"
"	add 4,4,5							\n"
"	lfd 0,0(4)							\n"
"	stfd 0,16(1)							\n"
"	lvx 0,0,9							\n"
"	vmrghb 4,13,0							\n"
"	vaddshs 0,4,26							\n"
"	vpkshus 1,0,0							\n"
"	stvx 1,0,9							\n"
"	stvewx 1,0,4							\n"
"	lvx 0,0,9							\n"
"	stvewx 0,11,4							\n"
"	addi 0,1,192							\n"
"	bl _restv22							\n"
"	lwz 0,196(1)							\n"
"	mtlr 0								\n"
"	la 1,192(1)							\n"
"	blr								\n"
".Lfe1_:								\n"
"	.size	 idct_block_add_altivec,.Lfe1_-idct_block_add_altivec	\n"
"	.align 2							\n"
"	.globl idct_block_copy_altivec					\n"
"	.type	 idct_block_copy_altivec,@function			\n"
"idct_block_copy_altivec:						\n"
"	.extern _savev25						\n"
"	.extern _restv25						\n"
"	stwu 1,-128(1)							\n"
"	mflr 0								\n"
"	stw 0,132(1)							\n"
"	addi 0,1,128							\n"
"	bl _savev25							\n"
"	addi 9,3,112							\n"
"	vspltish 25,4							\n"
"	vxor 13,13,13							\n"
"	lis 10,constants@ha						\n"
"	lvx 1,0,9							\n"
"	la 10,constants@l(10)						\n"
"	lvx 5,0,3							\n"
"	addi 9,3,16							\n"
"	lvx 8,0,10							\n"
"	addi 11,10,32							\n"
"	lvx 12,0,9							\n"
"	lvx 6,0,11							\n"
"	addi 8,3,48							\n"
"	vslh 1,1,25							\n"
"	addi 9,3,80							\n"
"	lvx 11,0,8							\n"
"	vslh 5,5,25							\n"
"	lvx 0,0,9							\n"
"	addi 11,10,64							\n"
"	vsplth 3,8,2							\n"
"	lvx 7,0,11							\n"
"	addi 9,3,96							\n"
"	vslh 12,12,25							\n"
"	vmhraddshs 27,1,6,13						\n"
"	addi 8,3,32							\n"
"	vsplth 2,8,5							\n"
"	lvx 1,0,9							\n"
"	vslh 11,11,25							\n"
"	addi 3,3,64							\n"
"	lvx 9,0,8							\n"
"	addi 9,10,48							\n"
"	vslh 0,0,25							\n"
"	lvx 4,0,9							\n"
"	vmhraddshs 31,12,6,13						\n"
"	addi 10,10,16							\n"
"	vmhraddshs 30,0,7,13						\n"
"	lvx 10,0,3							\n"
"	vsplth 19,8,3							\n"
"	vmhraddshs 15,11,7,13						\n"
"	lvx 12,0,10							\n"
"	vsplth 6,8,4							\n"
"	vslh 1,1,25							\n"
"	vsplth 11,8,1							\n"
"	li 9,4								\n"
"	vslh 9,9,25							\n"
"	vsplth 7,8,0							\n"
"	vmhraddshs 18,1,4,13						\n"
"	vspltw 8,8,3							\n"
"	vsubshs 0,13,27							\n"
"	vmhraddshs 1,9,4,13						\n"
"	vmhraddshs 17,3,31,0						\n"
"	vmhraddshs 4,2,15,30						\n"
"	vslh 10,10,25							\n"
"	vmhraddshs 9,5,12,13						\n"
"	vspltish 25,6							\n"
"	vmhraddshs 5,10,12,13						\n"
"	vmhraddshs 28,19,30,15						\n"
"	vmhraddshs 27,3,27,31						\n"
"	vsubshs 0,13,18							\n"
"	vmhraddshs 18,11,18,1						\n"
"	vaddshs 30,17,4							\n"
"	vmhraddshs 12,11,1,0						\n"
"	vsubshs 4,17,4							\n"
"	vaddshs 10,9,5							\n"
"	vsubshs 17,27,28						\n"
"	vaddshs 27,27,28						\n"
"	vsubshs 1,9,5							\n"
"	vaddshs 28,10,18						\n"
"	vsubshs 18,10,18						\n"
"	vaddshs 10,1,12							\n"
"	vsubshs 1,1,12							\n"
"	vsubshs 12,17,4							\n"
"	vaddshs 4,17,4							\n"
"	vmhraddshs 5,7,12,1						\n"
"	vmhraddshs 26,6,4,10						\n"
"	vmhraddshs 29,6,12,1						\n"
"	vmhraddshs 14,7,4,10						\n"
"	vsubshs 12,18,30						\n"
"	vaddshs 9,28,27							\n"
"	vaddshs 16,18,30						\n"
"	vsubshs 10,28,27						\n"
"	vmrglh 31,9,12							\n"
"	vmrglh 30,5,26							\n"
"	vmrglh 15,14,29							\n"
"	vmrghh 5,5,26							\n"
"	vmrglh 27,16,10							\n"
"	vmrghh 9,9,12							\n"
"	vmrghh 18,16,10							\n"
"	vmrghh 1,14,29							\n"
"	vmrglh 14,9,5							\n"
"	vmrglh 16,31,30							\n"
"	vmrglh 10,15,27							\n"
"	vmrghh 9,9,5							\n"
"	vmrghh 26,15,27							\n"
"	vmrglh 27,16,10							\n"
"	vmrghh 12,1,18							\n"
"	vmrglh 29,1,18							\n"
"	vsubshs 0,13,27							\n"
"	vmrghh 5,31,30							\n"
"	vmrglh 31,9,12							\n"
"	vmrglh 30,5,26							\n"
"	vmrglh 15,14,29							\n"
"	vmhraddshs 17,3,31,0						\n"
"	vmrghh 18,16,10							\n"
"	vmhraddshs 27,3,27,31						\n"
"	vmhraddshs 4,2,15,30						\n"
"	vmrghh 1,14,29							\n"
"	vmhraddshs 28,19,30,15						\n"
"	vmrghh 0,9,12							\n"
"	vsubshs 13,13,18						\n"
"	vmrghh 5,5,26							\n"
"	vmhraddshs 18,11,18,1						\n"
"	vaddshs 9,0,8							\n"
"	vaddshs 30,17,4							\n"
"	vmhraddshs 12,11,1,13						\n"
"	vsubshs 4,17,4							\n"
"	vaddshs 10,9,5							\n"
"	vsubshs 17,27,28						\n"
"	vaddshs 27,27,28						\n"
"	vsubshs 1,9,5							\n"
"	vaddshs 28,10,18						\n"
"	vsubshs 18,10,18						\n"
"	vaddshs 10,1,12							\n"
"	vsubshs 1,1,12							\n"
"	vsubshs 12,17,4							\n"
"	vaddshs 4,17,4							\n"
"	vaddshs 9,28,27							\n"
"	vmhraddshs 14,7,4,10						\n"
"	vsrah 9,9,25							\n"
"	vmhraddshs 5,7,12,1						\n"
"	vpkshus 0,9,9							\n"
"	vmhraddshs 29,6,12,1						\n"
"	stvewx 0,0,4							\n"
"	vaddshs 16,18,30						\n"
"	vsrah 31,14,25							\n"
"	stvewx 0,9,4							\n"
"	add 4,4,5							\n"
"	vsrah 15,16,25							\n"
"	vpkshus 0,31,31							\n"
"	vsrah 1,5,25							\n"
"	stvewx 0,0,4							\n"
"	vsubshs 12,18,30						\n"
"	stvewx 0,9,4							\n"
"	vmhraddshs 26,6,4,10						\n"
"	vpkshus 0,1,1							\n"
"	add 4,4,5							\n"
"	vsrah 5,12,25							\n"
"	stvewx 0,0,4							\n"
"	vsrah 30,29,25							\n"
"	stvewx 0,9,4							\n"
"	vsubshs 10,28,27						\n"
"	vpkshus 0,15,15							\n"
"	add 4,4,5							\n"
"	stvewx 0,0,4							\n"
"	vsrah 18,26,25							\n"
"	stvewx 0,9,4							\n"
"	vsrah 27,10,25							\n"
"	vpkshus 0,5,5							\n"
"	add 4,4,5							\n"
"	stvewx 0,0,4							\n"
"	stvewx 0,9,4							\n"
"	vpkshus 0,30,30							\n"
"	add 4,4,5							\n"
"	stvewx 0,0,4							\n"
"	stvewx 0,9,4							\n"
"	vpkshus 0,18,18							\n"
"	add 4,4,5							\n"
"	stvewx 0,0,4							\n"
"	stvewx 0,9,4							\n"
"	add 4,4,5							\n"
"	vpkshus 0,27,27							\n"
"	stvewx 0,0,4							\n"
"	stvewx 0,9,4							\n"
"	addi 0,1,128							\n"
"	bl _restv25							\n"
"	lwz 0,132(1)							\n"
"	mtlr 0								\n"
"	la 1,128(1)							\n"
"	blr								\n"
".Lfe2_:								\n"
"	.size	 idct_block_copy_altivec,.Lfe2_-idct_block_copy_altivec	\n"
);

void idct_altivec_init (void)
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

#endif
