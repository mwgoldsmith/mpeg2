/*
 * mmx.h
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
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
 * You should have received a copy of the GNU General Public License along
 * with mpeg2dec; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef LIBMPEG2_MMX_H
#define LIBMPEG2_MMX_H

#include "attributes.h"

/*
 * The type of an value that fits in an MMX register (note that long
 * long constant values MUST be suffixed by LL and unsigned long long
 * values by ULL, lest they be truncated by the compiler)
 */

typedef	union {
	long long		q;	/* Quadword (64-bit) value */
	unsigned long long	uq;	/* Unsigned Quadword */
	int			d[2];	/* 2 Doubleword (32-bit) values */
	unsigned int		ud[2];	/* 2 Unsigned Doubleword */
	short			w[4];	/* 4 Word (16-bit) values */
	unsigned short		uw[4];	/* 4 Unsigned Word */
	char			b[8];	/* 8 Byte (8-bit) values */
	unsigned char		ub[8];	/* 8 Unsigned Byte */
	float			s[2];	/* Single-precision (32-bit) value */
} ALIGNED(8) mmx_t;	/* On an 8-byte (64-bit) boundary */


#define	mmx_i2r(op,imm,reg) \
	__asm__ __volatile__ (#op " %0, %%" #reg \
			      : /* nothing */ \
			      : "i" (imm) )

#define	mmx_m2r(op,mem,reg) \
	__asm__ __volatile__ (#op " %0, %%" #reg \
			      : /* nothing */ \
			      : "m" (mem))

#define	mmx_r2m(op,reg,mem) \
	__asm__ __volatile__ (#op " %%" #reg ", %0" \
			      : "=m" (mem) \
			      : /* nothing */ )

#define	mmx_r2r(op,regs,regd) \
	__asm__ __volatile__ (#op " %" #regs ", %" #regd)


#define	emms() __asm__ __volatile__ ("emms")

#define	movd_m2r(var,reg)	mmx_m2r (movd, var, reg)
#define	movd_r2m(reg,var)	mmx_r2m (movd, reg, var)
#define	movd_v2r(var,reg)	__asm__ __volatile__ ("movd %0, %%" #reg \
						      : /* nothing */ \
						      : "rm" (var))
#define	movd_r2v(reg,var)	__asm__ __volatile__ ("movd %%" #reg ", %0" \
						      : "=rm" (var) \
						      : /* nothing */ )

#define	movq_m2r(var,reg)	mmx_m2r (movq, var, reg)
#define	movq_r2m(reg,var)	mmx_r2m (movq, reg, var)
#define	movq_r2r(regs,regd)	mmx_r2r (movq, regs, regd)

#define	packssdw_m2r(var,reg)	mmx_m2r (packssdw, var, reg)
#define	packssdw_r2r(regs,regd) mmx_r2r (packssdw, regs, regd)
#define	packsswb_m2r(var,reg)	mmx_m2r (packsswb, var, reg)
#define	packsswb_r2r(regs,regd) mmx_r2r (packsswb, regs, regd)

#define	packuswb_m2r(var,reg)	mmx_m2r (packuswb, var, reg)
#define	packuswb_r2r(regs,regd) mmx_r2r (packuswb, regs, regd)

#define	paddb_m2r(var,reg)	mmx_m2r (paddb, var, reg)
#define	paddb_r2r(regs,regd)	mmx_r2r (paddb, regs, regd)
#define	paddd_m2r(var,reg)	mmx_m2r (paddd, var, reg)
#define	paddd_r2r(regs,regd)	mmx_r2r (paddd, regs, regd)
#define	paddw_m2r(var,reg)	mmx_m2r (paddw, var, reg)
#define	paddw_r2r(regs,regd)	mmx_r2r (paddw, regs, regd)

#define	paddsb_m2r(var,reg)	mmx_m2r (paddsb, var, reg)
#define	paddsb_r2r(regs,regd)	mmx_r2r (paddsb, regs, regd)
#define	paddsw_m2r(var,reg)	mmx_m2r (paddsw, var, reg)
#define	paddsw_r2r(regs,regd)	mmx_r2r (paddsw, regs, regd)

#define	paddusb_m2r(var,reg)	mmx_m2r (paddusb, var, reg)
#define	paddusb_r2r(regs,regd)	mmx_r2r (paddusb, regs, regd)
#define	paddusw_m2r(var,reg)	mmx_m2r (paddusw, var, reg)
#define	paddusw_r2r(regs,regd)	mmx_r2r (paddusw, regs, regd)

#define	pand_m2r(var,reg)	mmx_m2r (pand, var, reg)
#define	pand_r2r(regs,regd)	mmx_r2r (pand, regs, regd)

