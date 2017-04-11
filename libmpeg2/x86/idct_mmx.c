/*
 * idct_mmx.c
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

#include "config.h"
#include <emmintrin.h>
#include <xmmintrin.h>
#include <inttypes.h>
#include <xmmintrin.h>
#include <emmintrin.h>
#include <immintrin.h>
#include <mmintrin.h>

#if defined(MSVC_ASM) && (defined(ARCH_X86) || defined(ARCH_X86_64))

#include <inttypes.h>

#include "mpeg2.h"
#include "attributes.h"
#include "mpeg2_internal.h"

#include "mmx.h"

static inline void mmxext_row_head(int16_t * const row, const int offset, const int16_t * const table) {
  __m64 mm2;
  movq_m2r (*(row+offset), mm2); /* mm2 = x6 x4 x2 x0 */

  movq_m2r (*(row+offset+4), mm5); /* mm5 = x7 x5 x3 x1 */
  movq_r2r (mm2, mm0); /* mm0 = x6 x4 x2 x0 */

  movq_m2r (*table, mm3); /* mm3 = -C2 -C4 C2 C4 */
  movq_r2r (mm5, mm6); /* mm6 = x7 x5 x3 x1 */

  movq_m2r (*(table+4), mm4); /* mm4 = C6 C4 C6 C4 */
  pmaddwd_r2r (mm0, mm3); /* mm3 = -C4*x4-C2*x6 C4*x0+C2*x2 */

  mm2 = _mm_shuffle_pi16(mm2, 0x4e);
  pshufw_r2r (mm2, mm2, 0x4e); /* mm2 = x2 x0 x6 x4 */
}

static inline void mmxext_row(const int16_t * const table, const int32_t * const rounder) {
  movq_m2r (*(table+8), mm1); /* mm1 = -C5 -C1 C3 C1 */
  pmaddwd_r2r (mm2, mm4); /* mm4 = C4*x0+C6*x2 C4*x4+C6*x6 */

  pmaddwd_m2r (*(table+16), mm0); /* mm0 = C4*x4-C6*x6 C4*x0-C6*x2 */
  pshufw_r2r (mm6, mm6, 0x4e); /* mm6 = x3 x1 x7 x5 */

  movq_m2r (*(table+12), mm7); /* mm7 = -C7 C3 C7 C5 */
  pmaddwd_r2r (mm5, mm1); /* mm1 = -C1*x5-C5*x7 C1*x1+C3*x3 */

  paddd_m2r (*rounder, mm3); /* mm3 += rounder */
  pmaddwd_r2r (mm6, mm7); /* mm7 = C3*x1-C7*x3 C5*x5+C7*x7 */

  pmaddwd_m2r (*(table+20), mm2); /* mm2 = C4*x0-C2*x2 -C4*x4+C2*x6 */
  paddd_r2r (mm4, mm3); /* mm3 = a1 a0 + rounder */

  pmaddwd_m2r (*(table+24), mm5); /* mm5 = C3*x5-C1*x7 C5*x1-C1*x3 */
  movq_r2r (mm3, mm4); /* mm4 = a1 a0 + rounder */

  pmaddwd_m2r (*(table+28), mm6); /* mm6 = C7*x1-C5*x3 C7*x5+C3*x7 */
  paddd_r2r (mm7, mm1); /* mm1 = b1 b0 */

  paddd_m2r (*rounder, mm0); /* mm0 += rounder */
  psubd_r2r (mm1, mm3); /* mm3 = a1-b1 a0-b0 + rounder */

  psrad_i2r (ROW_SHIFT, mm3); /* mm3 = y6 y7 */
  paddd_r2r (mm4, mm1); /* mm1 = a1+b1 a0+b0 + rounder */

  paddd_r2r (mm2, mm0); /* mm0 = a3 a2 + rounder */
  psrad_i2r (ROW_SHIFT, mm1); /* mm1 = y1 y0 */

  paddd_r2r (mm6, mm5); /* mm5 = b3 b2 */
  movq_r2r (mm0, mm4); /* mm4 = a3 a2 + rounder */

  paddd_r2r (mm5, mm0); /* mm0 = a3+b3 a2+b2 + rounder */
  psubd_r2r (mm5, mm4); /* mm4 = a3-b3 a2-b2 + rounder */
}

static inline void mmxext_row_tail(int16_t * const row, const int store) {
  psrad_i2r (ROW_SHIFT, mm0); /* mm0 = y3 y2 */

  psrad_i2r (ROW_SHIFT, mm4); /* mm4 = y4 y5 */

  packssdw_r2r (mm0, mm1); /* mm1 = y3 y2 y1 y0 */

  packssdw_r2r (mm3, mm4); /* mm4 = y6 y7 y4 y5 */

  movq_r2m (mm1, *(row+store)); /* save y3 y2 y1 y0 */
  pshufw_r2r (mm4, mm4, 0xb1); /* mm4 = y7 y6 y5 y4 */

  /* slot */

  movq_r2m (mm4, *(row+store+4)); /* save y7 y6 y5 y4 */
}

static inline void mmxext_row_mid(int16_t * const row, const int store, const int offset, const int16_t * const table) {
  movq_m2r (*(row+offset), mm2); /* mm2 = x6 x4 x2 x0 */
  psrad_i2r (ROW_SHIFT, mm0); /* mm0 = y3 y2 */

  movq_m2r (*(row+offset+4), mm5); /* mm5 = x7 x5 x3 x1 */
  psrad_i2r (ROW_SHIFT, mm4); /* mm4 = y4 y5 */

  packssdw_r2r (mm0, mm1); /* mm1 = y3 y2 y1 y0 */
  movq_r2r (mm5, mm6); /* mm6 = x7 x5 x3 x1 */

  packssdw_r2r (mm3, mm4); /* mm4 = y6 y7 y4 y5 */
  movq_r2r (mm2, mm0); /* mm0 = x6 x4 x2 x0 */

  movq_r2m (mm1, *(row+store)); /* save y3 y2 y1 y0 */
  pshufw_r2r (mm4, mm4, 0xb1); /* mm4 = y7 y6 y5 y4 */

  movq_m2r (*table, mm3); /* mm3 = -C2 -C4 C2 C4 */
  movq_r2m (mm4, *(row+store+4)); /* save y7 y6 y5 y4 */

  pmaddwd_r2r (mm0, mm3); /* mm3 = -C4*x4-C2*x6 C4*x0+C2*x2 */

  movq_m2r (*(table+4), mm4); /* mm4 = C6 C4 C6 C4 */
  pshufw_r2r (mm2, mm2, 0x4e); /* mm2 = x2 x0 x6 x4 */
}

static inline void mmx_row_head(int16_t * const row, const int offset, const int16_t * const table) {
  movq_m2r (*(row+offset), mm2); /* mm2 = x6 x4 x2 x0 */

  movq_m2r (*(row+offset+4), mm5); /* mm5 = x7 x5 x3 x1 */
  movq_r2r (mm2, mm0); /* mm0 = x6 x4 x2 x0 */

  movq_m2r (*table, mm3); /* mm3 = C6 C4 C2 C4 */
  movq_r2r (mm5, mm6); /* mm6 = x7 x5 x3 x1 */

  punpckldq_r2r (mm0, mm0); /* mm0 = x2 x0 x2 x0 */

  movq_m2r (*(table+4), mm4); /* mm4 = -C2 -C4 C6 C4 */
  pmaddwd_r2r (mm0, mm3); /* mm3 = C4*x0+C6*x2 C4*x0+C2*x2 */

  movq_m2r (*(table+8), mm1); /* mm1 = -C7 C3 C3 C1 */
  punpckhdq_r2r (mm2, mm2); /* mm2 = x6 x4 x6 x4 */
}

static inline void mmx_row(const int16_t * const table, const int32_t * const rounder) {
  pmaddwd_r2r (mm2, mm4); /* mm4 = -C4*x4-C2*x6 C4*x4+C6*x6 */
  punpckldq_r2r (mm5, mm5); /* mm5 = x3 x1 x3 x1 */

  pmaddwd_m2r (*(table+16), mm0); /* mm0 = C4*x0-C2*x2 C4*x0-C6*x2 */
  punpckhdq_r2r (mm6, mm6); /* mm6 = x7 x5 x7 x5 */

  movq_m2r (*(table+12), mm7); /* mm7 = -C5 -C1 C7 C5 */
  pmaddwd_r2r (mm5, mm1); /* mm1 = C3*x1-C7*x3 C1*x1+C3*x3 */

  paddd_m2r (*rounder, mm3); /* mm3 += rounder */
  pmaddwd_r2r (mm6, mm7); /* mm7 = -C1*x5-C5*x7 C5*x5+C7*x7 */

  pmaddwd_m2r (*(table+20), mm2); /* mm2 = C4*x4-C6*x6 -C4*x4+C2*x6 */
  paddd_r2r (mm4, mm3); /* mm3 = a1 a0 + rounder */

  pmaddwd_m2r (*(table+24), mm5); /* mm5 = C7*x1-C5*x3 C5*x1-C1*x3 */
  movq_r2r (mm3, mm4); /* mm4 = a1 a0 + rounder */

  pmaddwd_m2r (*(table+28), mm6); /* mm6 = C3*x5-C1*x7 C7*x5+C3*x7 */
  paddd_r2r (mm7, mm1); /* mm1 = b1 b0 */

  paddd_m2r (*rounder, mm0); /* mm0 += rounder */
  psubd_r2r (mm1, mm3); /* mm3 = a1-b1 a0-b0 + rounder */

  psrad_i2r (ROW_SHIFT, mm3); /* mm3 = y6 y7 */
  paddd_r2r (mm4, mm1); /* mm1 = a1+b1 a0+b0 + rounder */

  paddd_r2r (mm2, mm0); /* mm0 = a3 a2 + rounder */
  psrad_i2r (ROW_SHIFT, mm1); /* mm1 = y1 y0 */

  paddd_r2r (mm6, mm5); /* mm5 = b3 b2 */
  movq_r2r (mm0, mm7); /* mm7 = a3 a2 + rounder */

  paddd_r2r (mm5, mm0); /* mm0 = a3+b3 a2+b2 + rounder */
  psubd_r2r (mm5, mm7); /* mm7 = a3-b3 a2-b2 + rounder */
}

