/*
 * motion_comp_alpha.c
 * Copyright (C) 2002 Falk Hueffner <falk@debian.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#ifdef ARCH_ALPHA

#include <inttypes.h>

#include "mpeg2.h"
#include "mpeg2_internal.h"
#include "alpha_asm.h"

static inline uint64_t avg2(uint64_t a, uint64_t b)
{
    return (a | b) - (((a ^ b) & BYTE_VEC(0xfe)) >> 1);    
}

#define OP8(LOAD, STORE)			\
    do {					\
	STORE(LOAD(pixels), block);		\
	pixels += line_size;			\
	block += line_size;			\
    } while (--h)

#define OP16(LOAD, STORE)			\
    do {					\
	STORE(LOAD(pixels), block);		\
	STORE(LOAD(pixels + 8), (block) + 8);	\
	pixels += line_size;			\
	block += line_size;			\
    } while (--h)

#define OP8_X2(LOAD, STORE)				\
    do {						\
	uint64_t p0, p1;				\
							\
	p0 = LOAD(pixels);				\
	p1 = p0 >> 8 | ((uint64_t) pixels[8] << 56);	\
	STORE(avg2(p0, p1), block);			\
	pixels += line_size;				\
	block += line_size;				\
    } while (--h)

#define OP16_X2(LOAD, STORE)					\
    do {							\
	uint64_t p0, p1;					\
								\
	p0 = LOAD(pixels);					\
	p1 = LOAD(pixels + 8);					\
	STORE(avg2(p0, p0 >> 8 | p1 << 56), block);		\
	STORE(avg2(p1, p1 >> 8 | (uint64_t) pixels[16] << 56),	\
	      block + 8);					\
	pixels += line_size;					\
	block += line_size;					\
    } while (--h)

#define OP8_Y2(LOAD, STORE)			\
    do {					\
	uint64_t pix = LOAD(pixels);		\
	do {					\
	    uint64_t np;			\
						\
	    pixels += line_size;		\
	    np = LOAD(pixels);			\
	    STORE(avg2(pix, np), block);	\
	    block += line_size;			\
	    pix = np;				\
	} while (--h);				\
    } while (0)

#define OP16_Y2(LOAD, STORE)			\
    do {					\
	uint64_t p0 = LOAD(pixels);		\
	uint64_t p1 = LOAD(pixels + 8);		\
	do {					\
	    uint64_t np0, np1;			\
						\
	    pixels += line_size;		\
	    np0 = LOAD(pixels);			\
	    np1 = LOAD(pixels + 8);		\
	    STORE(avg2(p0, np0), block);	\
	    STORE(avg2(p1, np1), block + 8);	\
	    block += line_size;			\
	    p0 = np0;				\
	    p1 = np1;				\
	} while (--h);				\
    } while (0)

#define OP8_XY2(LOAD, STORE)					\
    do {							\
	uint64_t pl, ph;					\
	uint64_t p1 = LOAD(pixels);				\
	uint64_t p2 = p1 >> 8 | ((uint64_t) pixels[8] << 56);	\
								\
	ph = ((p1 & ~BYTE_VEC(0x03)) >> 2)			\
	   + ((p2 & ~BYTE_VEC(0x03)) >> 2);			\
	pl = (p1 & BYTE_VEC(0x03))				\
	   + (p2 & BYTE_VEC(0x03));				\
								\
	do {							\
	    uint64_t npl, nph;					\
								\
	    pixels += line_size;				\
	    p1 = LOAD(pixels);					\
	    p2 = (p1 >> 8) | ((uint64_t) pixels[8] << 56);	\
	    nph = ((p1 & ~BYTE_VEC(0x03)) >> 2)			\
	        + ((p2 & ~BYTE_VEC(0x03)) >> 2);		\
	    npl = (p1 & BYTE_VEC(0x03))				\
	        + (p2 & BYTE_VEC(0x03));			\
								\
	    STORE(ph + nph					\
		  + (((pl + npl + BYTE_VEC(0x02)) >> 2)		\
		     & BYTE_VEC(0x03)), block);			\
								\
	    block += line_size;					\
            pl = npl;						\
	    ph = nph;						\
	} while (--h);						\
    } while (0)

#define OP16_XY2(LOAD, STORE)					\
    do {							\
	uint64_t pl_l, ph_l, pl_r, ph_r;			\
	uint64_t p0 = LOAD(pixels);				\
	uint64_t p2 = LOAD(pixels + 8);				\
	uint64_t p1 = p0 >> 8 | (p2 << 56);			\
	uint64_t p3 = p2 >> 8 | ((uint64_t) pixels[16] << 56);	\
								\
	ph_l = ((p0 & ~BYTE_VEC(0x03)) >> 2)			\
	     + ((p1 & ~BYTE_VEC(0x03)) >> 2);			\
	pl_l = (p0 & BYTE_VEC(0x03))				\
	     + (p1 & BYTE_VEC(0x03));				\
	ph_r = ((p2 & ~BYTE_VEC(0x03)) >> 2)			\
	     + ((p3 & ~BYTE_VEC(0x03)) >> 2);			\
	pl_r = (p2 & BYTE_VEC(0x03))				\
	     + (p3 & BYTE_VEC(0x03));				\
								\
	do {							\
	    uint64_t npl_l, nph_l, npl_r, nph_r;		\
								\
	    pixels += line_size;				\
	    p0 = LOAD(pixels);					\
	    p2 = LOAD(pixels + 8);				\
	    p1 = p0 >> 8 | (p2 << 56);				\
	    p3 = p2 >> 8 | ((uint64_t) pixels[16] << 56);	\
	    nph_l = ((p0 & ~BYTE_VEC(0x03)) >> 2)		\
		  + ((p1 & ~BYTE_VEC(0x03)) >> 2);		\
	    npl_l = (p0 & BYTE_VEC(0x03))			\
		  + (p1 & BYTE_VEC(0x03));			\
	    nph_r = ((p2 & ~BYTE_VEC(0x03)) >> 2)		\
		  + ((p3 & ~BYTE_VEC(0x03)) >> 2);		\
	    npl_r = (p2 & BYTE_VEC(0x03))			\
		  + (p3 & BYTE_VEC(0x03));			\
								\
	    STORE(ph_l + nph_l					\
		  + (((pl_l + npl_l + BYTE_VEC(0x02)) >> 2)	\
		     & BYTE_VEC(0x03)), block);			\
	    STORE(ph_r + nph_r					\
		  + (((pl_r + npl_r + BYTE_VEC(0x02)) >> 2)	\
		     & BYTE_VEC(0x03)), block + 8);		\
								\
	    block += line_size;					\
	    pl_l = npl_l;					\
	    ph_l = nph_l;					\
	    pl_r = npl_r;					\
	    ph_r = nph_r;					\
	} while (--h);						\
    } while (0)

#define MAKE_OP(OPNAME, SIZE, SUFF, OPKIND, STORE)			\
static void MC_ ## OPNAME ## _ ## SUFF ## _ ## SIZE ## _alpha		\
	(uint8_t *restrict block, const uint8_t *restrict pixels,	\
	 int line_size, int h)						\
{									\
    if ((uint64_t) pixels & 0x7) {					\
	OPKIND(uldq, STORE);						\
    } else {								\
	OPKIND(ldq, STORE);						\
    }									\
}

#define PIXOP(OPNAME, STORE)			\
    MAKE_OP(OPNAME, 8,  o,  OP8,      STORE);	\
    MAKE_OP(OPNAME, 8,  x,  OP8_X2,   STORE);	\
    MAKE_OP(OPNAME, 8,  y,  OP8_Y2,   STORE);	\
    MAKE_OP(OPNAME, 8,  xy, OP8_XY2,  STORE);	\
    MAKE_OP(OPNAME, 16, o,  OP16,     STORE);	\
    MAKE_OP(OPNAME, 16, x,  OP16_X2,  STORE);	\
    MAKE_OP(OPNAME, 16, y,  OP16_Y2,  STORE);	\
    MAKE_OP(OPNAME, 16, xy, OP16_XY2, STORE);

#define STORE(l, b) stq(l, b)
PIXOP(put, STORE);

#undef STORE
#define STORE(l, b) stq(avg2(l, ldq(b)), b);
PIXOP(avg, STORE);

mpeg2_mc_t mpeg2_mc_alpha = {
    { MC_put_o_16_alpha, MC_put_x_16_alpha,
      MC_put_y_16_alpha, MC_put_xy_16_alpha,
      MC_put_o_8_alpha, MC_put_x_8_alpha,
      MC_put_y_8_alpha, MC_put_xy_8_alpha },
    { MC_avg_o_16_alpha, MC_avg_x_16_alpha,
      MC_avg_y_16_alpha, MC_avg_xy_16_alpha,
      MC_avg_o_8_alpha, MC_avg_x_8_alpha,
      MC_avg_y_8_alpha, MC_avg_xy_8_alpha }
};

#endif
