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

#include "mb_buffer.h"
#include "motion_comp.h"
#include "motion_comp_mmx.h"



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
mmx_average_2_U8(uint_8 *dst, uint_8 *src1, uint_8 *src2)
{
   //
   // *dst = clip_to_u8((*src1 + *src2 + 1)/2);
   //

   pxor_r2r(mm0,mm0);         // load 0 into mm0

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
mmx_interp_average_2_U8(uint_8 *dst, uint_8 *src1, uint_8 *src2)
{
   //
   // *dst = clip_to_u8((*dst + (*src1 + *src2 + 1)/2 + 1)/2);
   //

   pxor_r2r(mm0,mm0);             // load 0 into mm0

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
mmx_average_4_U8(uint_8 *dst, uint_8 *src1, uint_8 *src2, uint_8 *src3, uint_8 *src4)
{
   //
   // *dst = clip_to_u8((*src1 + *src2 + *src3 + *src4 + 2)/4);
   //

   pxor_r2r(mm0,mm0);                  // load 0 into mm0

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
mmx_interp_average_4_U8(uint_8 *dst, uint_8 *src1, uint_8 *src2, uint_8 *src3, uint_8 *src4)
{
   //
   // *dst = clip_to_u8((*dst + (*src1 + *src2 + *src3 + *src4 + 2)/4 + 1)/2);
   //

   pxor_r2r(mm0,mm0);                  // load 0 into mm0

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

void 
motion_comp_idct_copy_mmx (uint_8 *dst, sint_16 *block,uint_32 stride)
{
	int y;

	for (y = 0; y < 8; y++) 
	{
		movq_m2r(*block,mm1);
		movq_m2r(*(block+4),mm2);
		block += 8;
		packuswb_r2r(mm2,mm1);
		movq_r2m(mm1,*dst);
		dst += stride;
	}
}


// Add a motion compensated 8x8 block to the current block
void
motion_comp_idct_add_mmx( uint_8 *dst, sint_16 *block, uint_32 stride) 
{
	int y;

	for (y = 0; y < 8; y++) 
	{
			pxor_r2r(mm0,mm0);             // load 0 into mm0

			movq_m2r(*dst,mm1);            // load 8 curr bytes
			movq_r2r(mm1,mm2);             // copy 8 curr bytes

			punpcklbw_r2r(mm0,mm1);        // unpack low curr bytes
			punpckhbw_r2r(mm0,mm2);        // unpack high curr bytes

			movq_m2r(*block,mm3);          // load 4 mc words (mc0)
			paddsw_r2r(mm3,mm1);           // mm1=curr+mc0 (w/ saturatation)

			movq_m2r(*(block+4),mm4);      // load next 4 mc words (mc1)
			paddsw_r2r(mm4,mm2);           // mm2=curr+mc1 (w/ saturatation)

			packuswb_r2r(mm2,mm1);         // pack (w/ saturation)
			movq_r2m(mm1,*dst);            // store result in curr

			dst += stride;
			block   += 8;
	}
}  

//-----------------------------------------------------------------------

static inline void 
motion_comp_avg_mmx( const uint_8 width, const sint_32 height, uint_8 *curr_block,
      uint_8 *ref_block, const sint_32 stride)
{
	int x,y;
	int step = 8;
	int jump = stride - width;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < width/8; x++) 
		{
			mmx_average_2_U8(curr_block, curr_block, ref_block);

			curr_block += step;
			ref_block  += step;
		}
		curr_block += jump;
		ref_block  += jump;
	}
}

void
motion_comp_avg_16x16_mmx( uint_8 *curr_block, uint_8 *ref_block, const sint_32 stride, 
		const sint_32 height)
{
   motion_comp_avg_mmx( 16, height, curr_block, ref_block, stride);
}

void
motion_comp_avg_8x8_mmx( uint_8 *curr_block, uint_8 *ref_block, const sint_32 stride, 
		const sint_32 height)
{
   motion_comp_avg_mmx( 8, height, curr_block, ref_block, stride);
}

//-----------------------------------------------------------------------

static inline void
motion_comp_put_mmx( const uint_8 width, const sint_32 height, uint_8 *curr_block, 
		uint_8 *ref_block, const sint_32 stride)
{
	int x,y;
	int step = 8;
	int jump = stride - width;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < width/8; x++) 
		{
			movq_m2r(*ref_block,mm1);    // load 8 ref bytes
			movq_r2m(mm1,*curr_block);   // store 8 bytes at curr

			curr_block += step;
			ref_block  += step;
		}
		curr_block += jump;
		ref_block  += jump;
	}
}

