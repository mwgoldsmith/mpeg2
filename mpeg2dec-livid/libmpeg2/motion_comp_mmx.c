/*
 *  motion_comp_mmx.c
 *
 *  Intel MMX implementation of motion_comp_c routines.
 *  MMX code written by David I. Lehn <dlehn@vt.edu>.
 *  lib{mmx,xmmx,sse} can be found at http://shay.ecn.purdue.edu/~swar/
 *
 *  Copyright (C) Aaron Holtzman <aholtzma@ess.engr.uvic.ca> - Nov 1999
 *
 *  This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 *	
 *  mpeg2dec is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  mpeg2dec is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#include <mmx.h>
#include "debug.h"
#include "mpeg2.h"
#include "mpeg2_internal.h"

#include "motion_comp.h"



// some rounding constants
mmx_t round1 = {0x0001000100010001LL};
mmx_t round4 = {0x0002000200020002LL};

/*
 * This code should probably be compiled with loop unrolling
 * (ie, -funroll-loops in gcc) becuase some of the loops
 * use a small static number of iterations.  This was written
 * with the assumption the compiler knows best about when
 * unrolling will help
 */

static inline void
mmx_zero_reg()
{
   // load 0 into mm0
   pxor_r2r(mm0,mm0);
}

static inline void
mmx_average_2_U8(uint8_t *dst, uint8_t *src1, uint8_t *src2)
{
   //
   // *dst = clip_to_u8((*src1 + *src2 + 1)/2);
   //

   //mmx_zero_reg();

   movq_m2r(*src1,mm1);        // load 8 src1 bytes
   movq_r2r(mm1,mm2);          // copy 8 src1 bytes

   movq_m2r(*src2,mm3);        // load 8 src2 bytes
   movq_r2r(mm3,mm4);          // copy 8 src2 bytes

   punpcklbw_r2r(mm0,mm1);     // unpack low src1 bytes
   punpckhbw_r2r(mm0,mm2);     // unpack high src1 bytes

   punpcklbw_r2r(mm0,mm3);     // unpack low src2 bytes
   punpckhbw_r2r(mm0,mm4);     // unpack high src2 bytes

   paddw_r2r(mm3,mm1);         // add lows to mm1
   paddw_m2r(round1,mm1);
   psraw_i2r(1,mm1);           // /2

   paddw_r2r(mm4,mm2);         // add highs to mm2
   paddw_m2r(round1,mm2);
   psraw_i2r(1,mm2);           // /2

   packuswb_r2r(mm2,mm1);      // pack (w/ saturation)
   movq_r2m(mm1,*dst);         // store result in dst
}

static inline void
mmx_interp_average_2_U8(uint8_t *dst, uint8_t *src1, uint8_t *src2)
{
   //
   // *dst = clip_to_u8((*dst + (*src1 + *src2 + 1)/2 + 1)/2);
   //

   //mmx_zero_reg();

   movq_m2r(*dst,mm1);            // load 8 dst bytes
   movq_r2r(mm1,mm2);             // copy 8 dst bytes

   movq_m2r(*src1,mm3);           // load 8 src1 bytes
   movq_r2r(mm3,mm4);             // copy 8 src1 bytes

   movq_m2r(*src2,mm5);           // load 8 src2 bytes
   movq_r2r(mm5,mm6);             // copy 8 src2 bytes

   punpcklbw_r2r(mm0,mm1);        // unpack low dst bytes
   punpckhbw_r2r(mm0,mm2);        // unpack high dst bytes

   punpcklbw_r2r(mm0,mm3);        // unpack low src1 bytes
   punpckhbw_r2r(mm0,mm4);        // unpack high src1 bytes

   punpcklbw_r2r(mm0,mm5);        // unpack low src2 bytes
   punpckhbw_r2r(mm0,mm6);        // unpack high src2 bytes

   paddw_r2r(mm5,mm3);            // add lows
   paddw_m2r(round1,mm3);
   psraw_i2r(1,mm3);              // /2

   paddw_r2r(mm6,mm4);            // add highs
   paddw_m2r(round1,mm4);
   psraw_i2r(1,mm4);              // /2

   paddw_r2r(mm3,mm1);            // add lows
   paddw_m2r(round1,mm1);
   psraw_i2r(1,mm1);              // /2

   paddw_r2r(mm4,mm2);            // add highs
   paddw_m2r(round1,mm2);
   psraw_i2r(1,mm2);              // /2

   packuswb_r2r(mm2,mm1);         // pack (w/ saturation)
   movq_r2m(mm1,*dst);            // store result in dst
}

