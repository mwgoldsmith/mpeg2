/*
 * idct_mmx.c
 *
 * Copyright (C) Aaron Holtzman <aholtzma@ess.engr.uvic.ca> - Nov 1999
 *
 * Portions of this code are from the MPEG software simulation group
 * idct implementation. This code will be replaced with a new
 * implementation soon.
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 *	
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Make; see the file COPYING. If not, write to
 * the Free Software Foundation, 
 *
 */

#include <stdio.h>
#include <mmx.h>
#include <sse.h>
#include "mpeg2.h"
#include "mpeg2_internal.h"

#include "idct.h"

//#define SSE	// only define if you have SSE

#define ROW_SHIFT 12
#define COL_SHIFT 5

#define round(bias) ((int)(((bias)+0.5) * (1<<ROW_SHIFT)))

#if 0
// C row IDCT - its just here to document the SSE and MMX versions
#define table(c1,c2,c3,c4,c5,c6,c7)	{ 0, c1, c2, c3, c4, c5, c6, c7 }
#define rounder(bias) {round (bias)}
static inline void idct_row (int16_t * in, int16_t * out,
			     int16_t * table, int32_t * rounder)
{
    int C1, C2, C3, C4, C5, C6, C7;
    int a0, a1, a2, a3, b0, b1, b2, b3;

    C1 = table[1];
    C2 = table[2];
    C3 = table[3];
    C4 = table[4];
    C5 = table[5];
    C6 = table[6];
    C7 = table[7];

    a0 = C4*in[0] + C2*in[2] + C4*in[4] + C6*in[6] + *rounder;
    a1 = C4*in[0] + C6*in[2] - C4*in[4] - C2*in[6] + *rounder;
    a2 = C4*in[0] - C6*in[2] - C4*in[4] + C2*in[6] + *rounder;
    a3 = C4*in[0] - C2*in[2] + C4*in[4] - C6*in[6] + *rounder;

    b0 = C1*in[1] + C3*in[3] + C5*in[5] + C7*in[7];
    b1 = C3*in[1] - C7*in[3] - C1*in[5] - C5*in[7];
    b2 = C5*in[1] - C1*in[3] + C7*in[5] + C3*in[7];
    b3 = C7*in[1] - C5*in[3] + C3*in[5] - C1*in[7];

    out[0] = (a0 + b0) >> ROW_SHIFT;
    out[1] = (a1 + b1) >> ROW_SHIFT;
    out[2] = (a2 + b2) >> ROW_SHIFT;
    out[3] = (a3 + b3) >> ROW_SHIFT;
    out[4] = (a3 - b3) >> ROW_SHIFT;
    out[5] = (a2 - b2) >> ROW_SHIFT;
    out[6] = (a1 - b1) >> ROW_SHIFT;
    out[7] = (a0 - b0) >> ROW_SHIFT;
}
#endif


#ifdef SSE

// SSE row IDCT
#define table(c1,c2,c3,c4,c5,c6,c7)	{  c4,  c2,  c4,  c6,	\
					   c4,  c6, -c4, -c2,	\
					   c1,  c3,  c3, -c7,	\
					   c5,  c7, -c1, -c5,	\
					   c4, -c6,  c4, -c2,	\
					  -c4,  c2,  c4, -c6,	\
					   c5, -c1,  c7, -c5,	\
					   c7,  c3,  c3, -c1 }
