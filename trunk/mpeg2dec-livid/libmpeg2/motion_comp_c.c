/*
 *  motion_comp_c.c
 *
 *  Copyright (C) Martin Norbäck <d95mback@dtek.chalmers.se> - Jan 2000
 *
 *  Hackage by:
 *     Michel LESPINASSE <walken@windriver.com>
 *     Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
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
 * 
 *
 */

#include <stdlib.h>
#include "mpeg2.h"
#include "mpeg2_internal.h"

#include "mb_buffer.h"
#include "motion_comp.h"

static uint_8 clip_lut[1024];
static uint_8 *clip_to_u8;

//
//  All the heavy lifting for motion_comp.c is done in here.
//
//  This should serve as a reference implementation. Optimized versions
//  in assembler are a must for speed.

void
motion_comp_avg_16x16 (uint_8 *curr_block, uint_8 *ref_block, sint_32 stride, sint_32 height)
{
	int x, y;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < 16; x++)
			curr_block[x] = (ref_block[x] + curr_block[x] + 1) >> 1;
		ref_block += stride;
		curr_block += stride;
	}
}

void
motion_comp_avg_8x8 (uint_8 *curr_block, uint_8 *ref_block, sint_32 stride, sint_32 height)
{
  int x, y;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < 8; x++)
			curr_block[x] = (ref_block[x] + curr_block[x] + 1) >> 1;
		ref_block += stride;
		curr_block += stride;
	}
}

void
motion_comp_put_16x16 (uint_8 *curr_block, uint_8 *ref_block, sint_32 stride, sint_32 height)
{
  int x, y;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < 16; x++)
			curr_block[x] = ref_block[x];
		ref_block += stride;
		curr_block += stride;
	}
}

void
motion_comp_put_8x8 (uint_8 *curr_block, uint_8 *ref_block, sint_32 stride, sint_32 height)
{
	int x, y;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < 8; x++)
			curr_block[x] = ref_block[x];
		ref_block += stride;
		curr_block += stride;
	}
}

void
motion_comp_avg_x_16x16 (uint_8 *curr_block, 
				  uint_8 *ref_block, 
				  sint_32 stride,   
				  sint_32 height) 
{
	int x, y;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < 16; x++)
		curr_block[x] = (((ref_block[x] + ref_block[x+1] + 1) >> 1) + 
				curr_block[x] + 1) >> 1;
		ref_block += stride;
		curr_block += stride;
	}
}

void
motion_comp_avg_x_8x8 (uint_8 *curr_block, 
				uint_8 *ref_block, 
				sint_32 stride,   
				sint_32 height) 
{
	int x, y;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < 8; x++)
			curr_block[x] = (((ref_block[x] + ref_block[x+1] + 1) >> 1) + 
					curr_block[x] + 1) >> 1;
		ref_block += stride;
		curr_block += stride;
	}
}

void
motion_comp_put_x_16x16 (uint_8 *curr_block, 
			       uint_8 *ref_block, 
			       sint_32 stride,   
			       sint_32 height) 
{
	int x, y;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < 16; x++)
			curr_block[x] = (ref_block[x] + ref_block[x+1] + 1) >> 1;
		ref_block += stride;
		curr_block += stride;
	}
}

void
motion_comp_put_x_8x8 (uint_8 *curr_block, uint_8 *ref_block, sint_32 stride,   sint_32 height) 
{
	int x, y;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < 8; x++)
				curr_block[x] = (ref_block[x] + ref_block[x+1] + 1) >> 1;
		ref_block += stride;
		curr_block += stride;
	}
}

void
motion_comp_avg_xy_16x16 (uint_8 *curr_block, uint_8 *ref_block, sint_32 stride,   sint_32 height) 
{
	int x, y;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < 16; x++)
			curr_block[x] = (((ref_block[x] + ref_block[x+1] + ref_block[x+stride] + 
							ref_block[x+stride+1] + 2) >> 2) + curr_block[x] + 1) >> 1;
		curr_block += stride;
		ref_block += stride;
	}
}
     