void
motion_comp_put_16x16_mmx( uint_8 *curr_block, uint_8 *ref_block, const sint_32 stride, const sint_32 height)
{
   motion_comp_put_mmx( 16, height, curr_block, ref_block, stride);
}

void
motion_comp_put_8x8_mmx( uint_8 *curr_block, uint_8 *ref_block, const sint_32 stride, const sint_32 height)
{
   motion_comp_put_mmx( 8, height, curr_block, ref_block, stride);
}

//-----------------------------------------------------------------------

// Half pixel interpolation in the x direction
static inline void 
motion_comp_avg_x_mmx( const uint_8 width, const sint_32 height, 
		uint_8 *curr_block, uint_8 *ref_block, const sint_32 frame_stride) 
{
	int x,y;
	int step = 8;
	int jump = frame_stride - width;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < width/8; x++) 
		{
			mmx_interp_average_2_U8(curr_block, ref_block, ref_block + 1);

			curr_block += step;
			ref_block  += step;
		}
		curr_block += jump;
		ref_block  += jump;
	}
}

void 
motion_comp_avg_x_16x16_mmx( uint_8 *curr_block, uint_8 *ref_block, const sint_32 frame_stride, 
		const sint_32 height) 
{
   motion_comp_avg_x_mmx( 16, height, curr_block, ref_block, frame_stride);
}

void 
motion_comp_avg_x_8x8_mmx( uint_8 *curr_block, uint_8 *ref_block, const sint_32 frame_stride, 
		const sint_32 height) 
{
   motion_comp_avg_x_mmx( 8, height, curr_block, ref_block, frame_stride);
}

//-----------------------------------------------------------------------

static inline void
motion_comp_put_x_mmx( const uint_8 width, const sint_32 height, uint_8 *curr_block, uint_8 *ref_block, 
		sint_32 frame_stride) 
{
	int x,y;
	int step = 8;
	int jump = frame_stride - width;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < width/8; x++) 
		{
			mmx_average_2_U8(curr_block, ref_block, ref_block + 1);

			curr_block += step;
			ref_block  += step;
		}
		curr_block += jump;
		ref_block  += jump;
	}
}

void 
motion_comp_put_x_16x16_mmx( uint_8 *curr_block, uint_8 *ref_block, const sint_32 frame_stride, 
		const sint_32 height) 
{
   motion_comp_put_x_mmx( 16, height, curr_block, ref_block, frame_stride);
}

void 
motion_comp_put_x_8x8_mmx( uint_8 *curr_block, uint_8 *ref_block, const sint_32 frame_stride, 
		const sint_32 height) 
{
   motion_comp_put_x_mmx( 8, height, curr_block, ref_block, frame_stride);
}


//-----------------------------------------------------------------------

static inline void 
motion_comp_avg_xy_mmx( const uint_8 width, const sint_32 height, uint_8 *curr_block, uint_8 *ref_block, 
		const sint_32 frame_stride) 
{
	int x,y;
	int step = 8;
	int jump = frame_stride - width;
	uint_8 *ref_block_next = ref_block + frame_stride;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < width/8; x++) 
		{
			mmx_interp_average_4_U8(curr_block, ref_block, ref_block + 1, ref_block_next, ref_block_next + 1);

			curr_block     += step;
			ref_block      += step;
			ref_block_next += step;
		}
		curr_block     += jump;
		ref_block      += jump;
		ref_block_next += jump;
	}
}

void 
motion_comp_avg_xy_16x16_mmx( uint_8 *curr_block, uint_8 *ref_block, const sint_32 frame_stride, 
		const sint_32 height) 
{
   motion_comp_avg_xy_mmx( 16, height, curr_block, ref_block, frame_stride);
}