#define rounder(bias) {round (bias), round (bias)}
static inline void idct_row (int16_t * in, int16_t * out,
			     int16_t * table, int32_t * rounder)
{
    movq_m2r (*in, mm0);		// mm0 = x3 x2 x1 x0
    movq_m2r (*(in+4), mm1);		// mm1 = x7 x6 x5 x4
    movq_r2r (mm0, mm2);		// mm2 = x3 x2 x1 x0
    movq_m2r (*table, mm3);		// mm3 = C6 C4 C2 C4
    pshufw_r2r (mm0, mm0, 0x88);	// mm0 = x2 x0 x2 x0
    movq_m2r (*(table+4), mm4);		// mm4 = -C2 -C4 C6 C4
    movq_r2r (mm1, mm5);		// mm5 = x7 x6 x5 x4
    pmaddwd_r2r (mm0, mm3);		// mm3 = C4*x0+C6*x2 C4*x0+C2*x2
    movq_m2r (*(table+8), mm6);		// mm6 = -C7 C3 C3 C1
    pshufw_r2r (mm1, mm1, 0x88);	// mm1 = x6 x4 x6 x4
    pmaddwd_r2r (mm1, mm4);		// mm4 = -C4*x4-C2*x6 C4*x4+C6*x6
    movq_m2r (*(table+12), mm7);	// mm7 = -C5 -C1 C7 C5
    pshufw_r2r (mm2, mm2, 0xdd);	// mm2 = x3 x1 x3 x1
    pmaddwd_r2r (mm2, mm6);		// mm6 = C3*x1-C7*x3 C1*x1+C3*x3
    pshufw_r2r (mm5, mm5, 0xdd);	// mm5 = x7 x5 x7 x5
    pmaddwd_r2r (mm5, mm7);		// mm7 = -C1*x5-C5*x7 C5*x5+C7*x7
    paddd_m2r (*rounder, mm3);		// mm3 += rounder
    pmaddwd_m2r (*(table+16), mm0);	// mm0 = C4*x0-C2*x2 C4*x0-C6*x2
    paddd_r2r (mm4, mm3);		// mm3 = a1 a0 + rounder
    pmaddwd_m2r (*(table+20), mm1);	// mm1 = C4*x4-C6*x6 -C4*x4+C2*x6
    movq_r2r (mm3, mm4);		// mm4 = a1 a0 + rounder
    pmaddwd_m2r (*(table+24), mm2);	// mm2 = C7*x1-C5*x3 C5*x1-C1*x3
    paddd_r2r (mm7, mm6);		// mm6 = b1 b0
    pmaddwd_m2r (*(table+28), mm5);	// mm5 = C3*x5-C1*x7 C7*x5+C3*x7
    paddd_r2r (mm6, mm3);		// mm3 = a1+b1 a0+b0 + rounder
    paddd_m2r (*rounder, mm0);		// mm0 += rounder
    psrad_i2r (ROW_SHIFT, mm3);		// mm3 = y1 y0
    paddd_r2r (mm1, mm0);		// mm0 = a3 a2 + rounder
    psubd_r2r (mm6, mm4);		// mm4 = a1-b1 a0-b0 + rounder
    movq_r2r (mm0, mm7);		// mm7 = a3 a2 + rounder
    paddd_r2r (mm5, mm2);		// mm2 = b3 b2
    paddd_r2r (mm2, mm0);		// mm0 = a3+b3 a2+b2 + rounder
    psrad_i2r (ROW_SHIFT, mm4);		// mm4 = y6 y7
    psubd_r2r (mm2, mm7);		// mm7 = a3-b3 a2-b2 + rounder
    psrad_i2r (ROW_SHIFT, mm0);		// mm0 = y3 y2
    psrad_i2r (ROW_SHIFT, mm7);		// mm7 = y4 y5
    packssdw_r2r (mm0, mm3);		// mm3 = y3 y2 y1 y0
    packssdw_r2r (mm4, mm7);		// mm7 = y6 y7 y4 y5
    movq_r2m (mm3, *out);		// save y3 y2 y1 y0
    pshufw_r2r (mm7, mm7, 0xb1);	// y7 y6 y5 y4
    movq_r2m (mm7, *(out+4));		// save y7 y6 y5 y4
}

#else

// MMX row IDCT
#define table(c1,c2,c3,c4,c5,c6,c7)	{  c4,  c4,  c4, -c4,	\
					   c2,  c6,  c6, -c2,	\
					   c1,  c5,  c3, -c1,	\
					   c3,  c7, -c7, -c5,	\
					   c4, -c4,  c4,  c4,	\
					  -c6,  c2, -c2, -c6,	\
					   c5,  c7,  c7,  c3,	\
					   -c1,  c3, -c5, -c1 }