static inline void mmx_row_tail(int16_t * const row, const int store) {
  psrad_i2r (ROW_SHIFT, mm0); /* mm0 = y3 y2 */

  psrad_i2r (ROW_SHIFT, mm7); /* mm7 = y4 y5 */

  packssdw_r2r (mm0, mm1); /* mm1 = y3 y2 y1 y0 */

  packssdw_r2r (mm3, mm7); /* mm7 = y6 y7 y4 y5 */

  movq_r2m (mm1, *(row+store)); /* save y3 y2 y1 y0 */
  movq_r2r (mm7, mm4); /* mm4 = y6 y7 y4 y5 */

  pslld_i2r (16, mm7); /* mm7 = y7 0 y5 0 */

  psrld_i2r (16, mm4); /* mm4 = 0 y6 0 y4 */

  por_r2r (mm4, mm7); /* mm7 = y7 y6 y5 y4 */

  /* slot */

  movq_r2m (mm7, *(row+store+4)); /* save y7 y6 y5 y4 */
}

static inline void mmx_row_mid(int16_t * const row, const int store, const int offset, const int16_t * const table) {
  movq_m2r (*(row+offset), mm2); /* mm2 = x6 x4 x2 x0 */
  psrad_i2r (ROW_SHIFT, mm0); /* mm0 = y3 y2 */

  movq_m2r (*(row+offset+4), mm5); /* mm5 = x7 x5 x3 x1 */
  psrad_i2r (ROW_SHIFT, mm7); /* mm7 = y4 y5 */

  packssdw_r2r (mm0, mm1); /* mm1 = y3 y2 y1 y0 */
  movq_r2r (mm5, mm6); /* mm6 = x7 x5 x3 x1 */

  packssdw_r2r (mm3, mm7); /* mm7 = y6 y7 y4 y5 */
  movq_r2r (mm2, mm0); /* mm0 = x6 x4 x2 x0 */

  movq_r2m (mm1, *(row+store)); /* save y3 y2 y1 y0 */
  movq_r2r (mm7, mm1); /* mm1 = y6 y7 y4 y5 */

  punpckldq_r2r (mm0, mm0); /* mm0 = x2 x0 x2 x0 */
  psrld_i2r (16, mm7); /* mm7 = 0 y6 0 y4 */

  movq_m2r (*table, mm3); /* mm3 = C6 C4 C2 C4 */
  pslld_i2r (16, mm1); /* mm1 = y7 0 y5 0 */

  movq_m2r (*(table+4), mm4); /* mm4 = -C2 -C4 C6 C4 */
  por_r2r (mm1, mm7); /* mm7 = y7 y6 y5 y4 */

  movq_m2r (*(table+8), mm1); /* mm1 = -C7 C3 C3 C1 */
  punpckhdq_r2r (mm2, mm2); /* mm2 = x6 x4 x6 x4 */

  movq_r2m (mm7, *(row+store+4)); /* save y7 y6 y5 y4 */
  pmaddwd_r2r (mm0, mm3); /* mm3 = C4*x0+C6*x2 C4*x0+C2*x2 */
}