#define	pandn_m2r(var,reg)	mmx_m2r (pandn, var, reg)
#define	pandn_r2r(regs,regd)	mmx_r2r (pandn, regs, regd)

#define	pcmpeqb_m2r(var,reg)	mmx_m2r (pcmpeqb, var, reg)
#define	pcmpeqb_r2r(regs,regd)	mmx_r2r (pcmpeqb, regs, regd)
#define	pcmpeqd_m2r(var,reg)	mmx_m2r (pcmpeqd, var, reg)
#define	pcmpeqd_r2r(regs,regd)	mmx_r2r (pcmpeqd, regs, regd)
#define	pcmpeqw_m2r(var,reg)	mmx_m2r (pcmpeqw, var, reg)
#define	pcmpeqw_r2r(regs,regd)	mmx_r2r (pcmpeqw, regs, regd)

#define	pcmpgtb_m2r(var,reg)	mmx_m2r (pcmpgtb, var, reg)
#define	pcmpgtb_r2r(regs,regd)	mmx_r2r (pcmpgtb, regs, regd)
#define	pcmpgtd_m2r(var,reg)	mmx_m2r (pcmpgtd, var, reg)
#define	pcmpgtd_r2r(regs,regd)	mmx_r2r (pcmpgtd, regs, regd)
#define	pcmpgtw_m2r(var,reg)	mmx_m2r (pcmpgtw, var, reg)
#define	pcmpgtw_r2r(regs,regd)	mmx_r2r (pcmpgtw, regs, regd)

#define	pmaddwd_m2r(var,reg)	mmx_m2r (pmaddwd, var, reg)
#define	pmaddwd_r2r(regs,regd)	mmx_r2r (pmaddwd, regs, regd)

#define	pmulhw_m2r(var,reg)	mmx_m2r (pmulhw, var, reg)
#define	pmulhw_r2r(regs,regd)	mmx_r2r (pmulhw, regs, regd)

#define	pmullw_m2r(var,reg)	mmx_m2r (pmullw, var, reg)
#define	pmullw_r2r(regs,regd)	mmx_r2r (pmullw, regs, regd)

#define	por_m2r(var,reg)	mmx_m2r (por, var, reg)
#define	por_r2r(regs,regd)	mmx_r2r (por, regs, regd)

#define	pslld_i2r(imm,reg)	mmx_i2r (pslld, imm, reg)
#define	pslld_m2r(var,reg)	mmx_m2r (pslld, var, reg)
#define	pslld_r2r(regs,regd)	mmx_r2r (pslld, regs, regd)
#define	psllq_i2r(imm,reg)	mmx_i2r (psllq, imm, reg)
#define	psllq_m2r(var,reg)	mmx_m2r (psllq, var, reg)
#define	psllq_r2r(regs,regd)	mmx_r2r (psllq, regs, regd)
#define	psllw_i2r(imm,reg)	mmx_i2r (psllw, imm, reg)
#define	psllw_m2r(var,reg)	mmx_m2r (psllw, var, reg)
#define	psllw_r2r(regs,regd)	mmx_r2r (psllw, regs, regd)

#define	psrad_i2r(imm,reg)	mmx_i2r (psrad, imm, reg)
#define	psrad_m2r(var,reg)	mmx_m2r (psrad, var, reg)
#define	psrad_r2r(regs,regd)	mmx_r2r (psrad, regs, regd)
#define	psraw_i2r(imm,reg)	mmx_i2r (psraw, imm, reg)
#define	psraw_m2r(var,reg)	mmx_m2r (psraw, var, reg)
#define	psraw_r2r(regs,regd)	mmx_r2r (psraw, regs, regd)

#define	psrld_i2r(imm,reg)	mmx_i2r (psrld, imm, reg)
#define	psrld_m2r(var,reg)	mmx_m2r (psrld, var, reg)
#define	psrld_r2r(regs,regd)	mmx_r2r (psrld, regs, regd)
#define	psrlq_i2r(imm,reg)	mmx_i2r (psrlq, imm, reg)
#define	psrlq_m2r(var,reg)	mmx_m2r (psrlq, var, reg)
#define	psrlq_r2r(regs,regd)	mmx_r2r (psrlq, regs, regd)
#define	psrlw_i2r(imm,reg)	mmx_i2r (psrlw, imm, reg)
#define	psrlw_m2r(var,reg)	mmx_m2r (psrlw, var, reg)
#define	psrlw_r2r(regs,regd)	mmx_r2r (psrlw, regs, regd)