#define rounder(bias) {round (bias), round (bias)}
static inline void idct_row (int16_t * in, int16_t * out,
			     int16_t * table, int32_t * rounder)
{
    movq_m2r (*in, mm0);		// mm0 = x3 x2 x1 x0
    movq_m2r (*(in+4), mm1);		// mm1 = x7 x6 x5 x4
    movq_r2r (mm0, mm2);		// mm2 = x3 x2 x1 x0
    movq_m2r (*table, mm3);		// mm3 = -C4 C4 C4 C4
    punpcklwd_r2r (mm1, mm0);		// mm0 = x5 x1 x4 x0
    movq_r2r (mm0, mm5);		// mm5 = x5 x1 x4 x0
    punpckldq_r2r (mm0, mm0);		// mm0 = x4 x0 x4 x0
    movq_m2r (*(table+4), mm4);		// mm4 = -C2 C6 C6 C2
    punpckhwd_r2r (mm1, mm2);		// mm2 = x7 x3 x6 x2
    pmaddwd_r2r (mm0, mm3);		// mm3 = C4*x0-C4*x4 C4*x0+C4*x4
    movq_r2r (mm2, mm6);		// mm6 = x7 x3 x6 x2
    movq_m2r (*(table+8), mm1);		// mm1 = -C1 C3 C5 C1
    punpckldq_r2r (mm2, mm2);		// mm2 = x6 x2 x6 x2
    pmaddwd_r2r (mm2, mm4);		// mm4 = C6*x2-C2*x6 C2*x2+C6*x6
    punpckhdq_r2r (mm5, mm5);		// mm5 = x5 x1 x5 x1
    pmaddwd_m2r (*(table+16), mm0);	// mm0 = C4*x0+C4*x4 C4*x0-C4*x4
    punpckhdq_r2r (mm6, mm6);		// mm6 = x7 x3 x7 x3
    movq_m2r (*(table+12), mm7);	// mm7 = -C5 -C7 C7 C3
    pmaddwd_r2r (mm5, mm1);		// mm1 = C3*x1-C1*x5 C1*x1+C5*x5
    paddd_m2r (*rounder, mm3);		// mm3 += rounder
    pmaddwd_r2r (mm6, mm7);		// mm7 = -C7*x3-C5*x7 C3*x3+C7*x7
    pmaddwd_m2r (*(table+20), mm2);	// mm2 = -C2*x2-C6*x6 -C6*x2+C2*x6
    paddd_r2r (mm4, mm3);		// mm3 = a1 a0 + rounder
    pmaddwd_m2r (*(table+24), mm5);	// mm5 = C7*x1+C3*x5 C5*x1+C7*x5
    movq_r2r (mm3, mm4);		// mm4 = a1 a0 + rounder
    pmaddwd_m2r (*(table+28), mm6);	// mm6 = -C5*x3-C1*x7 -C1*x3+C3*x7
    paddd_r2r (mm7, mm1);		// mm1 = b1 b0
    paddd_m2r (*rounder, mm0);		// mm0 += rounder
    psubd_r2r (mm1, mm3);		// mm3 = a1-b1 a0-b0 + rounder
    psrad_i2r (ROW_SHIFT, mm3);		// mm3 = y6 y7
    paddd_r2r (mm4, mm1);		// mm1 = a1+b1 a0+b0 + rounder
    paddd_r2r (mm2, mm0);		// mm0 = a3 a2 + rounder
    psrad_i2r (ROW_SHIFT, mm1);		// mm1 = y1 y0
    paddd_r2r (mm6, mm5);		// mm5 = b3 b2
    movq_r2r (mm0, mm4);		// mm4 = a3 a2 + rounder
    paddd_r2r (mm5, mm0);		// mm0 = a3+b3 a2+b2 + rounder
    psubd_r2r (mm5, mm4);		// mm4 = a3-b3 a2-b2 + rounder
    psrad_i2r (ROW_SHIFT, mm0);		// mm0 = y3 y2
    psrad_i2r (ROW_SHIFT, mm4);		// mm4 = y4 y5
    packssdw_r2r (mm0, mm1);		// mm1 = y3 y2 y1 y0
    packssdw_r2r (mm3, mm4);		// mm4 = y6 y7 y4 y5
    movq_r2r (mm4, mm7);		// mm7 = y6 y7 y4 y5
    psrld_i2r (16, mm4);		// mm4 = 0 y6 0 y4
    pslld_i2r (16, mm7);		// mm7 = y7 0 y5 0
    movq_r2m (mm1, *out);		// save y3 y2 y1 y0
    por_r2r (mm4, mm7);			// mm7 = y7 y6 y5 y4
    movq_r2m (mm7, *(out+4));		// save y7 y6 y5 y4
}

#endif