static inline void
mmx_average_4_U8(uint8_t *dst, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *src4)
{
   //
   // *dst = clip_to_u8((*src1 + *src2 + *src3 + *src4 + 2)/4);
   //

   //mmx_zero_reg();

   movq_m2r(*src1,mm1);                // load 8 src1 bytes
   movq_r2r(mm1,mm2);                  // copy 8 src1 bytes

   punpcklbw_r2r(mm0,mm1);             // unpack low src1 bytes
   punpckhbw_r2r(mm0,mm2);             // unpack high src1 bytes

   movq_m2r(*src2,mm3);                // load 8 src2 bytes
   movq_r2r(mm3,mm4);                  // copy 8 src2 bytes

   punpcklbw_r2r(mm0,mm3);             // unpack low src2 bytes
   punpckhbw_r2r(mm0,mm4);             // unpack high src2 bytes

   paddw_r2r(mm3,mm1);                 // add lows
   paddw_r2r(mm4,mm2);                 // add highs

   // now have partials in mm1 and mm2

   movq_m2r(*src3,mm3);                // load 8 src3 bytes
   movq_r2r(mm3,mm4);                  // copy 8 src3 bytes

   punpcklbw_r2r(mm0,mm3);             // unpack low src3 bytes
   punpckhbw_r2r(mm0,mm4);             // unpack high src3 bytes

   paddw_r2r(mm3,mm1);                 // add lows
   paddw_r2r(mm4,mm2);                 // add highs

   movq_m2r(*src4,mm5);                // load 8 src4 bytes
   movq_r2r(mm5,mm6);                  // copy 8 src4 bytes

   punpcklbw_r2r(mm0,mm5);             // unpack low src4 bytes
   punpckhbw_r2r(mm0,mm6);             // unpack high src4 bytes

   paddw_r2r(mm5,mm1);                 // add lows
   paddw_r2r(mm6,mm2);                 // add highs

   // now have subtotal in mm1 and mm2

   paddw_m2r(round4,mm1);
   psraw_i2r(2,mm1);                   // /4
   paddw_m2r(round4,mm2);
   psraw_i2r(2,mm2);                   // /4

   packuswb_r2r(mm2,mm1);              // pack (w/ saturation)
   movq_r2m(mm1,*dst);                 // store result in dst
}

