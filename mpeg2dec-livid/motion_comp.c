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
// motion_comp.c rewrite counter:  5
//

#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#include "mpeg2.h"
#include "mpeg2_internal.h"
#include "debug.h"

#include "mb_buffer.h"
#include "motion_comp.h"
#include "motion_comp_mmx.h"
#include "motion_comp_mlib.h"

// motion_comp main entry point 
void (*motion_comp)(picture_t *picture,mb_buffer_t *mb_buffer);

void motion_comp_c(picture_t *picture,mb_buffer_t *mb_buffer);

static uint_8 clip_lut[1024], *clip;

#include "soft_video.c" // FIXME

void
motion_comp_0mv_block(uint_8 *dst, sint_16 *block,uint_32 stride)
{
	int x,y;
	int jump = stride - 8;

  for (y = 0; y < 8; y++) {
    for (x = 0; x < 8; x++)
			*dst++ = clip_to_u8(*block++);
		dst += jump;
	}
}


void
motion_comp_init(void)
{
	sint_32 i;

	clip = clip_lut + 384;

	for(i=-384;i<640;i++)
		clip[i] = i < 0 ? 0 : (i > 255 ? 255 : i);

//FIXME turn mmx back on
//#ifdef __i386__
#if 0
	if(config.flags & MPEG2_MMX_ENABLE)
		motion_comp = motion_comp_mmx;
	else
#endif
#if HAVE_MLIB
	if(1 || config.flags & MPEG2_MLIB_ENABLE) // FIXME
		motion_comp = motion_comp_mlib;
	else
#endif
		motion_comp = motion_comp_c;
}


void motion_putblock(int x_pred, int y_pred, int field_select,
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
		soft_VideoInterpXY_U8_U8_16x16(dst_y, src1, pitch, height);
	} else if (x_half) {
		soft_VideoInterpX_U8_U8_16x16 (dst_y, src1, pitch, height);
	} else if (y_half) {
		soft_VideoInterpY_U8_U8_16x16 (dst_y, src1, pitch, height);
	} else {
		soft_VideoCopyRef_U8_U8_16x16 (dst_y, src1, pitch, height);
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
		soft_VideoInterpXY_U8_U8_8x8(dst_cr, src1, pitch, height);
		soft_VideoInterpXY_U8_U8_8x8(dst_cb, src2, pitch, height);
	} else if (x_half) {
		soft_VideoInterpX_U8_U8_8x8 (dst_cr, src1, pitch, height);
		soft_VideoInterpX_U8_U8_8x8 (dst_cb, src2, pitch, height);
	} else if (y_half) {
		soft_VideoInterpY_U8_U8_8x8 (dst_cr, src1, pitch, height);
		soft_VideoInterpY_U8_U8_8x8 (dst_cb, src2, pitch, height);
	} else {
		soft_VideoCopyRef_U8_U8_8x8 (dst_cr, src1, pitch, height);
		soft_VideoCopyRef_U8_U8_8x8 (dst_cb, src2, pitch, height);
	}
}


void motion_aveblock(int x_pred, int y_pred, int field_select,
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
		soft_VideoInterpAveXY_U8_U8_16x16(dst_y, src1, pitch, height);
	} else if (x_half) {
		soft_VideoInterpAveX_U8_U8_16x16 (dst_y, src1, pitch, height);
	} else if (y_half) {
		soft_VideoInterpAveY_U8_U8_16x16 (dst_y, src1, pitch, height);
	} else {
		soft_VideoCopyRefAve_U8_U8_16x16 (dst_y, src1, pitch, height);
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
		soft_VideoInterpAveXY_U8_U8_8x8(dst_cr, src1, pitch, height);
		soft_VideoInterpAveXY_U8_U8_8x8(dst_cb, src2, pitch, height);
	} else if (x_half) {
		soft_VideoInterpAveX_U8_U8_8x8 (dst_cr, src1, pitch, height);
		soft_VideoInterpAveX_U8_U8_8x8 (dst_cb, src2, pitch, height);
	} else if (y_half) {
		soft_VideoInterpAveY_U8_U8_8x8 (dst_cr, src1, pitch, height);
		soft_VideoInterpAveY_U8_U8_8x8 (dst_cb, src2, pitch, height);
	} else {
		soft_VideoCopyRefAve_U8_U8_8x8 (dst_cr, src1, pitch, height);
		soft_VideoCopyRefAve_U8_U8_8x8 (dst_cb, src2, pitch, height);
	}
}