/* SSE2 column IDCT */
static inline void sse2_idct_col(int16_t * const col) {
#ifdef _MSC_VER

#else
  /* Almost identical to mmxext version:  */
  /* just do both 4x8 columns in parallel */

  static ALIGNED_ARRAY(const short, 16) t1_vector[]  = {T1,T1,T1,T1,T1,T1,T1,T1};
  static ALIGNED_ARRAY(const short, 16) t2_vector[]  = {T2,T2,T2,T2,T2,T2,T2,T2};
  static ALIGNED_ARRAY(const short, 16) t3_vector[]  = {T3,T3,T3,T3,T3,T3,T3,T3};
  static ALIGNED_ARRAY(const short, 16) c4_vector[]  = {C4,C4,C4,C4,C4,C4,C4,C4};

  #if defined(__x86_64__)
    /* INPUT: block in xmm8 ... xmm15 */
    movdqa_m2r (*t1_vector, xmm0);	/* xmm0  = T1 */
    movdqa_r2r (xmm9, xmm1);		/* xmm1  = x1 */

    movdqa_r2r (xmm0, xmm2);		/* xmm2  = T1 */
    pmulhw_r2r (xmm1, xmm0);		/* xmm0  = T1*x1 */

    movdqa_m2r (*t3_vector, xmm5);	/* xmm5  = T3 */
    pmulhw_r2r (xmm15, xmm2);		/* xmm2  = T1*x7 */

    movdqa_r2r (xmm5, xmm7);		/* xmm7  = T3-1 */
    psubsw_r2r (xmm15, xmm0);		/* xmm0  = v17 */

    movdqa_m2r (*t2_vector, xmm9);	/* xmm9  = T2 */
    pmulhw_r2r (xmm11, xmm5);		/* xmm5  = (T3-1)*x3 */

    paddsw_r2r (xmm2, xmm1);		/* xmm1  = u17 */
    pmulhw_r2r (xmm13, xmm7);		/* xmm7  = (T3-1)*x5 */

    movdqa_r2r (xmm9, xmm2);		/* xmm2  = T2 */
    paddsw_r2r (xmm11, xmm5);		/* xmm5  = T3*x3 */

    pmulhw_r2r (xmm10, xmm9);   	/* xmm9  = T2*x2 */
    paddsw_r2r (xmm13, xmm7);		/* xmm7  = T3*x5 */

    psubsw_r2r (xmm13, xmm5);		/* xmm5  = v35 */
    paddsw_r2r (xmm11, xmm7);		/* xmm7  = u35 */

    movdqa_r2r (xmm0, xmm6);		/* xmm6  = v17 */
    pmulhw_r2r (xmm14, xmm2);		/* xmm2  = T2*x6 */

    psubsw_r2r (xmm5, xmm0);		/* xmm0  = b3 */
    psubsw_r2r (xmm14, xmm9);		/* xmm9  = v26 */

    paddsw_r2r (xmm6, xmm5);		/* xmm5  = v12 */
    movdqa_r2r (xmm0, xmm11);		/* xmm11 = b3 */

    movdqa_r2r (xmm1, xmm6);		/* xmm6  = u17 */
    paddsw_r2r (xmm10, xmm2);		/* xmm2  = u26 */

    paddsw_r2r (xmm7, xmm6);		/* xmm6  = b0 */
    psubsw_r2r (xmm7, xmm1);		/* xmm1  = u12 */

    movdqa_r2r (xmm1, xmm7);		/* xmm7  = u12 */
    paddsw_r2r (xmm5, xmm1);		/* xmm1  = u12+v12 */

    movdqa_m2r (*c4_vector, xmm0);	/* xmm0  = C4/2 */
    psubsw_r2r (xmm5, xmm7);		/* xmm7  = u12-v12 */

    movdqa_r2r (xmm6, xmm4);		/* xmm4  = b0 */
    pmulhw_r2r (xmm0, xmm1);		/* xmm1  = b1/2 */

    movdqa_r2r (xmm9, xmm6);		/* xmm6  = v26 */
    pmulhw_r2r (xmm0, xmm7);		/* xmm7  = b2/2 */

    movdqa_r2r (xmm8, xmm10);		/* xmm10 = x0 */
    movdqa_r2r (xmm8, xmm0);		/* xmm0  = x0 */

    psubsw_r2r (xmm12, xmm10);		/* xmm10 = v04 */
    paddsw_r2r (xmm12, xmm0);		/* xmm0  = u04 */

    paddsw_r2r (xmm10, xmm9);		/* xmm9  = a1 */
    movdqa_r2r (xmm0, xmm8);		/* xmm8  = u04 */

    psubsw_r2r (xmm6, xmm10);		/* xmm10 = a2 */
    paddsw_r2r (xmm2, xmm8);		/* xmm5  = a0 */

    paddsw_r2r (xmm1, xmm1);		/* xmm1  = b1 */
    psubsw_r2r (xmm2, xmm0);		/* xmm0  = a3 */

    paddsw_r2r (xmm7, xmm7);		/* xmm7  = b2 */
    movdqa_r2r (xmm10, xmm13);		/* xmm13 = a2 */

    movdqa_r2r (xmm9, xmm14);		/* xmm14 = a1 */
    paddsw_r2r (xmm7, xmm10);		/* xmm10 = a2+b2 */

    psraw_i2r (COL_SHIFT,xmm10);	/* xmm10 = y2 */
    paddsw_r2r (xmm1, xmm9);		/* xmm9  = a1+b1 */

    psraw_i2r (COL_SHIFT, xmm9);	/* xmm9  = y1 */
    psubsw_r2r (xmm1, xmm14);		/* xmm14 = a1-b1 */

    psubsw_r2r (xmm7, xmm13);		/* xmm13 = a2-b2 */
    psraw_i2r (COL_SHIFT,xmm14);	/* xmm14 = y6 */

    movdqa_r2r (xmm8, xmm15);		/* xmm15 = a0 */
    psraw_i2r (COL_SHIFT,xmm13);	/* xmm13 = y5 */

    paddsw_r2r (xmm4, xmm8);		/* xmm8  = a0+b0 */
    psubsw_r2r (xmm4, xmm15);		/* xmm15 = a0-b0 */

    psraw_i2r (COL_SHIFT, xmm8);	/* xmm8  = y0 */
    movdqa_r2r (xmm0, xmm12);		/* xmm12 = a3 */

    psubsw_r2r (xmm11, xmm12);		/* xmm12 = a3-b3 */
    psraw_i2r (COL_SHIFT,xmm15);	/* xmm15 = y7 */

    paddsw_r2r (xmm0, xmm11);		/* xmm11 = a3+b3 */
    psraw_i2r (COL_SHIFT,xmm12);	/* xmm12 = y4 */

    psraw_i2r (COL_SHIFT,xmm11);	/* xmm11 = y3 */

    /* OUTPUT: block in xmm8 ... xmm15 */

  #else
    movdqa_m2r (*t1_vector, xmm0); /* xmm0 = T1 */

    movdqa_m2r (*(col+1*8), xmm1); /* xmm1 = x1 */
    movdqa_r2r (xmm0, xmm2); /* xmm2 = T1 */

    movdqa_m2r (*(col+7*8), xmm4); /* xmm4 = x7 */
    pmulhw_r2r (xmm1, xmm0); /* xmm0 = T1*x1 */

    movdqa_m2r (*t3_vector, xmm5); /* xmm5 = T3 */
    pmulhw_r2r (xmm4, xmm2); /* xmm2 = T1*x7 */

    movdqa_m2r (*(col+5*8), xmm6); /* xmm6 = x5 */
    movdqa_r2r (xmm5, xmm7); /* xmm7 = T3-1 */

    movdqa_m2r (*(col+3*8), xmm3); /* xmm3 = x3 */
    psubsw_r2r (xmm4, xmm0); /* xmm0 = v17 */

    movdqa_m2r (*t2_vector, xmm4); /* xmm4 = T2 */
    pmulhw_r2r (xmm3, xmm5); /* xmm5 = (T3-1)*x3 */

    paddsw_r2r (xmm2, xmm1); /* xmm1 = u17 */
    pmulhw_r2r (xmm6, xmm7); /* xmm7 = (T3-1)*x5 */

    /* slot */

    movdqa_r2r (xmm4, xmm2); /* xmm2 = T2 */
    paddsw_r2r (xmm3, xmm5); /* xmm5 = T3*x3 */

    pmulhw_m2r (*(col+2*8), xmm4); /* xmm4 = T2*x2 */
    paddsw_r2r (xmm6, xmm7); /* xmm7 = T3*x5 */

    psubsw_r2r (xmm6, xmm5); /* xmm5 = v35 */
    paddsw_r2r (xmm3, xmm7); /* xmm7 = u35 */

    movdqa_m2r (*(col+6*8), xmm3); /* xmm3 = x6 */
    movdqa_r2r (xmm0, xmm6); /* xmm6 = v17 */

    pmulhw_r2r (xmm3, xmm2); /* xmm2 = T2*x6 */
    psubsw_r2r (xmm5, xmm0); /* xmm0 = b3 */

    psubsw_r2r (xmm3, xmm4); /* xmm4 = v26 */
    paddsw_r2r (xmm6, xmm5); /* xmm5 = v12 */

    movdqa_r2m (xmm0, *(col+3*8)); /* save b3 in scratch0 */
    movdqa_r2r (xmm1, xmm6); /* xmm6 = u17 */

    paddsw_m2r (*(col+2*8), xmm2); /* xmm2 = u26 */
    paddsw_r2r (xmm7, xmm6); /* xmm6 = b0 */

    psubsw_r2r (xmm7, xmm1); /* xmm1 = u12 */
    movdqa_r2r (xmm1, xmm7); /* xmm7 = u12 */

    movdqa_m2r (*(col+0*8), xmm3); /* xmm3 = x0 */
    paddsw_r2r (xmm5, xmm1); /* xmm1 = u12+v12 */

    movdqa_m2r (*c4_vector, xmm0); /* xmm0 = C4/2 */
    psubsw_r2r (xmm5, xmm7); /* xmm7 = u12-v12 */

    movdqa_r2m (xmm6, *(col+5*8)); /* save b0 in scratch1 */
    pmulhw_r2r (xmm0, xmm1); /* xmm1 = b1/2 */

    movdqa_r2r (xmm4, xmm6); /* xmm6 = v26 */
    pmulhw_r2r (xmm0, xmm7); /* xmm7 = b2/2 */

    movdqa_m2r (*(col+4*8), xmm5); /* xmm5 = x4 */
    movdqa_r2r (xmm3, xmm0); /* xmm0 = x0 */

    psubsw_r2r (xmm5, xmm3); /* xmm3 = v04 */
    paddsw_r2r (xmm5, xmm0); /* xmm0 = u04 */

    paddsw_r2r (xmm3, xmm4); /* xmm4 = a1 */
    movdqa_r2r (xmm0, xmm5); /* xmm5 = u04 */

    psubsw_r2r (xmm6, xmm3); /* xmm3 = a2 */
    paddsw_r2r (xmm2, xmm5); /* xmm5 = a0 */

    paddsw_r2r (xmm1, xmm1); /* xmm1 = b1 */
    psubsw_r2r (xmm2, xmm0); /* xmm0 = a3 */

    paddsw_r2r (xmm7, xmm7); /* xmm7 = b2 */
    movdqa_r2r (xmm3, xmm2); /* xmm2 = a2 */

    movdqa_r2r (xmm4, xmm6); /* xmm6 = a1 */
    paddsw_r2r (xmm7, xmm3); /* xmm3 = a2+b2 */

    psraw_i2r (COL_SHIFT, xmm3); /* xmm3 = y2 */
    paddsw_r2r (xmm1, xmm4); /* xmm4 = a1+b1 */

    psraw_i2r (COL_SHIFT, xmm4); /* xmm4 = y1 */
    psubsw_r2r (xmm1, xmm6); /* xmm6 = a1-b1 */

    movdqa_m2r (*(col+5*8), xmm1); /* xmm1 = b0 */
    psubsw_r2r (xmm7, xmm2); /* xmm2 = a2-b2 */

    psraw_i2r (COL_SHIFT, xmm6); /* xmm6 = y6 */
    movdqa_r2r (xmm5, xmm7); /* xmm7 = a0 */

    movdqa_r2m (xmm4, *(col+1*8)); /* save y1 */
    psraw_i2r (COL_SHIFT, xmm2); /* xmm2 = y5 */

    movdqa_r2m (xmm3, *(col+2*8)); /* save y2 */
    paddsw_r2r (xmm1, xmm5); /* xmm5 = a0+b0 */

    movdqa_m2r (*(col+3*8), xmm4); /* xmm4 = b3 */
    psubsw_r2r (xmm1, xmm7); /* xmm7 = a0-b0 */

    psraw_i2r (COL_SHIFT, xmm5); /* xmm5 = y0 */
    movdqa_r2r (xmm0, xmm3); /* xmm3 = a3 */

    movdqa_r2m (xmm2, *(col+5*8)); /* save y5 */
    psubsw_r2r (xmm4, xmm3); /* xmm3 = a3-b3 */

    psraw_i2r (COL_SHIFT, xmm7); /* xmm7 = y7 */
    paddsw_r2r (xmm0, xmm4); /* xmm4 = a3+b3 */

    movdqa_r2m (xmm5, *(col+0*8)); /* save y0 */
    psraw_i2r (COL_SHIFT, xmm3); /* xmm3 = y4 */

    movdqa_r2m (xmm6, *(col+6*8)); /* save y6 */
    psraw_i2r (COL_SHIFT, xmm4); /* xmm4 = y3 */

    movdqa_r2m (xmm7, *(col+7*8)); /* save y7 */

    movdqa_r2m (xmm3, *(col+4*8)); /* save y4 */

    movdqa_r2m (xmm4, *(col+3*8)); /* save y3 */
  #endif
#endif
}