static inline void
mmx_interp_average_4_U8(uint8_t *dst, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *src4)
{
   //
   // *dst = clip_to_u8((*dst + (*src1 + *src2 + *src3 + *src4 + 2)/4 + 1)/2);
   //

   //mmx_zero_reg();

   movq_m2r(*src1,mm1);                // load 8 src1 bytes
   movq_r2r(mm1,mm2);                  // copy 8 src1 bytes

   punpcklbw_r2r(mm0,mm1);             // unpack low src1 bytes
   punpckhbw_r2r(mm0,mm2);             // unpack high src1 bytes

   movq_m2r(*src2,mm3);                // load 8 src2 bytes
   movq_r2r(mm3,mm4);                  // copy 8 src2 bytes

   punpcklbw_r2r(mm0,mm3);             // unpack low src2 bytes
   punpckhbw_r2r(mm0,mm4);             // unpack high src2 bytes

   paddw_r2r(mm3,mm1);                 // add lows
   paddw_r2r(mm4,mm2);                 // add highs

   // now have partials in mm1 and mm2

   movq_m2r(*src3,mm3);                // load 8 src3 bytes
   movq_r2r(mm3,mm4);                  // copy 8 src3 bytes

   punpcklbw_r2r(mm0,mm3);             // unpack low src3 bytes
   punpckhbw_r2r(mm0,mm4);             // unpack high src3 bytes

   paddw_r2r(mm3,mm1);                 // add lows
   paddw_r2r(mm4,mm2);                 // add highs

   movq_m2r(*src4,mm5);                // load 8 src4 bytes
   movq_r2r(mm5,mm6);                  // copy 8 src4 bytes

   punpcklbw_r2r(mm0,mm5);             // unpack low src4 bytes
   punpckhbw_r2r(mm0,mm6);             // unpack high src4 bytes

   paddw_r2r(mm5,mm1);                 // add lows
   paddw_r2r(mm6,mm2);                 // add highs

   paddw_m2r(round4,mm1);
   psraw_i2r(2,mm1);                   // /4
   paddw_m2r(round4,mm2);
   psraw_i2r(2,mm2);                   // /4

   // now have subtotal/4 in mm1 and mm2

   movq_m2r(*dst,mm3);                 // load 8 dst bytes
   movq_r2r(mm3,mm4);                  // copy 8 dst bytes

   punpcklbw_r2r(mm0,mm3);             // unpack low dst bytes
   punpckhbw_r2r(mm0,mm4);             // unpack high dst bytes

   paddw_r2r(mm3,mm1);                 // add lows
   paddw_r2r(mm4,mm2);                 // add highs

   paddw_m2r(round1,mm1);
   psraw_i2r(1,mm1);                   // /2
   paddw_m2r(round1,mm2);
   psraw_i2r(1,mm2);                   // /2

   // now have end value in mm1 and mm2

   packuswb_r2r(mm2,mm1);              // pack (w/ saturation)
   movq_r2m(mm1,*dst);                 // store result in dst
}

//-----------------------------------------------------------------------

static inline void 
motion_comp_avg_mmx( const uint8_t width, const int32_t height, uint8_t *curr_block,
      uint8_t *ref_block, const int32_t stride)
{
	int y;

	mmx_zero_reg();

	for (y = 0; y < height; y++) 
	{
		mmx_average_2_U8(curr_block, curr_block, ref_block);

		if(width == 16)
			mmx_average_2_U8(curr_block + 8, curr_block + 8, ref_block + 8);

		curr_block += stride;
		ref_block  += stride;
	}
}

void
motion_comp_avg_16x16_mmx( uint8_t *curr_block, uint8_t *ref_block, const int32_t stride, 
		const int32_t height)
{
   motion_comp_avg_mmx( 16, height, curr_block, ref_block, stride);
}

void
motion_comp_avg_8x8_mmx( uint8_t *curr_block, uint8_t *ref_block, const int32_t stride, 
		const int32_t height)
{
   motion_comp_avg_mmx( 8, height, curr_block, ref_block, stride);
}

//-----------------------------------------------------------------------

static inline void
motion_comp_put_mmx( const uint8_t width, const int32_t height, uint8_t *curr_block, 
		uint8_t *ref_block, const int32_t stride)
{
	int y;

	mmx_zero_reg();

	for (y = 0; y < height; y++) 
	{
		movq_m2r(*ref_block,mm1);    // load 8 ref bytes
		movq_r2m(mm1,*curr_block);   // store 8 bytes at curr

		if(width == 16)
		{
			movq_m2r(*(ref_block + 8),mm1);    // load 8 ref bytes
			movq_r2m(mm1,*(curr_block+8));   // store 8 bytes at curr
		}

		curr_block += stride;
		ref_block  += stride;
	}
}