void 
motion_comp_avg_xy_8x8_mmx( uint_8 *curr_block, uint_8 *ref_block, const sint_32 frame_stride, 
		const sint_32 height) 
{
   motion_comp_avg_xy_mmx( 8, height, curr_block, ref_block, frame_stride);
}

//-----------------------------------------------------------------------

static inline void 
motion_comp_put_xy_mmx( const uint_8 width, const sint_32 height, uint_8 *curr_block, uint_8 *ref_block, 
		const sint_32 frame_stride) 
{
	int x,y;
	int step = 8;
	int jump = frame_stride - width;
	uint_8 *ref_block_next = ref_block + frame_stride;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < width/8; x++) 
		{
			mmx_average_4_U8(curr_block, ref_block, ref_block + 1, ref_block_next, ref_block_next + 1);

			curr_block     += step;
			ref_block      += step;
			ref_block_next += step;
		}
		curr_block     += jump;
		ref_block      += jump;
		ref_block_next += jump;
	}
}

void 
motion_comp_put_xy_16x16_mmx( uint_8 *curr_block, uint_8 *ref_block, const sint_32 frame_stride, 
		const sint_32 height) 
{
   motion_comp_put_xy_mmx( 16, height, curr_block, ref_block, frame_stride);
}

void 
motion_comp_put_xy_8x8_mmx( uint_8 *curr_block, uint_8 *ref_block, const sint_32 frame_stride, 
		const sint_32 height) 
{
   motion_comp_put_xy_mmx( 8, height, curr_block, ref_block, frame_stride);
}

//-----------------------------------------------------------------------

static inline void 
motion_comp_avg_y_mmx( const uint_8 width, const sint_32 height, uint_8 *curr_block, uint_8 *ref_block,
      const sint_32 frame_stride) 
{
	int x,y;
	int step = 8;
	int jump = frame_stride - width;
	uint_8 *ref_block_next = ref_block + frame_stride;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < width/8; x++) 
		{
			mmx_interp_average_2_U8(curr_block, ref_block, ref_block_next);

			curr_block     += step;
			ref_block      += step;
			ref_block_next += step;
		}
		curr_block     += jump;
		ref_block      += jump;
		ref_block_next += jump;
	}
}

void 
motion_comp_avg_y_16x16_mmx( uint_8 *curr_block, uint_8 *ref_block, const sint_32 frame_stride, 
		const sint_32 height) 
{
   motion_comp_avg_y_mmx( 16, height, curr_block, ref_block, frame_stride);
}

void 
motion_comp_avg_y_8x8_mmx( uint_8 *curr_block, uint_8 *ref_block, const sint_32 frame_stride, 
		const sint_32 height) 
{
   motion_comp_avg_y_mmx( 8, height, curr_block, ref_block, frame_stride);
}

//-----------------------------------------------------------------------

static inline void
motion_comp_put_y_mmx( const uint_8 width, const sint_32 height, uint_8 *curr_block, uint_8 *ref_block, 
		const sint_32 frame_stride) 
{
	int x,y;
	int step = 8;
	int jump = frame_stride - width;
	uint_8 *ref_block_next = ref_block + frame_stride;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < width/8; x++) 
		{
			mmx_average_2_U8(curr_block, ref_block, ref_block_next);

			curr_block     += step;
			ref_block      += step;
			ref_block_next += step;
		}
		curr_block     += jump;
		ref_block      += jump;
		ref_block_next += jump;
	}
}

void 
motion_comp_put_y_16x16_mmx( uint_8 *curr_block, uint_8 *ref_block, const sint_32 frame_stride, 
		const sint_32 height) 
{
   motion_comp_put_y_mmx( 16, height, curr_block, ref_block, frame_stride);
}

void 
motion_comp_put_y_8x8_mmx( uint_8 *curr_block, uint_8 *ref_block, const sint_32 frame_stride, 
		const sint_32 height) 
{
   motion_comp_put_y_mmx( 8, height, curr_block, ref_block, frame_stride);
}

#if 0

