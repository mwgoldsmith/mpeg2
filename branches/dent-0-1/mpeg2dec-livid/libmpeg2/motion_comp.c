/*
 *  motion_comp.c
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

//
// This will get amusing in a few months I'm sure
//
// motion_comp.c rewrite counter:  6
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "mpeg2.h"
#include "mpeg2_internal.h"
#include "debug.h"

#include "motion_comp.h"

static mc_functions_t mc_functions;

void
motion_comp_init (void)
{

#ifdef __i386__
    if (1 | (config.flags & MPEG2_MMX_ENABLE))
		{
			mc_functions = mc_functions_mmx;
		}
#if HAVE_MLIB
	else if(1 || config.flags & MPEG2_MLIB_ENABLE) // FIXME
		mc_functions = mc_functions_mlib;
#endif
	else
#endif
	{
		mc_functions = mc_functions_c;
		motion_comp_c_init();
	}
}

void motion_block (void (** table) (uint_8 *, uint_8 *, sint_32, sint_32), 
		int x_pred, int y_pred, int field_select,
		uint_8 * dst_y, uint_8 * dst_cr, uint_8 * dst_cb,
		uint_8 * refframe[3], int pitch, int height)
{
	uint_32 xy_half;
	uint_8 *src1, *src2;

	xy_half = ((y_pred & 1) << 1) | (x_pred & 1);
	x_pred >>= 1;
	y_pred >>= 1;

	src1 = refframe[0] + x_pred + y_pred * pitch + field_select * (pitch >> 1);

	table[xy_half] (dst_y, src1, pitch, height);

	xy_half = ((y_pred & 1) << 1) | (x_pred & 1);
	x_pred >>= 1;
	y_pred >>= 1;
	pitch >>= 1;
	height >>= 1;

	src1 = refframe[1] + x_pred + y_pred * pitch + field_select * (pitch >> 1);
	src2 = refframe[2] + x_pred + y_pred * pitch + field_select * (pitch >> 1);


	table[4+xy_half] (dst_cr, src1, pitch, height);
	table[4+xy_half] (dst_cb, src2, pitch, height);
}

void
motion_comp (picture_t * picture, macroblock_t *mb)
{
	int width, x, y;
	int mb_width;
	int pitch;
	int d;
	uint_8 *dst_y,*dst_cr,*dst_cb;
	void (** table) (uint_8 *, uint_8 *, sint_32, sint_32);

	width = picture->coded_picture_width;
	mb_width = width >> 4;


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
	//
	//This don't seem to make much difference yet.
	//y = ((mb->mba) * 1457) >> 16;
	y = mb->mba / mb_width;
	x = mb->mba - mb_width * y; 

	dst_y = &picture->current_frame[0][x * 16 + y * width * 16];
	dst_cr = &picture->current_frame[1][x * 8 + y * width * 8/2];
	dst_cb = &picture->current_frame[2][x * 8 + y * width * 8/2];

	if (mb->macroblock_type & MACROBLOCK_INTRA) 
	{
		/* intra block : there is no prediction, only IDCT components */

		/* FIXME this could be merged into the IDCT transform */
		mc_functions.idct_copy (dst_y, &mb->blocks[0*64], pitch);
		mc_functions.idct_copy (dst_y + 8, &mb->blocks[1*64], pitch);
		mc_functions.idct_copy (dst_y + width * d, &mb->blocks[2*64], pitch);
		mc_functions.idct_copy (dst_y + width * d + 8, &mb->blocks[3*64], pitch);
		mc_functions.idct_copy (dst_cr, &mb->blocks[4*64], width>>1);
		mc_functions.idct_copy (dst_cb, &mb->blocks[5*64], width>>1);
		
		//clear the blocks for next time
		memset(mb->blocks,0,sizeof(sint_16) * 6 * 64);
	} 
	else 
	{ 
		// Not an intra block

		if (mb->macroblock_type & MACROBLOCK_MOTION_FORWARD) 
		{
			if (mb->motion_type == MC_FRAME) 
			{
				motion_block (mc_functions.put, mb->f_motion_vectors[0][0] + x * 32, 
						mb->f_motion_vectors[0][1] + y * 32, 0, dst_y, dst_cr, dst_cb, 
						picture->forward_reference_frame, width, 16);

			} 
			else 
			{
				motion_block (mc_functions.put, mb->f_motion_vectors[0][0] + x * 32, 
						(mb->f_motion_vectors[0][1] >> 1) + y * 16, mb->f_motion_vertical_field_select[0], 
						dst_y, dst_cr, dst_cb, picture->forward_reference_frame, width * 2, 8);

				motion_block (mc_functions.put, mb->f_motion_vectors[1][0] + x * 32, 
						(mb->f_motion_vectors[1][1] >> 1) + y * 16, mb->f_motion_vertical_field_select[1], 
						dst_y + width, dst_cr + (width>>1), dst_cb + (width>>1), picture->forward_reference_frame, 
						width * 2, 8);
			}
		}

		if (mb->macroblock_type & MACROBLOCK_MOTION_BACKWARD) 
		{
			table = (mb->macroblock_type & MACROBLOCK_MOTION_FORWARD) ?
				mc_functions.avg: mc_functions.put;

			if (mb->motion_type == MC_FRAME) 
			{
				motion_block (table, mb->b_motion_vectors[0][0] + x * 32, 
						mb->b_motion_vectors[0][1] + y * 32, 0, dst_y, dst_cr, dst_cb, 
						picture->backward_reference_frame, width, 16);
			} 
			else 
			{
			motion_block (table, mb->b_motion_vectors[0][0] + x * 32, 
					(mb->b_motion_vectors[0][1] >> 1) + y * 16, mb->b_motion_vertical_field_select[0], 
					dst_y, dst_cr, dst_cb, picture->backward_reference_frame, width * 2, 8);

			motion_block (table, mb->b_motion_vectors[1][0] + x * 32, 
					(mb->b_motion_vectors[1][1] >> 1) + y * 16, mb->b_motion_vertical_field_select[1], 
					dst_y + width, dst_cr + (width>>1), dst_cb + (width>>1), picture->backward_reference_frame, 
					width * 2, 8);
			}
		}

		if(mb->macroblock_type & MACROBLOCK_PATTERN) 
		{
			// We should assume zero forward motion if the block has none.
			if (!(mb->macroblock_type & (MACROBLOCK_MOTION_FORWARD | MACROBLOCK_MOTION_BACKWARD)) ) 
			{
				fprintf (stderr, "PATTERN - NO MOTION");
				fprintf (stderr, "EXIT\n");
			}

			if (mb->coded_block_pattern & 0x20)
				mc_functions.idct_add (dst_y, &mb->blocks[0*64], pitch);
			if (mb->coded_block_pattern & 0x10)
				mc_functions.idct_add (dst_y + 8, &mb->blocks[1*64], pitch);
			if (mb->coded_block_pattern & 0x08)
				mc_functions.idct_add (dst_y + width * d, &mb->blocks[2*64], pitch);
			if (mb->coded_block_pattern & 0x04)
				mc_functions.idct_add (dst_y + width * d + 8, &mb->blocks[3*64], pitch);
			if (mb->coded_block_pattern & 0x02)
				mc_functions.idct_add (dst_cr, &mb->blocks[4*64], width >> 1);
			if (mb->coded_block_pattern & 0x01)
				mc_functions.idct_add (dst_cb, &mb->blocks[5*64], width >> 1);

			//clear the blocks for next time
			//I've tried only clearing blocks that had patterns, but it
			//wasn't a gain - ah
			memset(mb->blocks,0,sizeof(sint_16) * 6 * 64);
		}
	}
}