void
motion_comp_put_16x16_mmx( uint8_t *curr_block, uint8_t *ref_block, const int32_t stride, const int32_t height)
{
   motion_comp_put_mmx( 16, height, curr_block, ref_block, stride);
}

void
motion_comp_put_8x8_mmx( uint8_t *curr_block, uint8_t *ref_block, const int32_t stride, const int32_t height)
{
   motion_comp_put_mmx( 8, height, curr_block, ref_block, stride);
}

//-----------------------------------------------------------------------

// Half pixel interpolation in the x direction
static inline void 
motion_comp_avg_x_mmx( const uint8_t width, const int32_t height, uint8_t *curr_block, 
		uint8_t *ref_block, const int32_t stride) 
{
	int y;

	mmx_zero_reg();

	for (y = 0; y < height; y++) 
	{
		mmx_interp_average_2_U8(curr_block, ref_block, ref_block + 1);

		if(width == 16)
			mmx_interp_average_2_U8(curr_block + 8, ref_block + 8, ref_block + 9);

		curr_block += stride;
		ref_block  += stride;
	}
}

void 
motion_comp_avg_x_16x16_mmx( uint8_t *curr_block, uint8_t *ref_block, const int32_t frame_stride, 
		const int32_t height) 
{
   motion_comp_avg_x_mmx( 16, height, curr_block, ref_block, frame_stride);
}

void 
motion_comp_avg_x_8x8_mmx( uint8_t *curr_block, uint8_t *ref_block, const int32_t frame_stride, 
		const int32_t height) 
{
   motion_comp_avg_x_mmx( 8, height, curr_block, ref_block, frame_stride);
}

//-----------------------------------------------------------------------

static inline void
motion_comp_put_x_mmx( const uint8_t width, const int32_t height, uint8_t *curr_block, uint8_t *ref_block, 
		int32_t stride) 
{
	int y;

	mmx_zero_reg();

	for (y = 0; y < height; y++) 
	{
		mmx_average_2_U8(curr_block, ref_block, ref_block + 1);

		if(width == 16)
			mmx_average_2_U8(curr_block + 8, ref_block + 8, ref_block + 9);

		curr_block += stride;
		ref_block  += stride;
	}
}

void 
motion_comp_put_x_16x16_mmx( uint8_t *curr_block, uint8_t *ref_block, const int32_t frame_stride, 
		const int32_t height) 
{
   motion_comp_put_x_mmx( 16, height, curr_block, ref_block, frame_stride);
}

void 
motion_comp_put_x_8x8_mmx( uint8_t *curr_block, uint8_t *ref_block, const int32_t frame_stride, 
		const int32_t height) 
{
   motion_comp_put_x_mmx( 8, height, curr_block, ref_block, frame_stride);
}


//-----------------------------------------------------------------------

static inline void 
motion_comp_avg_xy_mmx( const uint8_t width, const int32_t height, uint8_t *curr_block, uint8_t *ref_block, 
		const int32_t stride) 
{
	int y;
	uint8_t *ref_block_next = ref_block + stride;

	mmx_zero_reg();

	for (y = 0; y < height; y++) 
	{
		mmx_interp_average_4_U8(curr_block, ref_block, ref_block + 1, ref_block_next, ref_block_next + 1);

		if(width == 16)
			mmx_interp_average_4_U8(curr_block + 8, ref_block + 8, ref_block + 9, ref_block_next + 8, ref_block_next + 9);

		curr_block     += stride;
		ref_block      += stride;
		ref_block_next += stride;
	}
}

void 
motion_comp_avg_xy_16x16_mmx( uint8_t *curr_block, uint8_t *ref_block, const int32_t frame_stride, 
		const int32_t height) 
{
   motion_comp_avg_xy_mmx( 16, height, curr_block, ref_block, frame_stride);
}

void 
motion_comp_avg_xy_8x8_mmx( uint8_t *curr_block, uint8_t *ref_block, const int32_t frame_stride, 
		const int32_t height) 
{
   motion_comp_avg_xy_mmx( 8, height, curr_block, ref_block, frame_stride);
}