#define	psubb_m2r(var,reg)	mmx_m2r (psubb, var, reg)
#define	psubb_r2r(regs,regd)	mmx_r2r (psubb, regs, regd)
#define	psubd_m2r(var,reg)	mmx_m2r (psubd, var, reg)
#define	psubd_r2r(regs,regd)	mmx_r2r (psubd, regs, regd)
#define	psubw_m2r(var,reg)	mmx_m2r (psubw, var, reg)
#define	psubw_r2r(regs,regd)	mmx_r2r (psubw, regs, regd)

#define	psubsb_m2r(var,reg)	mmx_m2r (psubsb, var, reg)
#define	psubsb_r2r(regs,regd)	mmx_r2r (psubsb, regs, regd)
#define	psubsw_m2r(var,reg)	mmx_m2r (psubsw, var, reg)
#define	psubsw_r2r(regs,regd)	mmx_r2r (psubsw, regs, regd)

#define	psubusb_m2r(var,reg)	mmx_m2r (psubusb, var, reg)
#define	psubusb_r2r(regs,regd)	mmx_r2r (psubusb, regs, regd)
#define	psubusw_m2r(var,reg)	mmx_m2r (psubusw, var, reg)
#define	psubusw_r2r(regs,regd)	mmx_r2r (psubusw, regs, regd)

#define	punpckhbw_m2r(var,reg)		mmx_m2r (punpckhbw, var, reg)
#define	punpckhbw_r2r(regs,regd)	mmx_r2r (punpckhbw, regs, regd)
#define	punpckhdq_m2r(var,reg)		mmx_m2r (punpckhdq, var, reg)
#define	punpckhdq_r2r(regs,regd)	mmx_r2r (punpckhdq, regs, regd)
#define	punpckhwd_m2r(var,reg)		mmx_m2r (punpckhwd, var, reg)
#define	punpckhwd_r2r(regs,regd)	mmx_r2r (punpckhwd, regs, regd)

#define	punpcklbw_m2r(var,reg) 		mmx_m2r (punpcklbw, var, reg)
#define	punpcklbw_r2r(regs,regd)	mmx_r2r (punpcklbw, regs, regd)
#define	punpckldq_m2r(var,reg)		mmx_m2r (punpckldq, var, reg)
#define	punpckldq_r2r(regs,regd)	mmx_r2r (punpckldq, regs, regd)
#define	punpcklwd_m2r(var,reg)		mmx_m2r (punpcklwd, var, reg)
#define	punpcklwd_r2r(regs,regd)	mmx_r2r (punpcklwd, regs, regd)

#define	pxor_m2r(var,reg)	mmx_m2r (pxor, var, reg)
#define	pxor_r2r(regs,regd)	mmx_r2r (pxor, regs, regd)


/* 3DNOW extensions */

#define pavgusb_m2r(var,reg)	mmx_m2r (pavgusb, var, reg)
#define pavgusb_r2r(regs,regd)	mmx_r2r (pavgusb, regs, regd)


/* AMD MMX extensions - also available in intel SSE */


#define mmx_m2ri(op,mem,reg,imm) \
	__asm__ __volatile__ (#op " %1, %0, %%" #reg \
			      : /* nothing */ \
			      : "m" (mem), "i" (imm))

#define mmx_r2ri(op,regs,regd,imm) \
	__asm__ __volatile__ (#op " %0, %%" #regs ", %%" #regd \
			      : /* nothing */ \
			      : "i" (imm) )

#define	mmx_fetch(mem,hint) \
	__asm__ __volatile__ ("prefetch" #hint " %0" \
			      : /* nothing */ \
			      : "m" (mem))


#define	maskmovq(regs,maskreg)		mmx_r2ri (maskmovq, regs, maskreg)

#define	movntq_r2m(mmreg,var)		mmx_r2m (movntq, mmreg, var)

#define	pavgb_m2r(var,reg)		mmx_m2r (pavgb, var, reg)
#define	pavgb_r2r(regs,regd)		mmx_r2r (pavgb, regs, regd)
#define	pavgw_m2r(var,reg)		mmx_m2r (pavgw, var, reg)
#define	pavgw_r2r(regs,regd)		mmx_r2r (pavgw, regs, regd)

#define	pextrw_r2r(mmreg,reg,imm)	mmx_r2ri (pextrw, mmreg, reg, imm)

#define	pinsrw_r2r(reg,mmreg,imm)	mmx_r2ri (pinsrw, reg, mmreg, imm)

#define	pmaxsw_m2r(var,reg)		mmx_m2r (pmaxsw, var, reg)
#define	pmaxsw_r2r(regs,regd)		mmx_r2r (pmaxsw, regs, regd)

#define	pmaxub_m2r(var,reg)		mmx_m2r (pmaxub, var, reg)
#define	pmaxub_r2r(regs,regd)		mmx_r2r (pmaxub, regs, regd)

