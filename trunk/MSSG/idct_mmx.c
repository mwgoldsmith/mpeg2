#include <inttypes.h>

#define ATTR_ALIGN(x)

#include "mmx.h"

#define ROW_SHIFT 15
#define COL_SHIFT 6

#define round(bias) ((int)(((bias)+0.5) * (1<<ROW_SHIFT)))
#define rounder(bias) {round (bias), round (bias)}

/* MMX row IDCT */

#define mmx_table(c1,c2,c3,c4,c5,c6,c7)	{  c4,  c2,  c4,  c6,	\
					   c4,  c6, -c4, -c2,	\
					   c1,  c3,  c3, -c7,	\
					   c5,  c7, -c1, -c5,	\
					   c4, -c6,  c4, -c2,	\
					  -c4,  c2,  c4, -c6,	\
					   c5, -c1,  c7, -c5,	\
					   c7,  c3,  c3, -c1 }

static inline void mmx_row_head (int16_t * const row, const int offset,
				 const int16_t * const table)
{
    movq_m2r (*(row+offset), mm2);	/* mm2 = x6 x4 x2 x0 */

    movq_m2r (*(row+offset+4), mm5);	/* mm5 = x7 x5 x3 x1 */
    movq_r2r (mm2, mm0);		/* mm0 = x6 x4 x2 x0 */

    movq_m2r (*table, mm3);		/* mm3 = C6 C4 C2 C4 */
    movq_r2r (mm5, mm6);		/* mm6 = x7 x5 x3 x1 */

    punpckldq_r2r (mm0, mm0);		/* mm0 = x2 x0 x2 x0 */

    movq_m2r (*(table+4), mm4);		/* mm4 = -C2 -C4 C6 C4 */
    pmaddwd_r2r (mm0, mm3);		/* mm3 = C4*x0+C6*x2 C4*x0+C2*x2 */

    movq_m2r (*(table+8), mm1);		/* mm1 = -C7 C3 C3 C1 */
    punpckhdq_r2r (mm2, mm2);		/* mm2 = x6 x4 x6 x4 */
}

static inline void mmx_row (const int16_t * const table,
			    const int32_t * const rounder)
{
    pmaddwd_r2r (mm2, mm4);		/* mm4 = -C4*x4-C2*x6 C4*x4+C6*x6 */
    punpckldq_r2r (mm5, mm5);		/* mm5 = x3 x1 x3 x1 */

    pmaddwd_m2r (*(table+16), mm0);	/* mm0 = C4*x0-C2*x2 C4*x0-C6*x2 */
    punpckhdq_r2r (mm6, mm6);		/* mm6 = x7 x5 x7 x5 */

    movq_m2r (*(table+12), mm7);	/* mm7 = -C5 -C1 C7 C5 */
    pmaddwd_r2r (mm5, mm1);		/* mm1 = C3*x1-C7*x3 C1*x1+C3*x3 */

    paddd_m2r (*rounder, mm3);		/* mm3 += rounder */
    pmaddwd_r2r (mm6, mm7);		/* mm7 = -C1*x5-C5*x7 C5*x5+C7*x7 */

    pmaddwd_m2r (*(table+20), mm2);	/* mm2 = C4*x4-C6*x6 -C4*x4+C2*x6 */
    paddd_r2r (mm4, mm3);		/* mm3 = a1 a0 + rounder */

    pmaddwd_m2r (*(table+24), mm5);	/* mm5 = C7*x1-C5*x3 C5*x1-C1*x3 */
    movq_r2r (mm3, mm4);		/* mm4 = a1 a0 + rounder */

    pmaddwd_m2r (*(table+28), mm6);	/* mm6 = C3*x5-C1*x7 C7*x5+C3*x7 */
    paddd_r2r (mm7, mm1);		/* mm1 = b1 b0 */

    paddd_m2r (*rounder, mm0);		/* mm0 += rounder */
    psubd_r2r (mm1, mm3);		/* mm3 = a1-b1 a0-b0 + rounder */

    psrad_i2r (ROW_SHIFT, mm3);		/* mm3 = y6 y7 */
    paddd_r2r (mm4, mm1);		/* mm1 = a1+b1 a0+b0 + rounder */

    paddd_r2r (mm2, mm0);		/* mm0 = a3 a2 + rounder */
    psrad_i2r (ROW_SHIFT, mm1);		/* mm1 = y1 y0 */

    paddd_r2r (mm6, mm5);		/* mm5 = b3 b2 */
    movq_r2r (mm0, mm7);		/* mm7 = a3 a2 + rounder */

    paddd_r2r (mm5, mm0);		/* mm0 = a3+b3 a2+b2 + rounder */
    psubd_r2r (mm5, mm7);		/* mm7 = a3-b3 a2-b2 + rounder */
}