#if 0
// C column IDCT - its just here to document the SSE and MMX versions
static inline void idct_col (int16_t * in, int16_t * out)
{
// multiplication - as implemented on mmx
#define F(c,x) (((c) * (x)) >> 16)

// saturation - it helps us handle torture test cases
#define S(x) (((x)>32767) ? 32767 : ((x)<-32768) ? -32768 : (x))

#define T1 13036
#define T2 27146
#define T3 43790
#define C4 23170

    int16_t x0, x1, x2, x3, x4, x5, x6, x7;
    int16_t y0, y1, y2, y3, y4, y5, y6, y7;
    int16_t a0, a1, a2, a3, b0, b1, b2, b3;
    int16_t u04, v04, u26, v26, u17, v17, u35, v35, u12, v12;

    x0 = in[0*8];
    x1 = in[1*8];
    x2 = in[2*8];
    x3 = in[3*8];
    x4 = in[4*8];
    x5 = in[5*8];
    x6 = in[6*8];
    x7 = in[7*8];

    u04 = S (x0 + x4);
    v04 = S (x0 - x4);
    u26 = S (F (T2, x6) + x2);	// -0.5
    v26 = S (F (T2, x2) - x6);	// -0.5

    a0 = S (u04 + u26);
    a1 = S (v04 + v26);
    a2 = S (v04 - v26);
    a3 = S (u04 - u26);

    u17 = S (F (T1, x7) + x1);	// -0.5
    v17 = S (F (T1, x1) - x7);	// -0.5
    u35 = S (F (T3, x5) + x3);	// -0.5
    v35 = S (F (T3, x3) - x5);	// -0.5

    b0 = S (u17 + u35);
    b3 = S (v17 - v35);
    u12 = S (u17 - u35);
    v12 = S (v17 + v35);
    u12 = S (2 * F (C4, u12));	// -0.5
    v12 = S (2 * F (C4, v12));	// -0.5
    b1 = S (u12 + v12);
    b2 = S (u12 - v12);

    y0 = S (a0 + b0) >> COL_SHIFT;
    y1 = S (a1 + b1) >> COL_SHIFT;
    y2 = S (a2 + b2) >> COL_SHIFT;
    y3 = S (a3 + b3) >> COL_SHIFT;

    y4 = S (a3 - b3) >> COL_SHIFT;
    y5 = S (a2 - b2) >> COL_SHIFT;
    y6 = S (a1 - b1) >> COL_SHIFT;
    y7 = S (a0 - b0) >> COL_SHIFT;

    out[0*8] = y0;
    out[1*8] = y1;
    out[2*8] = y2;
    out[3*8] = y3;
    out[4*8] = y4;
    out[5*8] = y5;
    out[6*8] = y6;
    out[7*8] = y7;
}
#endif