#define	pminsw_m2r(var,reg)		mmx_m2r (pminsw, var, reg)
#define	pminsw_r2r(regs,regd)		mmx_r2r (pminsw, regs, regd)

#define	pminub_m2r(var,reg)		mmx_m2r (pminub, var, reg)
#define	pminub_r2r(regs,regd)		mmx_r2r (pminub, regs, regd)

#define	pmovmskb(mmreg,reg) \
	__asm__ __volatile__ ("movmskps %" #mmreg ", %" #reg)

#define	pmulhuw_m2r(var,reg)		mmx_m2r (pmulhuw, var, reg)
#define	pmulhuw_r2r(regs,regd)		mmx_r2r (pmulhuw, regs, regd)

#define	prefetcht0(mem)			mmx_fetch (mem, t0)
#define	prefetcht1(mem)			mmx_fetch (mem, t1)
#define	prefetcht2(mem)			mmx_fetch (mem, t2)
#define	prefetchnta(mem)		mmx_fetch (mem, nta)

#define	psadbw_m2r(var,reg)		mmx_m2r (psadbw, var, reg)
#define	psadbw_r2r(regs,regd)		mmx_r2r (psadbw, regs, regd)


/* SSE2 */

typedef	union {
	long long		q[2];	/* Quadword (64-bit) value */
	unsigned long long	uq[2];	/* Unsigned Quadword */
	int			d[4];	/* 2 Doubleword (32-bit) values */
	unsigned int		ud[4];	/* 2 Unsigned Doubleword */
	short			w[8];	/* 4 Word (16-bit) values */
	unsigned short		uw[8];	/* 4 Unsigned Word */
	char			b[16];	/* 8 Byte (8-bit) values */
	unsigned char		ub[16];	/* 8 Unsigned Byte */
	float			s[4];	/* Single-precision (32-bit) value */
} ALIGNED(16) sse_t;	/* On an 16-byte (128-bit) boundary */

#define	movdqu_m2r(var,reg)	mmx_m2r (movdqu, var, reg)
#define	movdqu_r2m(reg,var)	mmx_r2m (movdqu, reg, var)
#define	movdqu_r2r(regs,regd)	mmx_r2r (movdqu, regs, regd)
#define	movdqa_m2r(var,reg)	mmx_m2r (movdqa, var, reg)
#define	movdqa_r2m(reg,var)	mmx_r2m (movdqa, reg, var)
#define	movdqa_r2r(regs,regd)	mmx_r2r (movdqa, regs, regd)

#define	pshufd_r2r(regs,regd,imm)	mmx_r2ri(pshufd, regs, regd, imm)

#define	pshufw_m2r(var,reg,imm)		mmx_m2ri(pshufw, var, reg, imm)
#define	pshufw_r2r(regs,regd,imm)	mmx_r2ri(pshufw, regs, regd, imm)

#define	sfence() __asm__ __volatile__ ("sfence\n\t")


#define round(bias) ((int)(((bias)+0.5) * (1<<ROW_SHIFT)))
#define rounder(bias) {round (bias), round (bias)}
#define rounder_sse2(bias) {round (bias), round (bias), round (bias), round (bias)}


#if 0
                      /* C row IDCT - it is just here to document the MMXEXT and MMX versions */
static inline void idct_row(int16_t * row, int offset,
  int16_t * table, int32_t * rounder) {
  int C1, C2, C3, C4, C5, C6, C7;
  int a0, a1, a2, a3, b0, b1, b2, b3;

  row += offset;

  C1 = table[1];
  C2 = table[2];
  C3 = table[3];
  C4 = table[4];
  C5 = table[5];
  C6 = table[6];
  C7 = table[7];

  a0 = C4*row[0] + C2*row[2] + C4*row[4] + C6*row[6] + *rounder;
  a1 = C4*row[0] + C6*row[2] - C4*row[4] - C2*row[6] + *rounder;
  a2 = C4*row[0] - C6*row[2] - C4*row[4] + C2*row[6] + *rounder;
  a3 = C4*row[0] - C2*row[2] + C4*row[4] - C6*row[6] + *rounder;

  b0 = C1*row[1] + C3*row[3] + C5*row[5] + C7*row[7];
  b1 = C3*row[1] - C7*row[3] - C1*row[5] - C5*row[7];
  b2 = C5*row[1] - C1*row[3] + C7*row[5] + C3*row[7];
  b3 = C7*row[1] - C5*row[3] + C3*row[5] - C1*row[7];

  row[0] = (a0 + b0) >> ROW_SHIFT;
  row[1] = (a1 + b1) >> ROW_SHIFT;
  row[2] = (a2 + b2) >> ROW_SHIFT;
  row[3] = (a3 + b3) >> ROW_SHIFT;
  row[4] = (a3 - b3) >> ROW_SHIFT;
  row[5] = (a2 - b2) >> ROW_SHIFT;
  row[6] = (a1 - b1) >> ROW_SHIFT;
  row[7] = (a0 - b0) >> ROW_SHIFT;
}
#endif


/* SSE2 row IDCT */
#define sse2_table(c1,c2,c3,c4,c5,c6,c7) {  c4,  c2,  c4,  c6,   \
					    c4, -c6,  c4, -c2,   \
					    c4,  c6, -c4, -c2,   \
					   -c4,  c2,  c4, -c6,   \
					    c1,  c3,  c3, -c7,   \
					    c5, -c1,  c7, -c5,   \
					    c5,  c7, -c1, -c5,   \
					    c7,  c3,  c3, -c1 }