void
motion_comp_c(picture_t *picture,mb_buffer_t *mb_buffer)
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
			
			motion_comp_0mv_block(dst_y                , mb->y_blocks       , pitch);
			motion_comp_0mv_block(dst_y + 8            , mb->y_blocks +   64, pitch);
			motion_comp_0mv_block(dst_y + width * d    , mb->y_blocks + 2*64, pitch);
			motion_comp_0mv_block(dst_y + width * d + 8, mb->y_blocks + 3*64, pitch);
			motion_comp_0mv_block(dst_cr, mb->cr_blocks, width/2);
			motion_comp_0mv_block(dst_cb, mb->cb_blocks, width/2);
			
		}
		else // Not an intra block
		{
		
			if(mb->macroblock_type & MACROBLOCK_MOTION_FORWARD)
			{
				if (mb->motion_type == MC_FRAME) {
					motion_putblock(
						mb->f_motion_vectors[0][0] + x * 32,
						mb->f_motion_vectors[0][1] + y * 32,
						0,
						dst_y, dst_cr, dst_cb,
						picture->forward_reference_frame,
						width, 16);
				} else {
					motion_putblock(
						mb->f_motion_vectors[0][0] + x * 32,
						(mb->f_motion_vectors[0][1] >> 1) + y * 16,
						mb->f_motion_vertical_field_select[0],
						dst_y, dst_cr, dst_cb,
						picture->forward_reference_frame,
						width * 2, 8);
					motion_putblock(
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
						motion_aveblock(
							mb->b_motion_vectors[0][0] + x * 32,
							mb->b_motion_vectors[0][1] + y * 32,
							0,
							dst_y, dst_cr, dst_cb,
							picture->backward_reference_frame,
							width, 16);
					} else {
						motion_aveblock(
							mb->b_motion_vectors[0][0] + x * 32,
							(mb->b_motion_vectors[0][1] >> 1) + y * 16,
							mb->b_motion_vertical_field_select[0],
							dst_y, dst_cr, dst_cb,
							picture->backward_reference_frame,
							width * 2, 8);
						motion_aveblock(
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
						motion_putblock(
							mb->b_motion_vectors[0][0] + x * 32,
							mb->b_motion_vectors[0][1] + y * 32,
							0,
							dst_y, dst_cr, dst_cb,
							picture->backward_reference_frame,
							width, 16);
					} else {
						motion_putblock(
							mb->b_motion_vectors[0][0] + x * 32,
							(mb->b_motion_vectors[0][1] >> 1) + y * 16,
							mb->b_motion_vertical_field_select[0],
							dst_y, dst_cr, dst_cb,
							picture->backward_reference_frame,
							width * 2, 8);
						motion_putblock(
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
					soft_VideoAddBlock_U8_S16(dst_y,                 mb->y_blocks + 0 * 64, pitch);
				if(mb->coded_block_pattern & 0x10)
					soft_VideoAddBlock_U8_S16(dst_y + 8,             mb->y_blocks + 1 * 64, pitch);
				if(mb->coded_block_pattern & 0x08)
					soft_VideoAddBlock_U8_S16(dst_y + width * d,     mb->y_blocks + 2 * 64, pitch);
				if(mb->coded_block_pattern & 0x04)
					soft_VideoAddBlock_U8_S16(dst_y + width * d + 8, mb->y_blocks + 3 * 64, pitch);
				
				if(mb->coded_block_pattern & 0x02)
					soft_VideoAddBlock_U8_S16(dst_cr, mb->cr_blocks, width/2);
				if(mb->coded_block_pattern & 0x01)
					soft_VideoAddBlock_U8_S16(dst_cb, mb->cb_blocks, width/2);
				
			}	
		}
	}
}