/* MMX column IDCT */
static inline void idct_col(int16_t * const col, const int offset) {
#ifdef _MSC_VER

#else
  static ALIGNED_ARRAY(const short, 8) t1_vector[]  = {T1,T1,T1,T1};
  static ALIGNED_ARRAY(const short, 8) t2_vector[]  = {T2,T2,T2,T2};
  static ALIGNED_ARRAY(const short, 8) t3_vector[]  = {T3,T3,T3,T3};
  static ALIGNED_ARRAY(const short, 8) c4_vector[]  = {C4,C4,C4,C4};

  /* column code adapted from peter gubanov */
  /* http://www.elecard.com/peter/idct.shtml */

  movq_m2r (*t1_vector, mm0); /* mm0 = T1 */

  movq_m2r (*(col+offset+1*8), mm1); /* mm1 = x1 */
  movq_r2r (mm0, mm2); /* mm2 = T1 */

  movq_m2r (*(col+offset+7*8), mm4); /* mm4 = x7 */
  pmulhw_r2r (mm1, mm0); /* mm0 = T1*x1 */

  movq_m2r (*t3_vector, mm5); /* mm5 = T3 */
  pmulhw_r2r (mm4, mm2); /* mm2 = T1*x7 */

  movq_m2r (*(col+offset+5*8), mm6); /* mm6 = x5 */
  movq_r2r (mm5, mm7); /* mm7 = T3-1 */

  movq_m2r (*(col+offset+3*8), mm3); /* mm3 = x3 */
  psubsw_r2r (mm4, mm0); /* mm0 = v17 */

  movq_m2r (*t2_vector, mm4); /* mm4 = T2 */
  pmulhw_r2r (mm3, mm5); /* mm5 = (T3-1)*x3 */

  paddsw_r2r (mm2, mm1); /* mm1 = u17 */
  pmulhw_r2r (mm6, mm7); /* mm7 = (T3-1)*x5 */

  /* slot */

  movq_r2r (mm4, mm2); /* mm2 = T2 */
  paddsw_r2r (mm3, mm5); /* mm5 = T3*x3 */

  pmulhw_m2r (*(col+offset+2*8), mm4);/* mm4 = T2*x2 */
  paddsw_r2r (mm6, mm7); /* mm7 = T3*x5 */

  psubsw_r2r (mm6, mm5); /* mm5 = v35 */
  paddsw_r2r (mm3, mm7); /* mm7 = u35 */

  movq_m2r (*(col+offset+6*8), mm3); /* mm3 = x6 */
  movq_r2r (mm0, mm6); /* mm6 = v17 */

  pmulhw_r2r (mm3, mm2); /* mm2 = T2*x6 */
  psubsw_r2r (mm5, mm0); /* mm0 = b3 */

  psubsw_r2r (mm3, mm4); /* mm4 = v26 */
  paddsw_r2r (mm6, mm5); /* mm5 = v12 */

  movq_r2m (mm0, *(col+offset+3*8)); /* save b3 in scratch0 */
  movq_r2r (mm1, mm6); /* mm6 = u17 */

  paddsw_m2r (*(col+offset+2*8), mm2);/* mm2 = u26 */
  paddsw_r2r (mm7, mm6); /* mm6 = b0 */

  psubsw_r2r (mm7, mm1); /* mm1 = u12 */
  movq_r2r (mm1, mm7); /* mm7 = u12 */

  movq_m2r (*(col+offset+0*8), mm3); /* mm3 = x0 */
  paddsw_r2r (mm5, mm1); /* mm1 = u12+v12 */

  movq_m2r (*c4_vector, mm0); /* mm0 = C4/2 */
  psubsw_r2r (mm5, mm7); /* mm7 = u12-v12 */

  movq_r2m (mm6, *(col+offset+5*8)); /* save b0 in scratch1 */
  pmulhw_r2r (mm0, mm1); /* mm1 = b1/2 */

  movq_r2r (mm4, mm6); /* mm6 = v26 */
  pmulhw_r2r (mm0, mm7); /* mm7 = b2/2 */

  movq_m2r (*(col+offset+4*8), mm5); /* mm5 = x4 */
  movq_r2r (mm3, mm0); /* mm0 = x0 */

  psubsw_r2r (mm5, mm3); /* mm3 = v04 */
  paddsw_r2r (mm5, mm0); /* mm0 = u04 */

  paddsw_r2r (mm3, mm4); /* mm4 = a1 */
  movq_r2r (mm0, mm5); /* mm5 = u04 */

  psubsw_r2r (mm6, mm3); /* mm3 = a2 */
  paddsw_r2r (mm2, mm5); /* mm5 = a0 */

  paddsw_r2r (mm1, mm1); /* mm1 = b1 */
  psubsw_r2r (mm2, mm0); /* mm0 = a3 */

  paddsw_r2r (mm7, mm7); /* mm7 = b2 */
  movq_r2r (mm3, mm2); /* mm2 = a2 */

  movq_r2r (mm4, mm6); /* mm6 = a1 */
  paddsw_r2r (mm7, mm3); /* mm3 = a2+b2 */

  psraw_i2r (COL_SHIFT, mm3); /* mm3 = y2 */
  paddsw_r2r (mm1, mm4); /* mm4 = a1+b1 */

  psraw_i2r (COL_SHIFT, mm4); /* mm4 = y1 */
  psubsw_r2r (mm1, mm6); /* mm6 = a1-b1 */

  movq_m2r (*(col+offset+5*8), mm1); /* mm1 = b0 */
  psubsw_r2r (mm7, mm2); /* mm2 = a2-b2 */

  psraw_i2r (COL_SHIFT, mm6); /* mm6 = y6 */
  movq_r2r (mm5, mm7); /* mm7 = a0 */

  movq_r2m (mm4, *(col+offset+1*8)); /* save y1 */
  psraw_i2r (COL_SHIFT, mm2); /* mm2 = y5 */

  movq_r2m (mm3, *(col+offset+2*8)); /* save y2 */
  paddsw_r2r (mm1, mm5); /* mm5 = a0+b0 */

  movq_m2r (*(col+offset+3*8), mm4); /* mm4 = b3 */
  psubsw_r2r (mm1, mm7); /* mm7 = a0-b0 */

  psraw_i2r (COL_SHIFT, mm5); /* mm5 = y0 */
  movq_r2r (mm0, mm3); /* mm3 = a3 */

  movq_r2m (mm2, *(col+offset+5*8)); /* save y5 */
  psubsw_r2r (mm4, mm3); /* mm3 = a3-b3 */

  psraw_i2r (COL_SHIFT, mm7); /* mm7 = y7 */
  paddsw_r2r (mm0, mm4); /* mm4 = a3+b3 */

  movq_r2m (mm5, *(col+offset+0*8)); /* save y0 */
  psraw_i2r (COL_SHIFT, mm3); /* mm3 = y4 */

  movq_r2m (mm6, *(col+offset+6*8)); /* save y6 */
  psraw_i2r (COL_SHIFT, mm4); /* mm4 = y3 */

  movq_r2m (mm7, *(col+offset+7*8)); /* save y7 */

  movq_r2m (mm3, *(col+offset+4*8)); /* save y4 */

  movq_r2m (mm4, *(col+offset+3*8)); /* save y3 */
#endif
}

static inline void sse2_idct(int16_t * const block) {
  static ALIGNED_ARRAY(const int16_t, 16) table04[]  = sse2_table (22725, 21407, 19266, 16384, 12873, 8867, 4520);
  static ALIGNED_ARRAY(const int16_t, 16) table17[]  = sse2_table (31521, 29692, 26722, 22725, 17855, 12299, 6270);
  static ALIGNED_ARRAY(const int16_t, 16) table26[]  = sse2_table (29692, 27969, 25172, 21407, 16819, 11585, 5906);
  static ALIGNED_ARRAY(const int16_t, 16) table35[]  = sse2_table (26722, 25172, 22654, 19266, 15137, 10426, 5315);

  static ALIGNED_ARRAY(const int16_t, 16) rounder0_128[]  = rounder_sse2 ((1 << (COL_SHIFT - 1)) - 0.5);
  static ALIGNED_ARRAY(const int16_t, 16) rounder4_128[]  = rounder_sse2 (0);
  static ALIGNED_ARRAY(const int16_t, 16) rounder1_128[]  = rounder_sse2 (1.25683487303); /* C1*(C1/C4+C1+C7)/2 */
  static ALIGNED_ARRAY(const int16_t, 16) rounder7_128[]  = rounder_sse2 (-0.25); /* C1*(C7/C4+C7-C1)/2 */
  static ALIGNED_ARRAY(const int16_t, 16) rounder2_128[]  = rounder_sse2 (0.60355339059); /* C2 * (C6+C2)/2 */
  static ALIGNED_ARRAY(const int16_t, 16) rounder6_128[]  = rounder_sse2 (-0.25); /* C2 * (C6-C2)/2 */
  static ALIGNED_ARRAY(const int16_t, 16) rounder3_128[]  = rounder_sse2 (0.087788325588); /* C3*(-C3/C4+C3+C5)/2 */
  static ALIGNED_ARRAY(const int16_t, 16) rounder5_128[]  = rounder_sse2 (-0.441341716183); /* C3*(-C5/C4+C5-C3)/2 */

#if defined(__x86_64__)
    movdqa_m2r (block[0*8], xmm8);
    movdqa_m2r (block[4*8], xmm12);
    SSE2_IDCT_2ROW (table04,  xmm8, xmm12, *rounder0_128, *rounder4_128);

    movdqa_m2r (block[1*8], xmm9);
    movdqa_m2r (block[7*8], xmm15);
    SSE2_IDCT_2ROW (table17,  xmm9, xmm15, *rounder1_128, *rounder7_128);

    movdqa_m2r (block[2*8], xmm10);
    movdqa_m2r (block[6*8], xmm14);
    SSE2_IDCT_2ROW (table26, xmm10, xmm14, *rounder2_128, *rounder6_128);

    movdqa_m2r (block[3*8], xmm11);
    movdqa_m2r (block[5*8], xmm13);
    SSE2_IDCT_2ROW (table35, xmm11, xmm13, *rounder3_128, *rounder5_128);

  /* OUTPUT: block in xmm8 ... xmm15 */
#else
  movdqa_m2r (block[0*8], xmm0);
  movdqa_m2r (block[4*8], xmm4);
  SSE2_IDCT_2ROW (table04, xmm0, xmm4, *rounder0_128, *rounder4_128);
  movdqa_r2m (xmm0, block[0*8]);
  movdqa_r2m (xmm4, block[4*8]);

  movdqa_m2r (block[1*8], xmm0);
  movdqa_m2r (block[7*8], xmm4);
  SSE2_IDCT_2ROW (table17, xmm0, xmm4, *rounder1_128, *rounder7_128);
  movdqa_r2m (xmm0, block[1*8]);
  movdqa_r2m (xmm4, block[7*8]);

  movdqa_m2r (block[2*8], xmm0);
  movdqa_m2r (block[6*8], xmm4);
  SSE2_IDCT_2ROW (table26, xmm0, xmm4, *rounder2_128, *rounder6_128);
  movdqa_r2m (xmm0, block[2*8]);
  movdqa_r2m (xmm4, block[6*8]);

  movdqa_m2r (block[3*8], xmm0);
  movdqa_m2r (block[5*8], xmm4);
  SSE2_IDCT_2ROW (table35, xmm0, xmm4, *rounder3_128, *rounder5_128);
  movdqa_r2m (xmm0, block[3*8]);
  movdqa_r2m (xmm4, block[5*8]);
#endif

  sse2_idct_col(block);
}