static inline void mmx_row_tail (int16_t * const row, const int store)
{
    psrad_i2r (ROW_SHIFT, mm0);		/* mm0 = y3 y2 */

    psrad_i2r (ROW_SHIFT, mm7);		/* mm7 = y4 y5 */

    packssdw_r2r (mm0, mm1);		/* mm1 = y3 y2 y1 y0 */

    packssdw_r2r (mm3, mm7);		/* mm7 = y6 y7 y4 y5 */

    movq_r2m (mm1, *(row+store));	/* save y3 y2 y1 y0 */
    movq_r2r (mm7, mm4);		/* mm4 = y6 y7 y4 y5 */

    pslld_i2r (16, mm7);		/* mm7 = y7 0 y5 0 */

    psrld_i2r (16, mm4);		/* mm4 = 0 y6 0 y4 */

    por_r2r (mm4, mm7);			/* mm7 = y7 y6 y5 y4 */

    /* slot */

    movq_r2m (mm7, *(row+store+4));	/* save y7 y6 y5 y4 */
}

static inline void mmx_row_mid (int16_t * const row, const int store,
				const int offset, const int16_t * const table)
{
    movq_m2r (*(row+offset), mm2);	/* mm2 = x6 x4 x2 x0 */
    psrad_i2r (ROW_SHIFT, mm0);		/* mm0 = y3 y2 */

    movq_m2r (*(row+offset+4), mm5);	/* mm5 = x7 x5 x3 x1 */
    psrad_i2r (ROW_SHIFT, mm7);		/* mm7 = y4 y5 */

    packssdw_r2r (mm0, mm1);		/* mm1 = y3 y2 y1 y0 */
    movq_r2r (mm5, mm6);		/* mm6 = x7 x5 x3 x1 */

    packssdw_r2r (mm3, mm7);		/* mm7 = y6 y7 y4 y5 */
    movq_r2r (mm2, mm0);		/* mm0 = x6 x4 x2 x0 */

    movq_r2m (mm1, *(row+store));	/* save y3 y2 y1 y0 */
    movq_r2r (mm7, mm1);		/* mm1 = y6 y7 y4 y5 */

    punpckldq_r2r (mm0, mm0);		/* mm0 = x2 x0 x2 x0 */
    psrld_i2r (16, mm7);		/* mm7 = 0 y6 0 y4 */

    movq_m2r (*table, mm3);		/* mm3 = C6 C4 C2 C4 */
    pslld_i2r (16, mm1);		/* mm1 = y7 0 y5 0 */

    movq_m2r (*(table+4), mm4);		/* mm4 = -C2 -C4 C6 C4 */
    por_r2r (mm1, mm7);			/* mm7 = y7 y6 y5 y4 */

    movq_m2r (*(table+8), mm1);		/* mm1 = -C7 C3 C3 C1 */
    punpckhdq_r2r (mm2, mm2);		/* mm2 = x6 x4 x6 x4 */

    movq_r2m (mm7, *(row+store+4));	/* save y7 y6 y5 y4 */
    pmaddwd_r2r (mm0, mm3);		/* mm3 = C4*x0+C6*x2 C4*x0+C2*x2 */
}