// MMX column IDCT
static inline void idct_col (int16_t * in, int16_t * out)
{
#define T1 13036
#define T2 27146
#define T3 43790
#define C4 23170

    static mmx_t scratch0, scratch1;
    static short _T1[] ALIGN_8_BYTE = {T1,T1,T1,T1};
    static short _T2[] ALIGN_8_BYTE = {T2,T2,T2,T2};
    static short _T3[] ALIGN_8_BYTE = {T3,T3,T3,T3};
    static short _C4[] ALIGN_8_BYTE = {C4,C4,C4,C4};

    /* column code adapted from peter gubanov */
    /* http://www.elecard.com/peter/idct.shtml */

    movq_m2r (*_T1, mm0);		// mm0 = T1
    movq_m2r (*(in+1*8), mm1);		// mm1 = x1
    movq_r2r (mm0, mm2);		// mm2 = T1
    movq_m2r (*(in+7*8), mm4);		// mm4 = x7
    pmulhw_r2r (mm1, mm0);		// mm0 = T1*x1
    movq_m2r (*_T3, mm5);		// mm5 = T3
    pmulhw_r2r (mm4, mm2);		// mm2 = T1*x7
    movq_m2r (*(in+5*8), mm6);		// mm6 = x5
    movq_r2r (mm5, mm7);		// mm7 = T3-1
    movq_m2r (*(in+3*8), mm3);		// mm3 = x3
    psubsw_r2r (mm4, mm0);		// mm0 = v17
    movq_m2r (*_T2, mm4);		// mm4 = T2
    pmulhw_r2r (mm3, mm5);		// mm5 = (T3-1)*x3
    paddsw_r2r (mm2, mm1);		// mm1 = u17
    pmulhw_r2r (mm6, mm7);		// mm7 = (T3-1)*x5
    movq_r2r (mm4, mm2);		// mm2 = T2
    paddsw_r2r (mm3, mm5);		// mm5 = T3*x3
    pmulhw_m2r (*(in+2*8), mm4);	// mm4 = T2*x2
    paddsw_r2r (mm6, mm7);		// mm7 = T3*x5
    psubsw_r2r (mm6, mm5);		// mm5 = v35
    paddsw_r2r (mm3, mm7);		// mm7 = u35
    movq_m2r (*(in+6*8), mm3);		// mm3 = x6
    movq_r2r (mm0, mm6);		// mm6 = v17
    pmulhw_r2r (mm3, mm2);		// mm2 = T2*x6
    psubsw_r2r (mm5, mm0);		// mm0 = b3
    psubsw_r2r (mm3, mm4);		// mm4 = v26
    paddsw_r2r (mm6, mm5);		// mm5 = v12
    movq_r2m (mm0, *&scratch0);		// save b3
    movq_r2r (mm1, mm6);		// mm6 = u17
    paddsw_m2r (*(in+2*8), mm2);	// mm2 = u26
    paddsw_r2r (mm7, mm6);		// mm6 = b0
    psubsw_r2r (mm7, mm1);		// mm1 = u12
    movq_r2r (mm1, mm7);		// mm7 = u12
    movq_m2r (*(in+0*8), mm3);		// mm3 = x0
    paddsw_r2r (mm5, mm1);		// mm1 = u12+v12
    movq_m2r (*_C4, mm0);		// mm0 = C4/2
    paddsw_r2r (mm1, mm1);		// mm1 = 2*(u12+v12)
    psubsw_r2r (mm5, mm7);		// mm7 = u12-v12
    pmulhw_r2r (mm0, mm1);		// mm1 = b1
    movq_r2m (mm6, *&scratch1);		// save b0
    paddsw_r2r (mm7, mm7);		// mm7 = 2*(u12-v12)
    movq_r2r (mm4, mm6);		// mm6 = v26
    pmulhw_r2r (mm0, mm7);		// mm7 = b2
    movq_m2r (*(in+4*8), mm5);		// mm5 = x4
    movq_r2r (mm3, mm0);		// mm0 = x0
    psubsw_r2r (mm5, mm3);		// mm3 = v04
    paddsw_r2r (mm3, mm4);		// mm4 = a1
    paddsw_r2r (mm5, mm0);		// mm0 = u04
    psubsw_r2r (mm6, mm3);		// mm3 = a2
    movq_r2r (mm0, mm5);		// mm5 = u04
    paddsw_r2r (mm2, mm5);		// mm5 = a0
    psubsw_r2r (mm2, mm0);		// mm0 = a3
    movq_r2r (mm3, mm2);		// mm2 = a2
    movq_r2r (mm4, mm6);		// mm6 = a1
    paddsw_r2r (mm7, mm3);		// mm3 = a2+b2
    psraw_i2r (COL_SHIFT, mm3);		// mm3 = y2
    paddsw_r2r (mm1, mm4);		// mm4 = a1+b1
    psraw_i2r (COL_SHIFT, mm4);		// mm4 = y1
    psubsw_r2r (mm1, mm6);		// mm6 = a1-b1
    movq_m2r (*&scratch1, mm1);		// mm1 = b0
    psubsw_r2r (mm7, mm2);		// mm2 = a2-b2
    psraw_i2r (COL_SHIFT, mm6);		// mm6 = y6
    movq_r2r (mm5, mm7);		// mm7 = a0
    movq_r2m (mm4, *(out+1*8));		// save y1
    psraw_i2r (COL_SHIFT, mm2);		// mm2 = y5
    movq_r2m (mm3, *(out+2*8));		// save y2
    paddsw_r2r (mm1, mm5);		// mm5 = a0+b0
    movq_m2r (*&scratch0, mm4);		// mm4 = b3
    psubsw_r2r (mm1, mm7);		// mm7 = a0-b0
    psraw_i2r (COL_SHIFT, mm5);		// mm5 = y0
    movq_r2r (mm0, mm3);		// mm3 = a3
    movq_r2m (mm2, *(out+5*8));		// save y5
    psubsw_r2r (mm4, mm3);		// mm3 = a3-b3
    psraw_i2r (COL_SHIFT, mm7);		// mm7 = y7
    paddsw_r2r (mm0, mm4);		// mm4 = a3+b3
    movq_r2m (mm5, *(out+0*8));		// save y0
    psraw_i2r (COL_SHIFT, mm3);		// mm3 = y4
    movq_r2m (mm6, *(out+6*8));		// save y6
    psraw_i2r (COL_SHIFT, mm4);		// mm4 = y3
    movq_r2m (mm7, *(out+7*8));		// save y7
    movq_r2m (mm3, *(out+4*8));		// save y4
    movq_r2m (mm4, *(out+3*8));		// save y3
}