static void sse2_block_copy(int16_t * const block, uint8_t *dest, const int stride) {
#if defined(_MSC_VER)

#else
  #if defined(__x86_64__)
  /* INPUT: block in xmm8 ... xmm15 */
    packuswb_r2r (xmm8, xmm8);
    packuswb_r2r (xmm9, xmm9);
    movq_r2m (xmm8,  *(dest+0*stride));
    packuswb_r2r (xmm10, xmm10);
    movq_r2m (xmm9,  *(dest+1*stride));
    packuswb_r2r (xmm11, xmm11);
    movq_r2m (xmm10, *(dest+2*stride));
    packuswb_r2r (xmm12, xmm12);
    movq_r2m (xmm11, *(dest+3*stride));
    packuswb_r2r (xmm13, xmm13);
    movq_r2m (xmm12, *(dest+4*stride));
    packuswb_r2r (xmm14, xmm14);
    movq_r2m (xmm13, *(dest+5*stride));
    packuswb_r2r (xmm15, xmm15);
    movq_r2m (xmm14, *(dest+6*stride));
    movq_r2m (xmm15, *(dest+7*stride));
  #else
    movdqa_m2r (*(block+0*8), xmm0);
    movdqa_m2r (*(block+1*8), xmm1);
    movdqa_m2r (*(block+2*8), xmm2);
    packuswb_r2r (xmm0, xmm0);
    movdqa_m2r (*(block+3*8), xmm3);
    packuswb_r2r (xmm1, xmm1);
    movdqa_m2r (*(block+4*8), xmm4);
    packuswb_r2r (xmm2, xmm2);
    movdqa_m2r (*(block+5*8), xmm5);
    packuswb_r2r (xmm3, xmm3);
    movdqa_m2r (*(block+6*8), xmm6);
    packuswb_r2r (xmm4, xmm4);
    movdqa_m2r (*(block+7*8), xmm7);
    movq_r2m (xmm0, *(dest+0*stride));
    packuswb_r2r (xmm5, xmm5);
    movq_r2m (xmm1, *(dest+1*stride));
    packuswb_r2r (xmm6, xmm6);
    movq_r2m (xmm2, *(dest+2*stride));
    packuswb_r2r (xmm7, xmm7);
    movq_r2m (xmm3, *(dest+3*stride));
    movq_r2m (xmm4, *(dest+4*stride));
    movq_r2m (xmm5, *(dest+5*stride));
    movq_r2m (xmm6, *(dest+6*stride));
    movq_r2m (xmm7, *(dest+7*stride));
  #endif
#endif
}

static inline void block_copy(int16_t * const block, uint8_t *dest, const int stride) {
#if defined(_MSC_VER)

#else
  movq_m2r (*(block+0*8), mm0);
  movq_m2r (*(block+0*8+4), mm1);
  movq_m2r (*(block+1*8), mm2);
  packuswb_r2r (mm1, mm0);
  movq_m2r (*(block+1*8+4), mm3);
  movq_r2m (mm0, *dest);
  packuswb_r2r (mm3, mm2);
  COPY_MMX (2*8, mm0, mm1, mm2);
  COPY_MMX (3*8, mm2, mm3, mm0);
  COPY_MMX (4*8, mm0, mm1, mm2);
  COPY_MMX (5*8, mm2, mm3, mm0);
  COPY_MMX (6*8, mm0, mm1, mm2);
  COPY_MMX (7*8, mm2, mm3, mm0);
  movq_r2m (mm2, *(dest+stride));
#endif
}

static void sse2_block_add(int16_t * const block, uint8_t *dest, const int stride) {
#if defined(_MSC_VER)
  __m128i *src = (__m128i*)block;
  __m128i r0, r1, r2, r3, r4, r5, r6, r7;
  __m128i zero;

  r0 = _mm_loadl_epi64((__m128i*)(dest + 0 * stride));
  r1 = _mm_loadl_epi64((__m128i*)(dest + 1 * stride));
  r2 = _mm_loadl_epi64((__m128i*)(dest + 2 * stride));
  r3 = _mm_loadl_epi64((__m128i*)(dest + 3 * stride));
  r4 = _mm_loadl_epi64((__m128i*)(dest + 4 * stride));
  r5 = _mm_loadl_epi64((__m128i*)(dest + 5 * stride));
  r6 = _mm_loadl_epi64((__m128i*)(dest + 6 * stride));
  r7 = _mm_loadl_epi64((__m128i*)(dest + 7 * stride));

  zero = _mm_setzero_si128();

  r0 = _mm_unpacklo_epi8(r0, zero);
  r1 = _mm_unpacklo_epi8(r1, zero);
  r2 = _mm_unpacklo_epi8(r2, zero);
  r3 = _mm_unpacklo_epi8(r3, zero);
  r4 = _mm_unpacklo_epi8(r4, zero);
  r5 = _mm_unpacklo_epi8(r5, zero);
  r6 = _mm_unpacklo_epi8(r6, zero);
  r7 = _mm_unpacklo_epi8(r7, zero);

  r0 = _mm_adds_epi16(src0, r0);
  r1 = _mm_adds_epi16(src1, r1);
  r2 = _mm_adds_epi16(src2, r2);
  r3 = _mm_adds_epi16(src3, r3);
  r4 = _mm_adds_epi16(src4, r4);
  r5 = _mm_adds_epi16(src5, r5);
  r6 = _mm_adds_epi16(src6, r6);
  r7 = _mm_adds_epi16(src7, r7);

  r0 = _mm_packus_epi16(r0, r1);
  r1 = _mm_packus_epi16(r2, r3);
  r2 = _mm_packus_epi16(r4, r5);
  r3 = _mm_packus_epi16(r6, r7);

  mm_storel_epi64(dest + 0 * stride, r0);
  mm_storeh_epi64(dest + 1 * stride, r0);
  mm_storel_epi64(dest + 2 * stride, r1);
  mm_storeh_epi64(dest + 3 * stride, r1);
  mm_storel_epi64(dest + 4 * stride, r2);
  mm_storeh_epi64(dest + 5 * stride, r2);
  mm_storel_epi64(dest + 6 * stride, r3);
  mm_storeh_epi64(dest + 7 * stride, r3);
#else
  pxor_r2r(xmm0, xmm0);
  #if defined(__x86_64__)
    /* INPUT: block in xmm8 ... xmm15 */
    ADD_SSE2_2ROW(r2r, xmm8, xmm9);
    ADD_SSE2_2ROW(r2r, xmm10, xmm11);
    ADD_SSE2_2ROW(r2r, xmm12, xmm13);
    ADD_SSE2_2ROW(r2r, xmm14, xmm15);
  #else
    ADD_SSE2_2ROW(m2r, *(block+0*8), *(block+1*8));
    ADD_SSE2_2ROW(m2r, *(block+2*8), *(block+3*8));
    ADD_SSE2_2ROW(m2r, *(block+4*8), *(block+5*8));
    ADD_SSE2_2ROW(m2r, *(block+6*8), *(block+7*8));
  #endif
#endif
}

static inline void block_add(int16_t * const block, uint8_t *dest, const int stride) {
#if defined(_MSC_VER)

#else
  movq_m2r (*dest, mm1);
  pxor_r2r (mm0, mm0);
  movq_m2r (*(dest+stride), mm3);
  movq_r2r (mm1, mm2);
  punpcklbw_r2r (mm0, mm1);
  movq_r2r (mm3, mm4);
  paddsw_m2r (*(block+0*8), mm1);
  punpckhbw_r2r (mm0, mm2);
  paddsw_m2r (*(block+0*8+4), mm2);
  punpcklbw_r2r (mm0, mm3);
  paddsw_m2r (*(block+1*8), mm3);
  packuswb_r2r (mm2, mm1);
  punpckhbw_r2r (mm0, mm4);
  movq_r2m (mm1, *dest);
  paddsw_m2r (*(block+1*8+4), mm4);
  ADD_MMX (2*8, mm1, mm2, mm3, mm4);
  ADD_MMX (3*8, mm3, mm4, mm1, mm2);
  ADD_MMX (4*8, mm1, mm2, mm3, mm4);
  ADD_MMX (5*8, mm3, mm4, mm1, mm2);
  ADD_MMX (6*8, mm1, mm2, mm3, mm4);
  ADD_MMX (7*8, mm3, mm4, mm1, mm2);
  packuswb_r2r (mm4, mm3);
  movq_r2m (mm3, *(dest+stride));
#endif
}