/* MMX column IDCT */
static inline void idct_col (int16_t * const col, const int offset)
{
#define T1 13036
#define T2 27146
#define T3 43790
#define C4 23170

    static const short _T1[] ATTR_ALIGN(8) = {T1,T1,T1,T1};
    static const short _T2[] ATTR_ALIGN(8) = {T2,T2,T2,T2};
    static const short _T3[] ATTR_ALIGN(8) = {T3,T3,T3,T3};
    static const short _C4[] ATTR_ALIGN(8) = {C4,C4,C4,C4};

    /* column code adapted from peter gubanov */
    /* http://www.elecard.com/peter/idct.shtml */

    movq_m2r (*_T1, mm0);		/* mm0 = T1 */

    movq_m2r (*(col+offset+1*8), mm1);	/* mm1 = x1 */
    movq_r2r (mm0, mm2);		/* mm2 = T1 */

    movq_m2r (*(col+offset+7*8), mm4);	/* mm4 = x7 */
    pmulhw_r2r (mm1, mm0);		/* mm0 = T1*x1 */

    movq_m2r (*_T3, mm5);		/* mm5 = T3 */
    pmulhw_r2r (mm4, mm2);		/* mm2 = T1*x7 */

    movq_m2r (*(col+offset+5*8), mm6);	/* mm6 = x5 */
    movq_r2r (mm5, mm7);		/* mm7 = T3-1 */

    movq_m2r (*(col+offset+3*8), mm3);	/* mm3 = x3 */
    psubsw_r2r (mm4, mm0);		/* mm0 = v17 */

    movq_m2r (*_T2, mm4);		/* mm4 = T2 */
    pmulhw_r2r (mm3, mm5);		/* mm5 = (T3-1)*x3 */

    paddsw_r2r (mm2, mm1);		/* mm1 = u17 */
    pmulhw_r2r (mm6, mm7);		/* mm7 = (T3-1)*x5 */

    /* slot */

    movq_r2r (mm4, mm2);		/* mm2 = T2 */
    paddsw_r2r (mm3, mm5);		/* mm5 = T3*x3 */

    pmulhw_m2r (*(col+offset+2*8), mm4);/* mm4 = T2*x2 */
    paddsw_r2r (mm6, mm7);		/* mm7 = T3*x5 */

    psubsw_r2r (mm6, mm5);		/* mm5 = v35 */
    paddsw_r2r (mm3, mm7);		/* mm7 = u35 */

    movq_m2r (*(col+offset+6*8), mm3);	/* mm3 = x6 */
    movq_r2r (mm0, mm6);		/* mm6 = v17 */

    pmulhw_r2r (mm3, mm2);		/* mm2 = T2*x6 */
    psubsw_r2r (mm5, mm0);		/* mm0 = b3 */

    psubsw_r2r (mm3, mm4);		/* mm4 = v26 */
    paddsw_r2r (mm6, mm5);		/* mm5 = v12 */

    movq_r2m (mm0, *(col+offset+3*8));	/* save b3 in scratch0 */
    movq_r2r (mm1, mm6);		/* mm6 = u17 */

    paddsw_m2r (*(col+offset+2*8), mm2);/* mm2 = u26 */
    paddsw_r2r (mm7, mm6);		/* mm6 = b0 */

    psubsw_r2r (mm7, mm1);		/* mm1 = u12 */
    movq_r2r (mm1, mm7);		/* mm7 = u12 */

    movq_m2r (*(col+offset+0*8), mm3);	/* mm3 = x0 */
    paddsw_r2r (mm5, mm1);		/* mm1 = u12+v12 */

    movq_m2r (*_C4, mm0);		/* mm0 = C4/2 */
    psubsw_r2r (mm5, mm7);		/* mm7 = u12-v12 */

    movq_r2m (mm6, *(col+offset+5*8));	/* save b0 in scratch1 */
    pmulhw_r2r (mm0, mm1);		/* mm1 = b1/2 */

    movq_r2r (mm4, mm6);		/* mm6 = v26 */
    pmulhw_r2r (mm0, mm7);		/* mm7 = b2/2 */

    movq_m2r (*(col+offset+4*8), mm5);	/* mm5 = x4 */
    movq_r2r (mm3, mm0);		/* mm0 = x0 */

    psubsw_r2r (mm5, mm3);		/* mm3 = v04 */
    paddsw_r2r (mm5, mm0);		/* mm0 = u04 */

    paddsw_r2r (mm3, mm4);		/* mm4 = a1 */
    movq_r2r (mm0, mm5);		/* mm5 = u04 */

    psubsw_r2r (mm6, mm3);		/* mm3 = a2 */
    paddsw_r2r (mm2, mm5);		/* mm5 = a0 */

    paddsw_r2r (mm1, mm1);		/* mm1 = b1 */
    psubsw_r2r (mm2, mm0);		/* mm0 = a3 */

    paddsw_r2r (mm7, mm7);		/* mm7 = b2 */
    movq_r2r (mm3, mm2);		/* mm2 = a2 */

    movq_r2r (mm4, mm6);		/* mm6 = a1 */
    paddsw_r2r (mm7, mm3);		/* mm3 = a2+b2 */

    psraw_i2r (COL_SHIFT, mm3);		/* mm3 = y2 */
    paddsw_r2r (mm1, mm4);		/* mm4 = a1+b1 */

    psraw_i2r (COL_SHIFT, mm4);		/* mm4 = y1 */
    psubsw_r2r (mm1, mm6);		/* mm6 = a1-b1 */

    movq_m2r (*(col+offset+5*8), mm1);	/* mm1 = b0 */
    psubsw_r2r (mm7, mm2);		/* mm2 = a2-b2 */

    psraw_i2r (COL_SHIFT, mm6);		/* mm6 = y6 */
    movq_r2r (mm5, mm7);		/* mm7 = a0 */

    movq_r2m (mm4, *(col+offset+1*8));	/* save y1 */
    psraw_i2r (COL_SHIFT, mm2);		/* mm2 = y5 */

    movq_r2m (mm3, *(col+offset+2*8));	/* save y2 */
    paddsw_r2r (mm1, mm5);		/* mm5 = a0+b0 */

    movq_m2r (*(col+offset+3*8), mm4);	/* mm4 = b3 */
    psubsw_r2r (mm1, mm7);		/* mm7 = a0-b0 */

    psraw_i2r (COL_SHIFT, mm5);		/* mm5 = y0 */
    movq_r2r (mm0, mm3);		/* mm3 = a3 */

    movq_r2m (mm2, *(col+offset+5*8));	/* save y5 */
    psubsw_r2r (mm4, mm3);		/* mm3 = a3-b3 */

    psraw_i2r (COL_SHIFT, mm7);		/* mm7 = y7 */
    paddsw_r2r (mm0, mm4);		/* mm4 = a3+b3 */

    movq_r2m (mm5, *(col+offset+0*8));	/* save y0 */
    psraw_i2r (COL_SHIFT, mm3);		/* mm3 = y4 */

    movq_r2m (mm6, *(col+offset+6*8));	/* save y6 */
    psraw_i2r (COL_SHIFT, mm4);		/* mm4 = y3 */

    movq_r2m (mm7, *(col+offset+7*8));	/* save y7 */

    movq_r2m (mm3, *(col+offset+4*8));	/* save y4 */

    movq_r2m (mm4, *(col+offset+3*8));	/* save y3 */
}