#define SSE2_IDCT_2ROW(table, row1, row2, round1, round2) do {               \
    /* no scheduling: trust in out of order execution */                     \
    /* based on Intel AP-945 */                                              \
    /* (http://cache-www.intel.com/cd/00/00/01/76/17680_w_idct.pdf) */       \
                                                                             \
    /* input */                      /* 1: row1= x7 x5 x3 x1  x6 x4 x2 x0 */ \
    pshufd_r2r   (row1, xmm1, 0);    /* 1: xmm1= x2 x0 x2 x0  x2 x0 x2 x0 */ \
    pmaddwd_m2r  (table[0], xmm1);   /* 1: xmm1= x2*C + x0*C ...          */ \
    pshufd_r2r   (row1, xmm3, 0xaa); /* 1: xmm3= x3 x1 x3 x1  x3 x1 x3 x1 */ \
    pmaddwd_m2r  (table[2*8], xmm3); /* 1: xmm3= x3*C + x1*C ...          */ \
    pshufd_r2r   (row1, xmm2, 0x55); /* 1: xmm2= x6 x4 x6 x4  x6 x4 x6 x4 */ \
    pshufd_r2r   (row1, row1, 0xff); /* 1: row1= x7 x5 x7 x5  x7 x5 x7 x5 */ \
    pmaddwd_m2r  (table[1*8], xmm2); /* 1: xmm2= x6*C + x4*C ...          */ \
    paddd_m2r    (round1, xmm1);     /* 1: xmm1= x2*C + x0*C + round ...  */ \
    pmaddwd_m2r  (table[3*8], row1); /* 1: row1= x7*C + x5*C ...          */ \
    pshufd_r2r   (row2, xmm5, 0);    /*    2:                             */ \
    pshufd_r2r   (row2, xmm6, 0x55); /*    2:                             */ \
    pmaddwd_m2r  (table[0], xmm5);   /*    2:                             */ \
    paddd_r2r    (xmm2, xmm1);       /* 1: xmm1= a[]                      */ \
    movdqa_r2r   (xmm1, xmm2);       /* 1: xmm2= a[]                      */ \
    pshufd_r2r   (row2, xmm7, 0xaa); /*    2:                             */ \
    pmaddwd_m2r  (table[1*8], xmm6); /*    2:                             */ \
    paddd_r2r    (xmm3, row1);       /* 1: row1= b[]= 7*C+5*C+3*C+1*C ... */ \
    pshufd_r2r   (row2, row2, 0xff); /*    2:                             */ \
    psubd_r2r    (row1, xmm2);       /* 1: xmm2= a[] - b[]                */ \
    pmaddwd_m2r  (table[2*8], xmm7); /*    2:                             */ \
    paddd_r2r    (xmm1, row1);       /* 1: row1= a[] + b[]                */ \
    psrad_i2r    (ROW_SHIFT, xmm2);  /* 1: xmm2= result 4...7             */ \
    paddd_m2r    (round2, xmm5);     /*    2:                             */ \
    pmaddwd_m2r  (table[3*8], row2); /*    2:                             */ \
    paddd_r2r    (xmm6, xmm5);       /*    2:                             */ \
    movdqa_r2r   (xmm5, xmm6);       /*    2:                             */ \
    psrad_i2r    (ROW_SHIFT, row1);  /* 1: row1= result 0...4             */ \
    pshufd_r2r   (xmm2, xmm2, 0x1b); /* 1: [0 1 2 3] -> [3 2 1 0]         */ \
    packssdw_r2r (xmm2, row1);       /* 1: row1= result[]                 */ \
    paddd_r2r    (xmm7, row2);       /*    2:                             */ \
    psubd_r2r    (row2, xmm6);       /*    2:                             */ \
    paddd_r2r    (xmm5, row2);       /*    2:                             */ \
    psrad_i2r    (ROW_SHIFT, xmm6);  /*    2:                             */ \
    psrad_i2r    (ROW_SHIFT, row2);  /*    2:                             */ \
    pshufd_r2r   (xmm6, xmm6, 0x1b); /*    2:                             */ \
    packssdw_r2r (xmm6, row2);       /*    2:                             */ \
} while (0)