static inline void sse2_block_zero(int16_t * const block) {
#if defined(_MSC_VER)
  __m128i* src = (__m128i*) block;
  __m128i zero;

  zero = _mm_setzero_si128();
  _mm_store_si128(src + 0, zero);
  _mm_store_si128(src + 1, zero);
  _mm_store_si128(src + 2, zero);
  _mm_store_si128(src + 3, zero);
  _mm_store_si128(src + 4, zero);
  _mm_store_si128(src + 5, zero);
  _mm_store_si128(src + 6, zero);
  _mm_store_si128(src + 7, zero);
#else
  pxor_r2r(xmm0, xmm0);
  movdqa_r2m(xmm0, *(block + 0 * 8));
  movdqa_r2m(xmm0, *(block + 1 * 8));
  movdqa_r2m(xmm0, *(block + 2 * 8));
  movdqa_r2m(xmm0, *(block + 3 * 8));
  movdqa_r2m(xmm0, *(block + 4 * 8));
  movdqa_r2m(xmm0, *(block + 5 * 8));
  movdqa_r2m(xmm0, *(block + 6 * 8));
  movdqa_r2m(xmm0, *(block + 7 * 8));
#endif
}

static inline void block_zero(int16_t * const block) {
#if defined(_MSC_VER)
  __m64* src = (__m64*) block;
  __m64 zero;

  zero = _m_pxor(zero, zero);
  _mm_stream_pi(src + 0 * 4, zero);
  _mm_stream_pi(src + 1 * 4, zero);
  _mm_stream_pi(src + 2 * 4, zero);
  _mm_stream_pi(src + 3 * 4, zero);
  _mm_stream_pi(src + 4 * 4, zero);
  _mm_stream_pi(src + 5 * 4, zero);
  _mm_stream_pi(src + 6 * 4, zero);
  _mm_stream_pi(src + 7 * 4, zero);
  _mm_stream_pi(src + 8 * 4, zero);
  _mm_stream_pi(src + 9 * 4, zero);
  _mm_stream_pi(src + 10 * 4, zero);
  _mm_stream_pi(src + 11 * 4, zero);
  _mm_stream_pi(src + 12 * 4, zero);
  _mm_stream_pi(src + 13 * 4, zero);
  _mm_stream_pi(src + 14 * 4, zero);
  _mm_stream_pi(src + 15 * 4, zero);
#else
  pxor_r2r(mm0, mm0);
  movq_r2m(mm0, *(block + 0 * 4));
  movq_r2m(mm0, *(block + 1 * 4));
  movq_r2m (mm0, *(block+2*4));
  movq_r2m (mm0, *(block+3*4));
  movq_r2m (mm0, *(block+4*4));
  movq_r2m (mm0, *(block+5*4));
  movq_r2m (mm0, *(block+6*4));
  movq_r2m (mm0, *(block+7*4));
  movq_r2m (mm0, *(block+8*4));
  movq_r2m (mm0, *(block+9*4));
  movq_r2m (mm0, *(block+10*4));
  movq_r2m (mm0, *(block+11*4));
  movq_r2m (mm0, *(block+12*4));
  movq_r2m (mm0, *(block+13*4));
  movq_r2m (mm0, *(block+14*4));
  movq_r2m (mm0, *(block+15*4));
#endif
}

static inline void block_add_DC(int16_t * const block, uint8_t *dest, const int stride, const int cpu) {
#ifdef _MSC_VER

#else
  movd_v2r ((block[0] + 64) >> 7, mm0);
  pxor_r2r (mm1, mm1);
  movq_m2r (*dest, mm2);
  dup4 (mm0);
  psubsw_r2r (mm0, mm1);
  packuswb_r2r (mm0, mm0);
  paddusb_r2r (mm0, mm2);
  packuswb_r2r (mm1, mm1);
  movq_m2r (*(dest + stride), mm3);
  psubusb_r2r (mm1, mm2);
  block[0] = 0;
  paddusb_r2r (mm0, mm3);
  movq_r2m (mm2, *dest);
  psubusb_r2r (mm1, mm3);
  movq_m2r (*(dest + 2*stride), mm2);
  dest += stride;
  movq_r2m (mm3, *dest);
  paddusb_r2r (mm0, mm2);
  movq_m2r (*(dest + 2*stride), mm3);
  psubusb_r2r (mm1, mm2);
  dest += stride;
  paddusb_r2r (mm0, mm3);
  movq_r2m (mm2, *dest);
  psubusb_r2r (mm1, mm3);
  movq_m2r (*(dest + 2*stride), mm2);
  dest += stride;
  movq_r2m (mm3, *dest);
  paddusb_r2r (mm0, mm2);
  movq_m2r (*(dest + 2*stride), mm3);
  psubusb_r2r (mm1, mm2);
  dest += stride;
  paddusb_r2r (mm0, mm3);
  movq_r2m (mm2, *dest);
  psubusb_r2r (mm1, mm3);
  movq_m2r (*(dest + 2*stride), mm2);
  dest += stride;
  movq_r2m (mm3, *dest);
  paddusb_r2r (mm0, mm2);
  movq_m2r (*(dest + 2*stride), mm3);
  psubusb_r2r (mm1, mm2);
  block[63] = 0;
  paddusb_r2r (mm0, mm3);
  movq_r2m (mm2, *(dest + stride));
  psubusb_r2r (mm1, mm3);
  movq_r2m (mm3, *(dest + 2*stride));
#endif
}



void idct_sse2_asm() {
  __m128i xmm0, xmm1, xmm2, xmm3, xmm4,
    xmm5, xmm6, xmm7;
  const uint8_t * esi, *ecx;
  xmm0 = src0;
  esi = (const uint8_t *)M128_tab_i_04;
  xmm4 = src2;
  ecx = (const uint8_t *)M128_tab_i_26;
  __asm call dct_8_inv_row
    src0 = xmm0;
  src2 = xmm4;
  xmm0 = src4;
  xmm4 = src6;
  __asm call dct_8_inv_row
    src4 = xmm0;
  src6 = xmm4;
  xmm0 = src3;
  esi = (const uint8_t *)M128_tab_i_35;
  xmm4 = src1;
  ecx = (const uint8_t *)M128_tab_i_17;
  __asm call dct_8_inv_row
    src3 = xmm0;
  src1 = xmm4;
  xmm0 = src5;
  xmm4 = src7;
  __asm call dct_8_inv_row
  __asm call dct_8_inv_col_8
  return;

dct_8_inv_row:
  xmm0 = _mm_shufflelo_epi16(xmm0, 0xd8);
  xmm1 = _mm_shuffle_epi32(xmm0, 0);
  xmm1 = mm_madd_epi16(xmm1, esi);
  xmm3 = _mm_shuffle_epi32(xmm0, 0x55);
  xmm0 = _mm_shufflehi_epi16(xmm0, 0xd8);
  xmm3 = mm_madd_epi16(xmm3, esi + 32);
  xmm2 = _mm_shuffle_epi32(xmm0, 0xaa);
  xmm0 = _mm_shuffle_epi32(xmm0, 0xff);
  xmm2 = mm_madd_epi16(xmm2, esi + 16);
  xmm4 = _mm_shufflehi_epi16(xmm4, 0xd8);
  xmm1 = mm_add_epi32(xmm1);
  xmm4 = _mm_shufflelo_epi16(xmm4, 0xd8);
  xmm0 = mm_madd_epi16(xmm0, esi + 48);
  xmm5 = _mm_shuffle_epi32(xmm4, 0);
  xmm6 = _mm_shuffle_epi32(xmm4, 0xaa);
  xmm5 = mm_madd_epi16(xmm5, ecx);
  xmm1 = _mm_add_epi32(xmm1, xmm2);
  xmm2 = xmm1;
  xmm7 = _mm_shuffle_epi32(xmm4, 0x55);
  xmm6 = mm_madd_epi16(xmm6, ecx + 16);
  xmm0 = _mm_add_epi32(xmm0, xmm3);
  xmm4 = _mm_shuffle_epi32(xmm4, 0xff);
  xmm2 = _mm_sub_epi32(xmm2, xmm0);
  xmm7 = mm_madd_epi16(xmm7, ecx + 32);
  xmm0 = _mm_add_epi32(xmm0, xmm1);
  xmm2 = _mm_srai_epi32(xmm2, 12);
  xmm5 = mm_add_epi32(xmm5);
  xmm4 = mm_madd_epi16(xmm4, ecx + 48);
  xmm5 = _mm_add_epi32(xmm5, xmm6);
  xmm6 = xmm5;
  xmm0 = _mm_srai_epi32(xmm0, 12);
  xmm2 = _mm_shuffle_epi32(xmm2, 0x1b);
  xmm0 = _mm_packs_epi32(xmm0, xmm2);
  xmm4 = _mm_add_epi32(xmm4, xmm7);
  xmm6 = _mm_sub_epi32(xmm6, xmm4);
  xmm4 = _mm_add_epi32(xmm4, xmm5);
  xmm6 = _mm_srai_epi32(xmm6, 12);
  xmm4 = _mm_srai_epi32(xmm4, 12);
  xmm6 = _mm_shuffle_epi32(xmm6, 0x1b);
  xmm4 = _mm_packs_epi32(xmm4, xmm6);
__asm ret

dct_8_inv_col_8: 
  xmm1 = *(__m128i*)M128_tg_3_16;
  xmm2 = xmm0;
  xmm3 = src3;
  xmm0 = _mm_mulhi_epi16(xmm0, xmm1);
  xmm1 = _mm_mulhi_epi16(xmm1, xmm3);
  xmm5 = *(__m128i*)M128_tg_1_16;
  xmm6 = xmm4;
  xmm4 = _mm_mulhi_epi16(xmm4, xmm5);
  xmm0 = _mm_adds_epi16(xmm0, xmm2);
  xmm5 = _mm_mulhi_epi16(xmm5, src1);
  xmm1 = _mm_adds_epi16(xmm1, xmm3);
  xmm7 = src6;
  xmm0 = _mm_adds_epi16(xmm0, xmm3);
  xmm3 = *(__m128i*)M128_tg_2_16;
  xmm2 = _mm_subs_epi16(xmm2, xmm1);
  xmm7 = _mm_mulhi_epi16(xmm7, xmm3);
  xmm1 = xmm0;
  xmm3 = _mm_mulhi_epi16(xmm3, src2);
  xmm5 = _mm_subs_epi16(xmm5, xmm6);
  xmm4 = _mm_adds_epi16(xmm4, src1);
  xmm0 = _mm_adds_epi16(xmm0, xmm4);
  xmm0 = mm_adds_epi16(xmm0, M128_one_corr);
  xmm4 = _mm_subs_epi16(xmm4, xmm1);
  xmm6 = xmm5;
  xmm5 = _mm_subs_epi16(xmm5, xmm2);
  xmm5 = mm_adds_epi16(xmm5, M128_one_corr);
  xmm6 = _mm_adds_epi16(xmm6, xmm2);
  src7 = xmm0;
  xmm1 = xmm4;
  xmm0 = *(__m128i*)M128_cos_4_16;
  xmm4 = _mm_adds_epi16(xmm4, xmm5);
  xmm2 = *(__m128i*)M128_cos_4_16;
  xmm2 = _mm_mulhi_epi16(xmm2, xmm4);
  src3 = xmm6;
  xmm1 = _mm_subs_epi16(xmm1, xmm5);
  xmm7 = _mm_adds_epi16(xmm7, src2);
  xmm3 = _mm_subs_epi16(xmm3, src6);
  xmm6 = src0;
  xmm0 = _mm_mulhi_epi16(xmm0, xmm1);
  xmm5 = src4;
  xmm5 = _mm_adds_epi16(xmm5, xmm6);
  xmm6 = _mm_subs_epi16(xmm6, src4);
  xmm4 = _mm_adds_epi16(xmm4, xmm2);
  xmm4 = mm_or_si128(xmm4, M128_one_corr);
  xmm0 = _mm_adds_epi16(xmm0, xmm1);
  xmm0 = mm_or_si128(xmm0, M128_one_corr);
  xmm2 = xmm5;
  xmm5 = _mm_adds_epi16(xmm5, xmm7);
  xmm1 = xmm6;
  xmm5 = mm_adds_epi16(xmm5, M128_round_inv_col);
  xmm2 = _mm_subs_epi16(xmm2, xmm7);
  xmm7 = src7;
  xmm6 = _mm_adds_epi16(xmm6, xmm3);
  xmm6 = mm_adds_epi16(xmm6, M128_round_inv_col);
  xmm7 = _mm_adds_epi16(xmm7, xmm5);
  xmm7 = _mm_srai_epi16(xmm7, SHIFT_INV_COL);
  xmm1 = _mm_subs_epi16(xmm1, xmm3);
  xmm1 = mm_adds_epi16(xmm1, M128_round_inv_corr);
  xmm3 = xmm6;
  xmm2 = mm_adds_epi16(xmm2, M128_round_inv_corr);
  xmm6 = _mm_adds_epi16(xmm6, xmm4);
  src0 = xmm7;
  xmm6 = _mm_srai_epi16(xmm6, SHIFT_INV_COL);
  xmm7 = xmm1;
  xmm1 = _mm_adds_epi16(xmm1, xmm0);
  src1 = xmm6;
  xmm1 = _mm_srai_epi16(xmm1, SHIFT_INV_COL);
  xmm6 = src3;
  xmm7 = _mm_subs_epi16(xmm7, xmm0);
  xmm7 = _mm_srai_epi16(xmm7, SHIFT_INV_COL);
  src2 = xmm1;
  xmm5 = _mm_subs_epi16(xmm5, src7);
  xmm5 = _mm_srai_epi16(xmm5, SHIFT_INV_COL);
  src7 = xmm5;
  xmm3 = _mm_subs_epi16(xmm3, xmm4);
  xmm6 = _mm_adds_epi16(xmm6, xmm2);
  xmm2 = _mm_subs_epi16(xmm2, src3);
  xmm6 = _mm_srai_epi16(xmm6, SHIFT_INV_COL);
  xmm2 = _mm_srai_epi16(xmm2, SHIFT_INV_COL);
  src3 = xmm6;
  xmm3 = _mm_srai_epi16(xmm3, SHIFT_INV_COL);
  src4 = xmm2;
  src5 = xmm7;
  src6 = xmm3;
  __asm ret
}