void motion_putblock_mmx(int x_pred, int y_pred, int field_select,
      uint_8 *dst_y, uint_8 *dst_cr, uint_8 *dst_cb,	uint_8 *refframe[3], int pitch, int height)
{
   int x_half, y_half;
   uint_8 *src1, *src2;

   x_half = x_pred & 1;
   y_half = y_pred & 1;
   x_pred >>= 1;
   y_pred >>= 1;

   src1 = refframe[0] + x_pred + y_pred * pitch + field_select * (pitch >> 1);

   if (x_half && y_half) {
      mmx_VideoInterpXY_U8_U8_16x16(dst_y, src1, pitch, height);
   } else if (x_half) {
      mmx_VideoInterpX_U8_U8_16x16 (dst_y, src1, pitch, height);
   } else if (y_half) {
      mmx_VideoInterpY_U8_U8_16x16 (dst_y, src1, pitch, height);
   } else {
      mmx_VideoCopyRef_U8_U8_16x16 (dst_y, src1, pitch, height);
   }

   x_half = x_pred & 1;
   y_half = y_pred & 1;
   x_pred >>= 1;
   y_pred >>= 1;
   pitch >>= 1;
   height >>= 1;

   src1 = refframe[1] + x_pred + y_pred * pitch + field_select * (pitch >> 1);
   src2 = refframe[2] + x_pred + y_pred * pitch + field_select * (pitch >> 1);

   if (x_half && y_half) {
      mmx_VideoInterpXY_U8_U8_8x8(dst_cr, src1, pitch, height);
      mmx_VideoInterpXY_U8_U8_8x8(dst_cb, src2, pitch, height);
   } else if (x_half) {
      mmx_VideoInterpX_U8_U8_8x8 (dst_cr, src1, pitch, height);
      mmx_VideoInterpX_U8_U8_8x8 (dst_cb, src2, pitch, height);
   } else if (y_half) {
      mmx_VideoInterpY_U8_U8_8x8 (dst_cr, src1, pitch, height);
      mmx_VideoInterpY_U8_U8_8x8 (dst_cb, src2, pitch, height);
   } else {
      mmx_VideoCopyRef_U8_U8_8x8 (dst_cr, src1, pitch, height);
      mmx_VideoCopyRef_U8_U8_8x8 (dst_cb, src2, pitch, height);
   }
}


void motion_aveblock_mmx(int x_pred, int y_pred, int field_select,
      uint_8 *dst_y, uint_8 *dst_cr, uint_8 *dst_cb,	uint_8 *refframe[3], int pitch, int height)
{
   int x_half, y_half;
   uint_8 *src1, *src2;

   x_half = x_pred & 1;
   y_half = y_pred & 1;
   x_pred >>= 1;
   y_pred >>= 1;

   src1 = refframe[0] + x_pred + y_pred * pitch + field_select * (pitch >> 1);

   if (x_half && y_half) {
      mmx_VideoInterpAveXY_U8_U8_16x16(dst_y, src1, pitch, height);
   } else if (x_half) {
      mmx_VideoInterpAveX_U8_U8_16x16 (dst_y, src1, pitch, height);
   } else if (y_half) {
      mmx_VideoInterpAveY_U8_U8_16x16 (dst_y, src1, pitch, height);
   } else {
      mmx_VideoCopyRefAve_U8_U8_16x16 (dst_y, src1, pitch, height);
   }

   x_half = x_pred & 1;
   y_half = y_pred & 1;
   x_pred >>= 1;
   y_pred >>= 1;
   pitch >>= 1;
   height >>= 1;

   src1 = refframe[1] + x_pred + y_pred * pitch + field_select * (pitch >> 1);
   src2 = refframe[2] + x_pred + y_pred * pitch + field_select * (pitch >> 1);

   if (x_half && y_half) {
      mmx_VideoInterpAveXY_U8_U8_8x8(dst_cr, src1, pitch, height);
      mmx_VideoInterpAveXY_U8_U8_8x8(dst_cb, src2, pitch, height);
   } else if (x_half) {
      mmx_VideoInterpAveX_U8_U8_8x8 (dst_cr, src1, pitch, height);
      mmx_VideoInterpAveX_U8_U8_8x8 (dst_cb, src2, pitch, height);
   } else if (y_half) {
      mmx_VideoInterpAveY_U8_U8_8x8 (dst_cr, src1, pitch, height);
      mmx_VideoInterpAveY_U8_U8_8x8 (dst_cb, src2, pitch, height);
   } else {
      mmx_VideoCopyRefAve_U8_U8_8x8 (dst_cr, src1, pitch, height);
      mmx_VideoCopyRefAve_U8_U8_8x8 (dst_cb, src2, pitch, height);
   }
}