static const int32_t rounder0[] ATTR_ALIGN(8) =
    rounder ((1 << (COL_SHIFT - 1)) - 0.5);
static const int32_t rounder4[] ATTR_ALIGN(8) = rounder (0);
static const int32_t rounder1[] ATTR_ALIGN(8) =
    rounder (1.25683487303);	/* C1*(C1/C4+C1+C7)/2 */
static const int32_t rounder7[] ATTR_ALIGN(8) =
    rounder (-0.25);		/* C1*(C7/C4+C7-C1)/2 */
static const int32_t rounder2[] ATTR_ALIGN(8) =
    rounder (0.60355339059);	/* C2 * (C6+C2)/2 */
static const int32_t rounder6[] ATTR_ALIGN(8) =
    rounder (-0.25);		/* C2 * (C6-C2)/2 */
static const int32_t rounder3[] ATTR_ALIGN(8) =
    rounder (0.087788325588);	/* C3*(-C3/C4+C3+C5)/2 */
static const int32_t rounder5[] ATTR_ALIGN(8) =
    rounder (-0.441341716183);	/* C3*(-C5/C4+C5-C3)/2 */


#define declare_idct(idct,table,idct_row_head,idct_row,idct_row_tail,idct_row_mid)	\
static inline void idct (int16_t * const block)				\
{									\
    static const int16_t table04[] ATTR_ALIGN(16) =			\
	table (22725, 21407, 19266, 16384, 12873,  8867, 4520);		\
    static const int16_t table17[] ATTR_ALIGN(16) =			\
	table (31521, 29692, 26722, 22725, 17855, 12299, 6270);		\
    static const int16_t table26[] ATTR_ALIGN(16) =			\
	table (29692, 27969, 25172, 21407, 16819, 11585, 5906);		\
    static const int16_t table35[] ATTR_ALIGN(16) =			\
	table (26722, 25172, 22654, 19266, 15137, 10426, 5315);		\
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

declare_idct (mmx_idct, mmx_table,
	      mmx_row_head, mmx_row, mmx_row_tail, mmx_row_mid)

int WKN;

void idct_mmx (int16_t * const block, int comp)
{
    int16_t block2[64];
    int i, j;

    if ((WKN & (1 << comp)) && (block[0] & 7) != 4) {
	WKN &= ~(1 << comp);
	j = (block[0] + 4) >> 3;
	for (i = 0; i < 64; i++)
	    block[i] = j;
	return;
    }
    WKN &= ~(1 << comp);
    for (i = 0; i < 64; i++)
	block2[i] = block[i];
    for (i = 0; i < 64; i++) {
	int j = (i & 0x38) | ((i & 6) >> 1) | ((i & 1) << 2);
	block[j] = block2[i] << 4;
    }
    mmx_idct (block);
    emms ();
}