/*
 * motion_comp_c.c
 *
 * Copyright (C) Martin Norbäck <d95mback@dtek.chalmers.se> - Jan 2000
 *
 * Hackage by:
 * Michel LESPINASSE <walken@windriver.com>
 * Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
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
 * 
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include "mpeg2.h"
#include "mpeg2_internal.h"

#include "motion_comp.h"

//
// All the heavy lifting for motion_comp.c is done in here.
//
// This should serve as a reference implementation. Optimized versions
// in assembler are a must for speed.

#define avg2(a,b) ((a+b+1)>>1)
#define avg4(a,b,c,d) ((a+b+c+d+2)>>2)

#define predict(i) (ref[i])
#define predict_x(i) (avg2 (ref[i], ref[i+1]))
#define predict_y(i) (avg2 (ref[i], (ref+stride)[i]))
#define predict_xy(i) (avg4 (ref[i], ref[i+1], (ref+stride)[i], (ref+stride)[i+1]))

#define put(predictor,i) dest[i] = predictor (i)
#define avg(predictor,i) dest[i] = avg2 (predictor (i), dest[i])

// mc function template

#define MC_FUNC(op,xy)						\
static void motion_comp_##op####xy##_16x16_c (uint8_t * dest,	\
					      uint8_t * ref,	\
					      int stride,	\
					      int height)	\
{								\
    do {							\
	op (predict##xy, 0);					\
	op (predict##xy, 1);					\
	op (predict##xy, 2);					\
	op (predict##xy, 3);					\
	op (predict##xy, 4);					\
	op (predict##xy, 5);					\
	op (predict##xy, 6);					\
	op (predict##xy, 7);					\
	op (predict##xy, 8);					\
	op (predict##xy, 9);					\
	op (predict##xy, 10);					\
	op (predict##xy, 11);					\
	op (predict##xy, 12);					\
	op (predict##xy, 13);					\
	op (predict##xy, 14);					\
	op (predict##xy, 15);					\
	ref += stride;						\
	dest += stride;						\
    } while (--height);						\
}								\
static void motion_comp_##op####xy##_8x8_c (uint8_t * dest,	\
					    uint8_t * ref,	\
					    int stride,		\
					    int height)		\
{								\
    do {							\
	op (predict##xy, 0);					\
	op (predict##xy, 1);					\
	op (predict##xy, 2);					\
	op (predict##xy, 3);					\
	op (predict##xy, 4);					\
	op (predict##xy, 5);					\
	op (predict##xy, 6);					\
	op (predict##xy, 7);					\
	ref += stride;						\
	dest += stride;						\
    } while (--height);						\
}

// definitions of the actual mc functions

MC_FUNC (put,)
MC_FUNC (avg,)
MC_FUNC (put,_x)
MC_FUNC (avg,_x)
MC_FUNC (put,_y)
MC_FUNC (avg,_y)
MC_FUNC (put,_xy)
MC_FUNC (avg,_xy)

MOTION_COMP_EXTERN (c)