void
motion_comp_mmx(picture_t *picture,mb_buffer_t *mb_buffer)
{
	macroblock_t *macroblocks   = mb_buffer->macroblocks,*mb;
	uint_32 num_blocks = mb_buffer->num_blocks;
	int i;
	int width,x,y;
	int mb_width;
	int pitch;
	int d;
	uint_8 *dst_y,*dst_cr,*dst_cb;

	width = picture->coded_picture_width;
	mb_width = picture->coded_picture_width >> 4;

	for(i=0;i<num_blocks;i++)
	{
		mb = macroblocks + i;

		//handle interlaced blocks
		if (mb->dct_type) 
		{
			d = 1;
			pitch = width *2;
		}
		else 
		{
			d = 8;
			pitch = width;
		}

		//FIXME I'd really to take these two divides out.
		//maybe do 16.16 fixed point mult 
		x = mb->mba % mb_width;
		y = mb->mba / mb_width;

		dst_y = &picture->current_frame[0][x * 16 + y * width * 16];
		dst_cr = &picture->current_frame[1][x * 8 + y * width/2 * 8];
		dst_cb = &picture->current_frame[2][x * 8 + y * width/2 * 8];
		
      if(mb->macroblock_type & MACROBLOCK_INTRA)
      {
         // FIXME
         // The idct transform should write these right into the frame earlier.

         motion_comp_0mv_block_mmx(dst_y                , mb->y_blocks       , pitch);
         motion_comp_0mv_block_mmx(dst_y + 8            , mb->y_blocks +   64, pitch);
         motion_comp_0mv_block_mmx(dst_y + width * d    , mb->y_blocks + 2*64, pitch);
         motion_comp_0mv_block_mmx(dst_y + width * d + 8, mb->y_blocks + 3*64, pitch);
         motion_comp_0mv_block_mmx(dst_cr, mb->cr_blocks, width/2);
         motion_comp_0mv_block_mmx(dst_cb, mb->cb_blocks, width/2);

      }
      else // Not an intra block
      {
         if(mb->macroblock_type & MACROBLOCK_MOTION_FORWARD)
         {
            if (mb->motion_type == MC_FRAME) {
               motion_putblock_mmx(
                     mb->f_motion_vectors[0][0] + x * 32,
                     mb->f_motion_vectors[0][1] + y * 32,
                     0,
                     dst_y, dst_cr, dst_cb,
                     picture->forward_reference_frame,
                     width, 16);
            } else {
               motion_putblock_mmx(
                     mb->f_motion_vectors[0][0] + x * 32,
                     (mb->f_motion_vectors[0][1] >> 1) + y * 16,
                     mb->f_motion_vertical_field_select[0],
                     dst_y, dst_cr, dst_cb,
                     picture->forward_reference_frame,
                     width * 2, 8);
               motion_putblock_mmx(
                     mb->f_motion_vectors[1][0] + x * 32,
                     (mb->f_motion_vectors[1][1] >> 1) + y * 16,
                     mb->f_motion_vertical_field_select[1],
                     dst_y + width, dst_cr + width/2, dst_cb + width/2,
                     picture->forward_reference_frame,
                     width * 2, 8);
            }
         }
         if(mb->macroblock_type & MACROBLOCK_MOTION_BACKWARD)
         {
            if(mb->macroblock_type & MACROBLOCK_MOTION_FORWARD)
            {
               if (mb->motion_type == MC_FRAME) {
                  motion_aveblock_mmx(
                        mb->b_motion_vectors[0][0] + x * 32,
                        mb->b_motion_vectors[0][1] + y * 32,
                        0,
                        dst_y, dst_cr, dst_cb,
                        picture->backward_reference_frame,
                        width, 16);
               } else {
                  motion_aveblock_mmx(
                        mb->b_motion_vectors[0][0] + x * 32,
                        (mb->b_motion_vectors[0][1] >> 1) + y * 16,
                        mb->b_motion_vertical_field_select[0],
                        dst_y, dst_cr, dst_cb,
                        picture->backward_reference_frame,
                        width * 2, 8);
                  motion_aveblock_mmx(
                        mb->b_motion_vectors[1][0] + x * 32,
                        (mb->b_motion_vectors[1][1] >> 1) + y * 16,
                        mb->b_motion_vertical_field_select[1],
                        dst_y + width, dst_cr + width/2, dst_cb + width/2,
                        picture->backward_reference_frame,
                        width * 2, 8);
               }
            }
            else
            {
               if (mb->motion_type == MC_FRAME) {
                  motion_putblock_mmx(
                        mb->b_motion_vectors[0][0] + x * 32,
                        mb->b_motion_vectors[0][1] + y * 32,
                        0,
                        dst_y, dst_cr, dst_cb,
                        picture->backward_reference_frame,
                        width, 16);
               } else {
                  motion_putblock_mmx(
                        mb->b_motion_vectors[0][0] + x * 32,
                        (mb->b_motion_vectors[0][1] >> 1) + y * 16,
                        mb->b_motion_vertical_field_select[0],
                        dst_y, dst_cr, dst_cb,
                        picture->backward_reference_frame,
                        width * 2, 8);
                  motion_putblock_mmx(
                        mb->b_motion_vectors[1][0] + x * 32,
                        (mb->b_motion_vectors[1][1] >> 1) + y * 16,
                        mb->b_motion_vertical_field_select[1],
                        dst_y + width, dst_cr + width/2, dst_cb + width/2,
                        picture->backward_reference_frame,
                        width * 2, 8);
               }
            }
         }

			if(mb->macroblock_type & MACROBLOCK_PATTERN)
			{
				// Asume zero forward motion if the block has none.	
				if( !(mb->macroblock_type & (MACROBLOCK_MOTION_FORWARD | MACROBLOCK_MOTION_BACKWARD)) )
				{
					fprintf(stderr, "PATTERN - NO MOTION");
					exit(2);
				}
			
				if(mb->coded_block_pattern & 0x20)
					mmx_VideoAddBlock_U8_S16(dst_y,                 mb->y_blocks + 0 * 64, pitch);
				if(mb->coded_block_pattern & 0x10)
					mmx_VideoAddBlock_U8_S16(dst_y + 8,             mb->y_blocks + 1 * 64, pitch);
				if(mb->coded_block_pattern & 0x08)
					mmx_VideoAddBlock_U8_S16(dst_y + width * d,     mb->y_blocks + 2 * 64, pitch);
				if(mb->coded_block_pattern & 0x04)
					mmx_VideoAddBlock_U8_S16(dst_y + width * d + 8, mb->y_blocks + 3 * 64, pitch);
				
				if(mb->coded_block_pattern & 0x02)
					mmx_VideoAddBlock_U8_S16(dst_cr, mb->cr_blocks, width/2);
				if(mb->coded_block_pattern & 0x01)
					mmx_VideoAddBlock_U8_S16(dst_cb, mb->cb_blocks, width/2);
				
			}	
		}
	}
   emms();
}

void
print_U8_U8( const uint_8 width, const sint_32 height, uint_8 *block0, uint_8 *block1, const sint_32 stride)
{
	int x,y;
	int jump = stride - width;
	printf("block %dx%d @ %x<-%x stride=%d\n",width,height,(uint_32)block0,(uint_32)block1,stride);
	for (y = 0; y < width; y++) 
	{
		printf("%2d: ",y);
		for (x = 0; x < width; x++) 
			printf("%3d<%3d ",*block0++,*block1++);
		printf("\n");
		block0 += jump;
		block1 += jump;
	}
}


#endif