//-----------------------------------------------------------------------

static inline void 
motion_comp_put_xy_mmx( const uint8_t width, const int32_t height, uint8_t *curr_block, uint8_t *ref_block, 
		const int32_t stride) 
{
	int y;
	uint8_t *ref_block_next = ref_block + stride;

	mmx_zero_reg();

	for (y = 0; y < height; y++) 
	{
		mmx_average_4_U8(curr_block, ref_block, ref_block + 1, ref_block_next, ref_block_next + 1);

		if(width == 16)
			mmx_average_4_U8(curr_block + 8, ref_block + 8, ref_block + 9, ref_block_next + 8, ref_block_next + 9);

		curr_block     += stride;
		ref_block      += stride;
		ref_block_next += stride;
	}
}

void 
motion_comp_put_xy_16x16_mmx( uint8_t *curr_block, uint8_t *ref_block, const int32_t frame_stride, 
		const int32_t height) 
{
   motion_comp_put_xy_mmx( 16, height, curr_block, ref_block, frame_stride);
}

void 
motion_comp_put_xy_8x8_mmx( uint8_t *curr_block, uint8_t *ref_block, const int32_t frame_stride, 
		const int32_t height) 
{
   motion_comp_put_xy_mmx( 8, height, curr_block, ref_block, frame_stride);
}

//-----------------------------------------------------------------------

static inline void 
motion_comp_avg_y_mmx( const uint8_t width, const int32_t height, uint8_t *curr_block, uint8_t *ref_block,
      const int32_t stride) 
{
	int y;
	uint8_t *ref_block_next = ref_block + stride;

	mmx_zero_reg();

	for (y = 0; y < height; y++) 
	{
		mmx_interp_average_2_U8(curr_block, ref_block, ref_block_next);

		if(width == 16)
			mmx_interp_average_2_U8(curr_block + 8, ref_block + 8, ref_block_next + 8);

		curr_block     += stride;
		ref_block      += stride;
		ref_block_next += stride;
	}
}

void 
motion_comp_avg_y_16x16_mmx( uint8_t *curr_block, uint8_t *ref_block, const int32_t frame_stride, 
		const int32_t height) 
{
   motion_comp_avg_y_mmx( 16, height, curr_block, ref_block, frame_stride);
}

void 
motion_comp_avg_y_8x8_mmx( uint8_t *curr_block, uint8_t *ref_block, const int32_t frame_stride, 
		const int32_t height) 
{
   motion_comp_avg_y_mmx( 8, height, curr_block, ref_block, frame_stride);
}

//-----------------------------------------------------------------------

static inline void
motion_comp_put_y_mmx( const uint8_t width, const int32_t height, uint8_t *curr_block, uint8_t *ref_block, 
		const int32_t stride) 
{
	int y;
	uint8_t *ref_block_next = ref_block + stride;

	mmx_zero_reg();

	for (y = 0; y < height; y++) 
	{
		mmx_average_2_U8(curr_block, ref_block, ref_block_next);

		if(width == 16)
			mmx_average_2_U8(curr_block + 8, ref_block + 8, ref_block_next + 8);

		curr_block     += stride;
		ref_block      += stride;
		ref_block_next += stride;
	}
}

void 
motion_comp_put_y_16x16_mmx( uint8_t *curr_block, uint8_t *ref_block, const int32_t frame_stride, 
		const int32_t height) 
{
	motion_comp_put_y_mmx( 16, height, curr_block, ref_block, frame_stride);
}

void 
motion_comp_put_y_8x8_mmx( uint8_t *curr_block, uint8_t *ref_block, const int32_t frame_stride, 
		const int32_t height) 
{
	motion_comp_put_y_mmx( 8, height, curr_block, ref_block, frame_stride);
}


MOTION_COMP_EXTERN(mmx)