/* MMXEXT row IDCT */

#define mmxext_table(c1,c2,c3,c4,c5,c6,c7)	{  c4,  c2, -c4, -c2,	\
						   c4,  c6,  c4,  c6,	\
						   c1,  c3, -c1, -c5,	\
						   c5,  c7,  c3, -c7,	\
						   c4, -c6,  c4, -c6,	\
						  -c4,  c2,  c4, -c2,	\
						   c5, -c1,  c3, -c1,	\
						   c7,  c3,  c7, -c5 }


/* MMX row IDCT */

#define mmx_table(c1,c2,c3,c4,c5,c6,c7)	{  c4,  c2,  c4,  c6,	\
					   c4,  c6, -c4, -c2,	\
					   c1,  c3,  c3, -c7,	\
					   c5,  c7, -c1, -c5,	\
					   c4, -c6,  c4, -c2,	\
					  -c4,  c2,  c4, -c6,	\
					   c5, -c1,  c7, -c5,	\
					   c7,  c3,  c3, -c1 }


#if 0
/* C column IDCT - it is just here to document the MMXEXT and MMX versions */
static inline void idct_col(int16_t * col, int offset) {
  /* multiplication - as implemented on mmx */
#define F(c,x) (((c) * (x)) >> 16)

  /* saturation - it helps us handle torture test cases */
#define S(x) (((x)>32767) ? 32767 : ((x)<-32768) ? -32768 : (x))

  int16_t x0, x1, x2, x3, x4, x5, x6, x7;
  int16_t y0, y1, y2, y3, y4, y5, y6, y7;
  int16_t a0, a1, a2, a3, b0, b1, b2, b3;
  int16_t u04, v04, u26, v26, u17, v17, u35, v35, u12, v12;

  col += offset;

  x0 = col[0 * 8];
  x1 = col[1 * 8];
  x2 = col[2 * 8];
  x3 = col[3 * 8];
  x4 = col[4 * 8];
  x5 = col[5 * 8];
  x6 = col[6 * 8];
  x7 = col[7 * 8];

  u04 = S(x0 + x4);
  v04 = S(x0 - x4);
  u26 = S(F(T2, x6) + x2);
  v26 = S(F(T2, x2) - x6);

  a0 = S(u04 + u26);
  a1 = S(v04 + v26);
  a2 = S(v04 - v26);
  a3 = S(u04 - u26);

  u17 = S(F(T1, x7) + x1);
  v17 = S(F(T1, x1) - x7);
  u35 = S(F(T3, x5) + x3);
  v35 = S(F(T3, x3) - x5);

  b0 = S(u17 + u35);
  b3 = S(v17 - v35);
  u12 = S(u17 - u35);
  v12 = S(v17 + v35);
  u12 = S(2 * F(C4, u12));
  v12 = S(2 * F(C4, v12));
  b1 = S(u12 + v12);
  b2 = S(u12 - v12);

  y0 = S(a0 + b0) >> COL_SHIFT;
  y1 = S(a1 + b1) >> COL_SHIFT;
  y2 = S(a2 + b2) >> COL_SHIFT;
  y3 = S(a3 + b3) >> COL_SHIFT;

  y4 = S(a3 - b3) >> COL_SHIFT;
  y5 = S(a2 - b2) >> COL_SHIFT;
  y6 = S(a1 - b1) >> COL_SHIFT;
  y7 = S(a0 - b0) >> COL_SHIFT;

  col[0 * 8] = y0;
  col[1 * 8] = y1;
  col[2 * 8] = y2;
  col[3 * 8] = y3;
  col[4 * 8] = y4;
  col[5 * 8] = y5;
  col[6 * 8] = y6;
  col[7 * 8] = y7;
}
#endif


#define T1 13036
#define T2 27146
#define T3 43790
#define C4 23170

#define ROW_SHIFT 15
#define COL_SHIFT 6