#if defined(_M_IX86) && (defined(_MSC_VER) || defined(__INTEL_COMPILER))

void  __declspec(naked) dct_8_inv_row_asm () {
  _asm {
    push ebp
    mov ebp, esp

    pshuflw    xmm0, xmm0, 0xD8
    pshufd     xmm1, xmm0, 0
    pmaddwd    xmm1, [esi]
    pshufd     xmm3, xmm0, 0x55
    pshufhw    xmm0, xmm0, 0xD8
    pmaddwd    xmm3, [esi + 32]
    pshufd     xmm2, xmm0, 0xAA
    pshufd     xmm0, xmm0, 0xFF
    pmaddwd    xmm2, [esi + 16]
    pshufhw    xmm4, xmm4, 0xD8
    paddd      xmm1, M128_round_inv_row
    pshuflw    xmm4, xmm4, 0xD8
    pmaddwd    xmm0, [esi + 48]
    pshufd     xmm5, xmm4, 0
    pshufd     xmm6, xmm4, 0xAA
    pmaddwd    xmm5, [ecx]
    paddd      xmm1, xmm2
    movdqa     xmm2, xmm1
    pshufd     xmm7, xmm4, 0x55
    pmaddwd    xmm6, [ecx + 16]
    paddd      xmm0, xmm3
    pshufd     xmm4, xmm4, 0xFF
    psubd      xmm2, xmm0
    pmaddwd    xmm7, [ecx + 32]
    paddd      xmm0, xmm1
    psrad      xmm2, 12
    paddd      xmm5, M128_round_inv_row
    pmaddwd    xmm4, [ecx + 48]
    paddd      xmm5, xmm6
    movdqa     xmm6, xmm5
    psrad      xmm0, 12
    pshufd     xmm2, xmm2, 0x1B
    packssdw   xmm0, xmm2
    paddd      xmm4, xmm7
    psubd      xmm6, xmm4
    paddd      xmm4, xmm5
    psrad      xmm6, 12
    psrad      xmm4, 12
    pshufd     xmm6, xmm6, 0x1B
    packssdw   xmm4, xmm6

    mov esp, ebp
    pop ebp
    ret
  }
}

#define DCT_8_INV_COL_8_ASM														      \
    __asm movdqa     xmm1, XMMWORD PTR M128_tg_3_16	       	\
    __asm movdqa     xmm2, xmm0														  \
    __asm movdqa     xmm3, XMMWORD PTR [edx+3*16]		        \
    __asm pmulhw     xmm0, xmm1														  \
    __asm pmulhw     xmm1, xmm3														  \
    __asm movdqa     xmm5, XMMWORD PTR M128_tg_1_16		      \
    __asm movdqa     xmm6, xmm4														  \
    __asm pmulhw     xmm4, xmm5														  \
    __asm paddsw     xmm0, xmm2													  	\
    __asm pmulhw     xmm5, [edx+1*16]											  \
    __asm paddsw     xmm1, xmm3														  \
    __asm movdqa     xmm7, XMMWORD PTR [edx+6*16]		        \
    __asm paddsw     xmm0, xmm3					 									  \
    __asm movdqa     xmm3, XMMWORD PTR M128_tg_2_16		      \
    __asm psubsw     xmm2, xmm1														  \
    __asm pmulhw     xmm7, xmm3														  \
    __asm movdqa     xmm1, xmm0														  \
    __asm pmulhw     xmm3, [edx+2*16]												\
    __asm psubsw     xmm5, xmm6														  \
    __asm paddsw     xmm4, [edx+1*16]										  	\
    __asm paddsw     xmm0, xmm4														  \
    __asm paddsw     xmm0, XMMWORD PTR M128_one_corr		    \
    __asm psubsw     xmm4, xmm1												  		\
    __asm movdqa     xmm6, xmm5														  \
    __asm psubsw     xmm5, xmm2														  \
    __asm paddsw     xmm5, XMMWORD PTR M128_one_corr	    	\
    __asm paddsw     xmm6, xmm2														  \
    __asm movdqa     [edx+7*16], xmm0											  \
    __asm movdqa     xmm1, xmm4														  \
    __asm movdqa     xmm0, XMMWORD PTR M128_cos_4_16		    \
    __asm paddsw     xmm4, xmm5													  	\
    __asm movdqa     xmm2, XMMWORD PTR M128_cos_4_16		    \
    __asm pmulhw     xmm2, xmm4													  	\
    __asm movdqa     [edx+3*16], xmm6												\
    __asm psubsw     xmm1, xmm5													  	\
    __asm paddsw     xmm7, [edx+2*16]								   	  	\
    __asm psubsw     xmm3, [edx+6*16]												\
    __asm movdqa     xmm6, [edx]													  \
    __asm pmulhw     xmm0, xmm1														  \
    __asm movdqa     xmm5, [edx+4*16]											  \
    __asm paddsw     xmm5, xmm6														  \
    __asm psubsw     xmm6, [edx+4*16]											  \
    __asm paddsw     xmm4, xmm2														  \
    __asm por        xmm4, XMMWORD PTR M128_one_corr	     	\
    __asm paddsw     xmm0, xmm1														  \
    __asm por        xmm0, XMMWORD PTR M128_one_corr		    \
    __asm movdqa     xmm2, xmm5														  \
    __asm paddsw     xmm5, xmm7														  \
    __asm movdqa     xmm1, xmm6														  \
    __asm paddsw     xmm5, XMMWORD PTR M128_round_inv_col	  \
    __asm psubsw     xmm2, xmm7														  \
    __asm movdqa     xmm7, [edx+7*16]												\
    __asm paddsw     xmm6, xmm3														  \
    __asm paddsw     xmm6, XMMWORD PTR M128_round_inv_col	  \
    __asm paddsw     xmm7, xmm5														  \
    __asm psraw      xmm7, SHIFT_INV_COL										\
    __asm psubsw     xmm1, xmm3														  \
    __asm paddsw     xmm1, XMMWORD PTR M128_round_inv_corr	\
    __asm movdqa     xmm3, xmm6														  \
    __asm paddsw     xmm2, XMMWORD PTR M128_round_inv_corr	\
    __asm paddsw     xmm6, xmm4														  \
    __asm movdqa     [edx], xmm7												  	\
    __asm psraw      xmm6, SHIFT_INV_COL										\
    __asm movdqa     xmm7, xmm1													  	\
    __asm paddsw     xmm1, xmm0													  	\
    __asm movdqa     [edx+1*16], xmm6				  					  	\
    __asm psraw      xmm1, SHIFT_INV_COL										\
    __asm movdqa     xmm6, [edx+3*16]											  \
    __asm psubsw     xmm7, xmm0												  		\
    __asm psraw      xmm7, SHIFT_INV_COL								  	\
    __asm movdqa     [edx+2*16], xmm1									    	\
    __asm psubsw     xmm5, [edx+7*16]												\
    __asm psraw      xmm5, SHIFT_INV_COL										\
    __asm movdqa     [edx+7*16], xmm5												\
    __asm psubsw     xmm3, xmm4														  \
    __asm paddsw     xmm6, xmm2													  	\
    __asm psubsw     xmm2, [edx+3*16]												\
    __asm psraw      xmm6, SHIFT_INV_COL										\
    __asm psraw      xmm2, SHIFT_INV_COL								  	\
    __asm movdqa     [edx+3*16], xmm6											  \
    __asm psraw      xmm3, SHIFT_INV_COL										\
    __asm movdqa     [edx+4*16], xmm2												\
    __asm movdqa     [edx+5*16], xmm7												\
    __asm movdqa     [edx+6*16], xmm3