void
motion_comp_avg_xy_8x8 (uint_8 *curr_block, uint_8 *ref_block, sint_32 stride, sint_32 height) 
{
	int x, y;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < 8; x++)
				curr_block[x] = (((ref_block[x] + ref_block[x+1] + ref_block[x+stride] + 
								ref_block[x+stride+1] + 2) >> 2) + curr_block[x] + 1) >> 1;
		curr_block += stride;
		ref_block += stride;
	}
}
     
void
motion_comp_put_xy_16x16 (uint_8 *curr_block, uint_8 *ref_block, sint_32 stride,   sint_32 height) 
{
	int x, y;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < 16; x++)
			curr_block[x] = (ref_block[x] + ref_block[x+1] + 
					ref_block[x+stride] + ref_block[x+stride+1] + 2) >> 2;
		curr_block += stride;
		ref_block += stride;
	}
}
     
void
motion_comp_put_xy_8x8 (uint_8 *curr_block, uint_8 *ref_block, sint_32 stride, sint_32 height) 
{
	int x, y;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < 8; x++)
			curr_block[x] = (ref_block[x] + ref_block[x+1] + 
					ref_block[x+stride] + ref_block[x+stride+1] + 2) >> 2;
		curr_block += stride;
		ref_block += stride;
	}
}
     
void
motion_comp_avg_y_16x16 (uint_8 *curr_block, uint_8 *ref_block, sint_32 stride,   sint_32 height) 
{
	int x, y;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < 16; x++)
			curr_block[x] = (((ref_block[x] + ref_block[x+stride] + 1) >> 1) + 
					curr_block[x] + 1) >> 1;
		curr_block += stride;
		ref_block += stride;
	}
}
     
void
motion_comp_avg_y_8x8 (uint_8 *curr_block, uint_8 *ref_block, sint_32 stride,   sint_32 height) 
{
	int x, y;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < 8; x++)
				curr_block[x] = (((ref_block[x] + ref_block[x+stride] + 1) >> 1) +
						 curr_block[x] + 1) >> 1;
		curr_block += stride;
		ref_block += stride;
	}
}
     
void
motion_comp_put_y_16x16 (uint_8 *curr_block, uint_8 *ref_block, sint_32 stride,   sint_32 height) 
{
	int x,y;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < 16; x++)
				curr_block[x] = (ref_block[x] + ref_block[x+stride] + 1) >> 1;
		curr_block += stride;
		ref_block += stride;
	}
}

void
motion_comp_put_y_8x8 (uint_8 *curr_block, uint_8 *ref_block, sint_32 stride,   sint_32 height) 
{
	int x,y;

	for (y = 0; y < height; y++) 
	{
		for (x = 0; x < 8; x++)
				curr_block[x] = (ref_block[x] + ref_block[x+stride] + 1) >> 1;
		curr_block += stride;
		ref_block += stride;
	}
}

void
motion_comp_idct_copy_c (uint_8 * dst, sint_16 * block, uint_32 stride)
{
	int x, y;

	for (y = 0; y < 8; y++) 
	{
		for (x = 0; x < 8; x++)
			dst[x] = block[x];
		dst += stride;
		block += 8;
	}
}

void
motion_comp_idct_add_c (uint_8 * dst, sint_16 * block, uint_32 stride)
{
	int x, y;

	for (y = 0; y < 8; y++) 
	{
		for (x = 0; x < 8; x++)
			dst[x] = clip_to_u8 [dst[x] + block[x]];
		dst += stride;
		block += 8;
	}
}

void motion_comp_c_init(void)
{
	sint_32 i;

	for ( i =-384; i < 640; i++)
		clip_lut[i+384] = i < 0 ? 0 : (i > 255 ? 255 : i);

	clip_to_u8 = clip_lut + 384;
}