static ALIGNED_ARRAY(const int32_t, 8) rounder0[] = rounder((1 << (COL_SHIFT - 1)) - 0.5);
static ALIGNED_ARRAY(const int32_t, 8) rounder4[] = rounder(0);
static ALIGNED_ARRAY(const int32_t, 8) rounder1[] = rounder(1.25683487303); /* C1*(C1/C4+C1+C7)/2 */
static ALIGNED_ARRAY(const int32_t, 8)rounder7[] = rounder(-0.25); /* C1*(C7/C4+C7-C1)/2 */
static ALIGNED_ARRAY(const int32_t, 8) rounder2[] = rounder(0.60355339059); /* C2 * (C6+C2)/2 */
static ALIGNED_ARRAY(const int32_t, 8) rounder6[] = rounder(-0.25); /* C2 * (C6-C2)/2 */
static ALIGNED_ARRAY(const int32_t, 8) rounder3[] = rounder(0.087788325588); /* C3*(-C3/C4+C3+C5)/2 */
static ALIGNED_ARRAY(const int32_t, 8) rounder5[] = rounder(-0.441341716183); /* C3*(-C5/C4+C5-C3)/2 */


#define declare_idct(idct,table,idct_row_head,idct_row,idct_row_tail,idct_row_mid)	\
static inline void idct (int16_t * const block)				\
{									\
    static ALIGNED_ARRAY(const int16_t, 16) table04[] = table (22725, 21407, 19266, 16384, 12873,  8867, 4520);		\
    static ALIGNED_ARRAY(const int16_t, 16) table17[] = table (31521, 29692, 26722, 22725, 17855, 12299, 6270);		\
    static ALIGNED_ARRAY(const int16_t, 16) table26[] = table (29692, 27969, 25172, 21407, 16819, 11585, 5906);		\
    static ALIGNED_ARRAY(const int16_t, 16) table35[] =	table (26722, 25172, 22654, 19266, 15137, 10426, 5315);		\
									\
    idct_row_head (block, 0*8, table04);				\
    idct_row (table04, rounder0);					\
    idct_row_mid (block, 0*8, 4*8, table04);				\
    idct_row (table04, rounder4);					\
    idct_row_mid (block, 4*8, 1*8, table17);				\
    idct_row (table17, rounder1);					\
    idct_row_mid (block, 1*8, 7*8, table17);				\
    idct_row (table17, rounder7);					\
    idct_row_mid (block, 7*8, 2*8, table26);				\
    idct_row (table26, rounder2);					\
    idct_row_mid (block, 2*8, 6*8, table26);				\
    idct_row (table26, rounder6);					\
    idct_row_mid (block, 6*8, 3*8, table35);				\
    idct_row (table35, rounder3);					\
    idct_row_mid (block, 3*8, 5*8, table35);				\
    idct_row (table35, rounder5);					\
    idct_row_tail (block, 5*8);						\
									\
    idct_col (block, 0);						\
    idct_col (block, 4);						\
}

declare_idct(mmxext_idct, mmxext_table, mmxext_row_head, mmxext_row, mmxext_row_tail, mmxext_row_mid)

declare_idct(mmx_idct, mmx_table, mmx_row_head, mmx_row, mmx_row_tail, mmx_row_mid)


#define COPY_MMX(offset,r0,r1,r2)	\
do {					\
    movq_m2r (*(block+offset), r0);	\
    dest += stride;			\
    movq_m2r (*(block+offset+4), r1);	\
    movq_r2m (r2, *dest);		\
    packuswb_r2r (r1, r0);		\
} while (0)


#define ADD_SSE2_2ROW(op, block0, block1)\
do {					\
    movq_m2r (*(dest), xmm1);		\
    movq_m2r (*(dest+stride), xmm2);	\
    punpcklbw_r2r (xmm0, xmm1);		\
    punpcklbw_r2r (xmm0, xmm2);		\
    paddsw_##op (block0, xmm1);		\
    paddsw_##op (block1, xmm2);		\
    packuswb_r2r (xmm1, xmm1);		\
    packuswb_r2r (xmm2, xmm2);		\
    movq_r2m (xmm1, *(dest));		\
    movq_r2m (xmm2, *(dest+stride));	\
    dest += 2*stride;			\
} while (0)

#define mm_storel_epi64(a, b) _mm_storel_epi64((__m128i*)(a), b)
#define mm_storeh_epi64(a, b) _mm_storel_epi64((__m128i*)(a), _mm_srli_si128(b, 8))
#define mm_madd_epi16(a, b)   _mm_madd_epi16(a, *(__m128i*)(b))
#define mm_add_epi32(a)       _mm_add_epi32(a, *(__m128i*)M128_round_inv_row)
#define mm_adds_epi16(a, b)   _mm_adds_epi16(a, *(__m128i*)(b))
#define mm_or_si128(a, b)     _mm_or_si128(a, *(__m128i*)(b))

#define ADD_MMX(offset,r1,r2,r3,r4)	\
do {					\
    movq_m2r (*(dest+2*stride), r1);	\
    packuswb_r2r (r4, r3);		\
    movq_r2r (r1, r2);			\
    dest += stride;			\
    movq_r2m (r3, *dest);		\
    punpcklbw_r2r (mm0, r1);		\
    paddsw_m2r (*(block+offset), r1);	\
    punpckhbw_r2r (mm0, r2);		\
    paddsw_m2r (*(block+offset+4), r2);	\
} while (0)