#define IDCT_SSE2_ASM(src) 

#endif


void mpeg2_idct_copy_sse2(int16_t * block, uint8_t * dest, int stride) {
#if defined(_MSC_VER)
  __m128i * src = (__m128i *) block;
  __m128i r0, r1, r2, r3;
  __m128i zero;

#ifdef IDCT_SSE2_ASM
  IDCT_SSE2_ASM(src);
#else
  IDCT_SSE2_SRC(src);
  IDCT_SSE2
#endif

  r0 = _mm_packus_epi16(src0, src1);
  r1 = _mm_packus_epi16(src2, src3);
  r2 = _mm_packus_epi16(src4, src5);
  r3 = _mm_packus_epi16(src6, src7);

  mm_storel_epi64(dest + 0 * stride, r0);
  mm_storeh_epi64(dest + 1 * stride, r0);
  mm_storel_epi64(dest + 2 * stride, r1);
  mm_storeh_epi64(dest + 3 * stride, r1);
  mm_storel_epi64(dest + 4 * stride, r2);
  mm_storeh_epi64(dest + 5 * stride, r2);
  mm_storel_epi64(dest + 6 * stride, r3);
  mm_storeh_epi64(dest + 7 * stride, r3);
#else

#endif
}

#if defined(MSVC_ASM) && (defined(ARCH_X86) || defined(ARCH_X86_64))

void mpeg2_idct_add_sse2(int last, int16_t * block, uint8_t * dest, int stride) {
#if defined(_MSC_VER)
  __m128i * src = (__m128i *) block;
  __m128i r0, r1, r2, r3, r4, r5, r6, r7;
  __m128i zero;
  
#ifdef ARCH_X86_64
#define IDCT_SSE2_SRC(src)						\
    __m128i xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;	\
    xmm8  = src[0];							\
    xmm9  = src[1];							\
    xmm10 = src[2];							\
    xmm11 = src[3];							\
    xmm12 = src[4];							\
    xmm13 = src[5];							\
    xmm14 = src[6];							\
    xmm15 = src[7];
#define src0 xmm8
#define src1 xmm9
#define src2 xmm10
#define src3 xmm11
#define src4 xmm12
#define src5 xmm13
#define src6 xmm14
#define src7 xmm15
#else
#define IDCT_SSE2_SRC(src)
#define src0 src[0]
#define src1 src[1]
#define src2 src[2]
#define src3 src[3]
#define src4 src[4]
#define src5 src[5]
#define src6 src[6]
#define src7 src[7]
#endif
#ifdef IDCT_SSE2_ASM
#else
  IDCT_SSE2
#endif

  r0 = _mm_loadl_epi64((__m128i*)(dest + 0 * stride));
  r1 = _mm_loadl_epi64((__m128i*)(dest + 1 * stride));
  r2 = _mm_loadl_epi64((__m128i*)(dest + 2 * stride));
  r3 = _mm_loadl_epi64((__m128i*)(dest + 3 * stride));
  r4 = _mm_loadl_epi64((__m128i*)(dest + 4 * stride));
  r5 = _mm_loadl_epi64((__m128i*)(dest + 5 * stride));
  r6 = _mm_loadl_epi64((__m128i*)(dest + 6 * stride));
  r7 = _mm_loadl_epi64((__m128i*)(dest + 7 * stride));

  zero = _mm_setzero_si128();

  r0 = _mm_unpacklo_epi8(r0, zero);
  r1 = _mm_unpacklo_epi8(r1, zero);
  r2 = _mm_unpacklo_epi8(r2, zero);
  r3 = _mm_unpacklo_epi8(r3, zero);
  r4 = _mm_unpacklo_epi8(r4, zero);
  r5 = _mm_unpacklo_epi8(r5, zero);
  r6 = _mm_unpacklo_epi8(r6, zero);
  r7 = _mm_unpacklo_epi8(r7, zero);

  r0 = _mm_adds_epi16(src0, r0);
  r1 = _mm_adds_epi16(src1, r1);
  r2 = _mm_adds_epi16(src2, r2);
  r3 = _mm_adds_epi16(src3, r3);
  r4 = _mm_adds_epi16(src4, r4);
  r5 = _mm_adds_epi16(src5, r5);
  r6 = _mm_adds_epi16(src6, r6);
  r7 = _mm_adds_epi16(src7, r7);

  r0 = _mm_packus_epi16(r0, r1);
  r1 = _mm_packus_epi16(r2, r3);
  r2 = _mm_packus_epi16(r4, r5);
  r3 = _mm_packus_epi16(r6, r7);

  mm_storel_epi64(dest + 0 * stride, r0);
  mm_storeh_epi64(dest + 1 * stride, r0);
  mm_storel_epi64(dest + 2 * stride, r1);
  mm_storeh_epi64(dest + 3 * stride, r1);
  mm_storel_epi64(dest + 4 * stride, r2);
  mm_storeh_epi64(dest + 5 * stride, r2);
  mm_storel_epi64(dest + 6 * stride, r3);
  mm_storeh_epi64(dest + 7 * stride, r3);

  sse2_block_zero(block);
#else
  if (last != 129 || (block[0] & (7 << 4)) == (4 << 4)) {
    sse2_idct(block);
    sse2_block_add(block, dest, stride);
    sse2_block_zero(block);
  } else
    block_add_DC(block, dest, stride, CPU_MMXEXT);
#endif
}

void mpeg2_idct_copy_mmxext(int16_t * const block, uint8_t * const dest, const int stride) {
  mmxext_idct(block);
  block_copy(block, dest, stride);
  block_zero(block);
}

void mpeg2_idct_add_mmxext(const int last, int16_t * const block, uint8_t * const dest, const int stride) {
  if (last != 129 || (block[0] & (7 << 4)) == (4 << 4)) {
    mmxext_idct(block);
    block_add(block, dest, stride);
    block_zero(block);
  } else {
    block_add_DC(block, dest, stride, CPU_MMXEXT);
  }
}

void mpeg2_idct_copy_mmx(int16_t * const block, uint8_t * const dest, const int stride) {
  mmx_idct(block);
  block_copy(block, dest, stride);
  block_zero(block);
}

void mpeg2_idct_add_mmx(const int last, int16_t * const block, uint8_t * const dest, const int stride) {
  if (last != 129 || (block[0] & (7 << 4)) == (4 << 4)) {
    mmx_idct(block);
    block_add(block, dest, stride);
    block_zero(block);
  } else {
    block_add_DC(block, dest, stride, CPU_MMX);
  }
}

void mpeg2_idct_mmx_init(void) {
  int i, j;

  /* the mmx/mmxext idct uses a reordered input, so we patch scan tables */
  for (i = 0; i < 64; i++) {
    j = mpeg2_scan_norm[i];
    mpeg2_scan_norm[i] = (j & 0x38) | ((j & 6) >> 1) | ((j & 1) << 2);
    j = mpeg2_scan_alt[i];
    mpeg2_scan_alt[i] = (j & 0x38) | ((j & 6) >> 1) | ((j & 1) << 2);
  }
}
#endif

#endif
