/*
 *  motion_comp_mlib.c
 *
 *  Copyright (C) Håkan Hjort <d95hjort@dtek.chalmers.se> - Jan 2000
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
// motion_comp.c rewrite counter:  4
//

#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#include "mpeg2.h"
#include "mpeg2_internal.h"
#include "debug.h"
#include "mb_buffer.h"
#include "motion_comp_mlib.h"

#include <mlib_types.h>
#include <mlib_status.h>
#include <mlib_sys.h>
#include <mlib_video.h>



void
motion_comp_mlib(picture_t *picture,mb_buffer_t *mb_buffer)
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
			// XXX FIXME
			// The idct transform could write these right into the frame earlier.
			
			motion_comp_0mv_block(dst_y,                 mb->y_blocks       , pitch);
			motion_comp_0mv_block(dst_y + 8,             mb->y_blocks +   64, pitch);
			motion_comp_0mv_block(dst_y + width * d,     mb->y_blocks + 2*64, pitch);
			motion_comp_0mv_block(dst_y + width * d + 8, mb->y_blocks + 3*64, pitch);
			motion_comp_0mv_block(dst_cr, mb->cr_blocks, width/2);
			motion_comp_0mv_block(dst_cb, mb->cb_blocks, width/2);
			
		}
		else // Not an intra block
		{
		
			if(mb->macroblock_type & MACROBLOCK_MOTION_FORWARD)
			{
				int x_half   = (mb->f_motion_vectors[0][0] & 1);
				int y_half   = (mb->f_motion_vectors[0][1] & 1);
				int x_pred_y = (mb->f_motion_vectors[0][0] >> 1) + x * 16;
				int y_pred_y = (mb->f_motion_vectors[0][1] >> 1) + y * 16;
				int x_pred_c = x_pred_y / 2;
				int y_pred_c = y_pred_y / 2;
				
				uint_8 *pred_y  = &picture->forward_reference_frame[0][x_pred_y + y_pred_y * width];
				uint_8 *pred_cr = &picture->forward_reference_frame[1][x_pred_c + y_pred_c * width/2];				
				uint_8 *pred_cb = &picture->forward_reference_frame[2][x_pred_c + y_pred_c * width/2];
				
				if (x_half && y_half) {
					mlib_VideoInterpXY_U8_U8_16x16(dst_y,  pred_y,  width,   width);
					mlib_VideoInterpXY_U8_U8_8x8  (dst_cr, pred_cr, width/2, width/2);
					mlib_VideoInterpXY_U8_U8_8x8  (dst_cb, pred_cb, width/2, width/2);
				} else if (x_half) {
					mlib_VideoInterpX_U8_U8_16x16(dst_y,  pred_y,  width,   width);
					mlib_VideoInterpX_U8_U8_8x8  (dst_cr, pred_cr, width/2, width/2);
					mlib_VideoInterpX_U8_U8_8x8  (dst_cb, pred_cb, width/2, width/2);
				} else if (y_half) {
					mlib_VideoInterpY_U8_U8_16x16(dst_y,  pred_y,  width,   width);
					mlib_VideoInterpY_U8_U8_8x8  (dst_cr, pred_cr, width/2, width/2);
					mlib_VideoInterpY_U8_U8_8x8  (dst_cb, pred_cb, width/2, width/2);
				} else {
					mlib_VideoCopyRef_U8_U8_16x16(dst_y,  pred_y,  width);
					mlib_VideoCopyRef_U8_U8_8x8  (dst_cr, pred_cr, width/2);
					mlib_VideoCopyRef_U8_U8_8x8  (dst_cb, pred_cb, width/2);
				}
			}
		
			if(mb->macroblock_type & MACROBLOCK_MOTION_BACKWARD)
			{
				int x_half   = (mb->b_motion_vectors[0][0] & 1);
				int y_half   = (mb->b_motion_vectors[0][1] & 1);
				int x_pred_y = (mb->b_motion_vectors[0][0] >> 1) + x * 16;
				int y_pred_y = (mb->b_motion_vectors[0][1] >> 1) + y * 16;
				int x_pred_c = x_pred_y / 2;
				int y_pred_c = y_pred_y / 2;
				
				uint_8 *pred_y  = &picture->backward_reference_frame[0][x_pred_y + y_pred_y * width];
				uint_8 *pred_cr = &picture->backward_reference_frame[1][x_pred_c + y_pred_c * width/2];
				uint_8 *pred_cb = &picture->backward_reference_frame[2][x_pred_c + y_pred_c * width/2];
				
				if(mb->macroblock_type & MACROBLOCK_MOTION_FORWARD)
				{
					if (x_half && y_half) {
						mlib_VideoInterpAveXY_U8_U8_16x16(dst_y,  pred_y,  width,   width);
						mlib_VideoInterpAveXY_U8_U8_8x8  (dst_cr, pred_cr, width/2, width/2);
						mlib_VideoInterpAveXY_U8_U8_8x8  (dst_cb, pred_cb, width/2, width/2);
					} else if (x_half) {
						mlib_VideoInterpAveX_U8_U8_16x16(dst_y,  pred_y,  width,   width);
						mlib_VideoInterpAveX_U8_U8_8x8  (dst_cr, pred_cr, width/2, width/2);
						mlib_VideoInterpAveX_U8_U8_8x8  (dst_cb, pred_cb, width/2, width/2);
					} else if (y_half) {
						mlib_VideoInterpAveY_U8_U8_16x16(dst_y,  pred_y,  width,   width);
						mlib_VideoInterpAveY_U8_U8_8x8  (dst_cr, pred_cr, width/2, width/2);
						mlib_VideoInterpAveY_U8_U8_8x8  (dst_cb, pred_cb, width/2, width/2);
					} else {
						mlib_VideoCopyRefAve_U8_U8_16x16(dst_y,  pred_y,  width);
						mlib_VideoCopyRefAve_U8_U8_8x8  (dst_cr, pred_cr, width/2);
						mlib_VideoCopyRefAve_U8_U8_8x8  (dst_cb, pred_cb, width/2);
					}
				}
				else
				{
					if (x_half && y_half) {
						mlib_VideoInterpXY_U8_U8_16x16(dst_y,  pred_y,  width,   width);
						mlib_VideoInterpXY_U8_U8_8x8  (dst_cr, pred_cr, width/2, width/2);
						mlib_VideoInterpXY_U8_U8_8x8  (dst_cb, pred_cb, width/2, width/2);
					} else if (x_half) {
						mlib_VideoInterpX_U8_U8_16x16(dst_y,  pred_y,  width,   width);
						mlib_VideoInterpX_U8_U8_8x8  (dst_cr, pred_cr, width/2, width/2);
						mlib_VideoInterpX_U8_U8_8x8  (dst_cb, pred_cb, width/2, width/2);
					} else if (y_half) {
						mlib_VideoInterpY_U8_U8_16x16(dst_y,  pred_y,  width,   width);
						mlib_VideoInterpY_U8_U8_8x8  (dst_cr, pred_cr, width/2, width/2);
						mlib_VideoInterpY_U8_U8_8x8  (dst_cb, pred_cb, width/2, width/2);
					} else {
						mlib_VideoCopyRef_U8_U8_16x16(dst_y,  pred_y,  width);
						mlib_VideoCopyRef_U8_U8_8x8  (dst_cr, pred_cr, width/2);
						mlib_VideoCopyRef_U8_U8_8x8  (dst_cb, pred_cb, width/2);
					}
				}
			}
				
			if(mb->macroblock_type & MACROBLOCK_PATTERN)
			{
				// Asume zero forward motion if the block has none.	
				if( !(mb->macroblock_type & (MACROBLOCK_MOTION_FORWARD | MACROBLOCK_MOTION_BACKWARD)) )
				{
					fprintf(stderr, "PATTERN - NO MOTION");
					fprintf ("EXIT\n");
				}
			
				if(mb->coded_block_pattern & 0x20)
					mlib_VideoAddBlock_U8_S16(dst_y,                 mb->y_blocks + 0 * 64, pitch);
				if(mb->coded_block_pattern & 0x10)
					mlib_VideoAddBlock_U8_S16(dst_y + 8,             mb->y_blocks + 1 * 64, pitch);
				if(mb->coded_block_pattern & 0x08)
					mlib_VideoAddBlock_U8_S16(dst_y + width * d,     mb->y_blocks + 2 * 64, pitch);
				if(mb->coded_block_pattern & 0x04)
					mlib_VideoAddBlock_U8_S16(dst_y + width * d + 8, mb->y_blocks + 3 * 64, pitch);
				
				if(mb->coded_block_pattern & 0x02)
					mlib_VideoAddBlock_U8_S16(dst_cr, mb->cr_blocks, width/2);
				if(mb->coded_block_pattern & 0x01)
					mlib_VideoAddBlock_U8_S16(dst_cb, mb->cb_blocks, width/2);
				
			}	
		}
	}
}