#define CPU_MMXEXT 0
#define CPU_MMX 1

#define dup4(reg)			\
do {					\
    if (cpu != CPU_MMXEXT) {		\
	punpcklwd_r2r (reg, reg);	\
	punpckldq_r2r (reg, reg);	\
    } else				\
	pshufw_r2r (reg, reg, 0x00);	\
} while (0)

#define BITS_INV_ACC  4
#define SHIFT_INV_ROW (16 - BITS_INV_ACC)
#define SHIFT_INV_COL (1 + BITS_INV_ACC)
#define RND_INV_ROW   (1024 * (6 - BITS_INV_ACC))
#define RND_INV_COL   (16 * (BITS_INV_ACC - 3))
#define RND_INV_CORR  (RND_INV_COL - 1)

static ALIGNED_ARRAY(const int16_t, 16) M128_round_inv_row[8] = {
  RND_INV_ROW, 0, RND_INV_ROW, 0, RND_INV_ROW, 0, RND_INV_ROW, 0
};

static ALIGNED_ARRAY(const int16_t, 16) M128_round_inv_col[8] = {
  RND_INV_COL, RND_INV_COL, RND_INV_COL, RND_INV_COL, RND_INV_COL, RND_INV_COL, RND_INV_COL, RND_INV_COL
};

static ALIGNED_ARRAY(const int16_t, 16) M128_round_inv_corr[8] = {
  RND_INV_CORR, RND_INV_CORR, RND_INV_CORR, RND_INV_CORR, RND_INV_CORR, RND_INV_CORR, RND_INV_CORR, RND_INV_CORR
};

static ALIGNED_ARRAY(const int16_t, 16) M128_one_corr[8] = {
  1, 1, 1, 1, 1, 1, 1, 1 };

static ALIGNED_ARRAY(const int16_t, 16) M128_tg_1_16[8] = {
  13036, 13036, 13036, 13036, 13036, 13036, 13036, 13036
}; /* tg * (2<<16) + 0.5 */

static ALIGNED_ARRAY(const int16_t, 16) M128_tg_2_16[8] = {
  27146, 27146, 27146, 27146, 27146, 27146, 27146, 27146
}; /* tg * (2<<16) + 0.5 */

static ALIGNED_ARRAY(const int16_t, 16) M128_tg_3_16[8] = {
  -21746, -21746, -21746, -21746, -21746, -21746, -21746, -21746
}; /* tg * (2<<16) + 0.5 */

static ALIGNED_ARRAY(const int16_t, 16) M128_cos_4_16[8] = {
  -19195, -19195, -19195, -19195, -19195, -19195, -19195, -19195
}; /* cos * (2<<16) + 0.5 */

static ALIGNED_ARRAY(const int16_t, 16) M128_tab_i_04[] = {
  16384, 21407,  16384,   8867,  16384,  -8867, 16384, -21407,
  16384,  8867, -16384, -21407, -16384,  21407, 16384,  -8867,
  22725, 19266,  19266,  -4520,  12873, -22725,  4520, -12873,
  12873,  4520, -22725, -12873,   4520,  19266, 19266, -22725
};
static ALIGNED_ARRAY(const int16_t, 16) M128_tab_i_17[] = {
  22725, 29692,  22725,  12299,  22725, -12299, 22725, -29692,
  22725, 12299, -22725, -29692, -22725,  29692, 22725, -12299,
  31521, 26722,  26722,  -6270,  17855, -31521,  6270, -17855,
  17855,  6270, -31521, -17855,   6270,  26722, 26722, -31521
};
static ALIGNED_ARRAY(const int16_t, 16) M128_tab_i_26[] = {
  21407, 27969,  21407,  11585,  21407, -11585, 21407, -27969,
  21407, 11585, -21407, -27969, -21407,  27969, 21407, -11585,
  29692, 25172,  25172,  -5906,  16819, -29692,  5906, -16819,
  16819,  5906, -29692, -16819,   5906,  25172, 25172, -29692
};
static ALIGNED_ARRAY(const int16_t, 16) M128_tab_i_35[] = {
  19266, 25172,  19266,  10426,  19266, -10426, 19266, -25172,
  19266, 10426, -19266, -25172, -19266,  25172, 19266, -10426,
  26722, 22654,  22654,  -5315,  15137, -26722,  5315, -15137,
  15137,  5315, -26722, -15137,   5315,  22654, 22654, -26722
};

#endif /* LIBMPEG2_MMX_H */