static int16_t table04[] ALIGN_16_BYTE =
    table (22725, 21407, 19266, 16384, 12873,  8867, 4520);
static int32_t rounder0[] ALIGN_8_BYTE =
    rounder ((1 << (COL_SHIFT - 1)) - 0.5);
static int32_t rounder4[] ALIGN_8_BYTE = rounder (0);

static int16_t table17[] ALIGN_16_BYTE =
    table (31521, 29692, 26722, 22725, 17855, 12299, 6270);
static int32_t rounder1[] ALIGN_8_BYTE =
    rounder (0.916737807126);	// C1*(C1/(4*C4)+(C1+C7)/2)
static int32_t rounder7[] ALIGN_8_BYTE =
    rounder (-0.317649512522);	// C1*(C7/(4*C4)+(C7-C1)/2)

static int16_t table26[] ALIGN_16_BYTE =
    table (29692, 27969, 25172, 21407, 16819, 11585, 5906);
static int32_t rounder2[] ALIGN_8_BYTE =
    rounder (0.60355339059);	// C2 * (C6+C2)/2
static int32_t rounder6[] ALIGN_8_BYTE =
    rounder (-0.25);		// C2 * (C6-C2)/2

static int16_t table35[] ALIGN_16_BYTE =
    table (26722, 25172, 22654, 19266, 15137, 10426, 5315);
static int32_t rounder3[] ALIGN_8_BYTE =
    rounder (0.332214533403);	// C3*(-C3/(4*C4)+(C3+C5)/2)
static int32_t rounder5[] ALIGN_8_BYTE =
    rounder (-0.278021345574);	// C3*(-C5/(4*C4)+(C5-C3)/2)

void idct_block_copy_mmx (int16_t * block, uint8_t * dest, int stride)
{
    int i;

    idct_row (block + 0*8, block + 0*8, table04, rounder0);
    idct_row (block + 4*8, block + 4*8, table04, rounder4);
    idct_row (block + 1*8, block + 1*8, table17, rounder1);
    idct_row (block + 7*8, block + 7*8, table17, rounder7);
    idct_row (block + 2*8, block + 2*8, table26, rounder2);
    idct_row (block + 6*8, block + 6*8, table26, rounder6);
    idct_row (block + 3*8, block + 3*8, table35, rounder3);
    idct_row (block + 5*8, block + 5*8, table35, rounder5);

    idct_col (block, block);
    idct_col (block + 4, block + 4);

    i = 8;
    do {
	movq_m2r (*block, mm1);
	movq_m2r (*(block+4), mm2);
	packuswb_r2r (mm2, mm1);
	movq_r2m (mm1, *dest);

	block += 8;
	dest += stride;
    } while (--i);
}

void idct_block_add_mmx (int16_t * block, uint8_t * dest, int stride)
{
    int i;

    idct_row (block + 0*8, block + 0*8, table04, rounder0);
    idct_row (block + 4*8, block + 4*8, table04, rounder4);
    idct_row (block + 1*8, block + 1*8, table17, rounder1);
    idct_row (block + 7*8, block + 7*8, table17, rounder7);
    idct_row (block + 2*8, block + 2*8, table26, rounder2);
    idct_row (block + 6*8, block + 6*8, table26, rounder6);
    idct_row (block + 3*8, block + 3*8, table35, rounder3);
    idct_row (block + 5*8, block + 5*8, table35, rounder5);

    idct_col (block, block);
    idct_col (block + 4, block + 4);

    pxor_r2r (mm0, mm0);
    i = 8;
    do {
	movq_m2r (*dest, mm1);
	movq_r2r (mm1, mm2);
	punpcklbw_r2r (mm0, mm1);
	punpckhbw_r2r (mm0, mm2);
	movq_m2r (*block, mm3);
	paddsw_r2r (mm3, mm1);
	movq_m2r (*(block+4), mm4);
	paddsw_r2r (mm4, mm2);
	packuswb_r2r (mm2, mm1);
	movq_r2m (mm1, *dest);

	block += 8;
	dest += stride;
    } while (--i);
}
