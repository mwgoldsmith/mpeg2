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
// motion_comp.c rewrite counter:  3
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

// motion_comp main entry point 
void (*motion_comp)(picture_t *picture,mb_buffer_t *mb_buffer);

void motion_comp_c(picture_t *picture,mb_buffer_t *mb_buffer);

static uint_8 clip_lut[1024], *clip;

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
		motion_comp = motion_comp_c;
}



//
// The motion_comp heavy lifting functions have the following
// naming convention:
//
// motion_comp_?mv_[no_]block
//
// Where the ? specifies the number of motion vectors to combine
// The _no_block specifies that there is no dct coded block to
// add to the final output
//

static void
motion_comp_0mv_block(sint_16 *block,uint_8 *dst,uint_32 pitch)
{
	uint_32 i;

	pitch = pitch - 8;

	for(i=0;i<8;i++)
	{
		*dst++ = clip[*block++];
		*dst++ = clip[*block++];
		*dst++ = clip[*block++];
		*dst++ = clip[*block++];

		*dst++ = clip[*block++];
		*dst++ = clip[*block++];
		*dst++ = clip[*block++];
		*dst++ = clip[*block++];
		dst += pitch;
	}
}

static void
motion_comp_block_2mv_no_block(uint_8 *pred,uint_8 *pred2,uint_8 *dst,uint_32 pitch)
{
	uint_32 i;

	pitch = pitch - 8;

	for(i=0;i<8;i++)
	{
		*dst++ = clip[*pred++ + *pred2++];
		*dst++ = clip[*pred++ + *pred2++];
		*dst++ = clip[*pred++ + *pred2++];
		*dst++ = clip[*pred++ + *pred2++];

		*dst++ = clip[*pred++ + *pred2++];
		*dst++ = clip[*pred++ + *pred2++];
		*dst++ = clip[*pred++ + *pred2++];
		*dst++ = clip[*pred++ + *pred2++];

		pred  += pitch;
		pred2 += pitch;
		dst += pitch;
	}
}

static void
motion_comp_block_1mv_no_block(uint_8 *pred,uint_8 *dst,uint_32 pitch)
{
	uint_32 i;

	pitch = pitch - 8;

	//This should be two 32 bit reads and writes instead...the
	//unalignedness of pred could be a problem
	for(i=0;i<8;i++)
	{
		*dst++ = *pred++;
		*dst++ = *pred++;
		*dst++ = *pred++;
		*dst++ = *pred++;

		*dst++ = *pred++;
		*dst++ = *pred++;
		*dst++ = *pred++;
		*dst++ = *pred++;

		pred += pitch;
		dst += pitch;
	}
}

static void
motion_comp_block_1mv_block(uint_8 *pred,sint_16 *block,uint_8 *dst,uint_32 pitch)
{
	uint_32 i;

	pitch = pitch - 8;

	for(i=0;i<8;i++)
	{
		*dst++ = clip[*pred++ + *block++];
		*dst++ = clip[*pred++ + *block++];
		*dst++ = clip[*pred++ + *block++];
		*dst++ = clip[*pred++ + *block++];

		*dst++ = clip[*pred++ + *block++];
		*dst++ = clip[*pred++ + *block++];
		*dst++ = clip[*pred++ + *block++];
		*dst++ = clip[*pred++ + *block++];

		pred += pitch;
		dst += pitch;
	}
}


inline void
motion_comp_no_block(picture_t *picture,macroblock_t * mb)
{
	uint_32 width,x,y;
	uint_32 mb_width;
	uint_32 pitch;
	uint_32 d;
	uint_8 *dst;
	uint_32 x_pred,y_pred;
	uint_32 x_pred2,y_pred2;
	uint_8 *pred;
	uint_8 *pred2;

	width = picture->coded_picture_width;
	mb_width = picture->coded_picture_width >> 4;

	d = 8;
	pitch = width;

	//FIXME I'd really like to take these two divides out.
	//maybe do 16.16 fixed point mult 
	x = mb->mba % mb_width;
	y = mb->mba / mb_width;

	if (0)
	//this doesn't work in some case as we're adding the
	//wrong fields together
	//if((mb->macroblock_type & MACROBLOCK_MOTION_FORWARD) && 
		 //(mb->macroblock_type & MACROBLOCK_MOTION_BACKWARD))
	{
		//fprintf(stderr,"(motion_comp) backward_mv %d,%d\n",mb->b_motion_vectors[0][0] >> 1,mb->b_motion_vectors[0][1] >> 1);
		x_pred = (mb->f_motion_vectors[0][0] >> 1) + x * 16;
		y_pred = (mb->f_motion_vectors[0][1] >> 1) + y * 16;
		x_pred2 = (mb->b_motion_vectors[0][0] >> 1) + x * 16;
		y_pred2 = (mb->b_motion_vectors[0][1] >> 1) + y * 16;

		//Do y component
		dst =  &picture->current_frame[0][x * 16 + y * width * 16];
		pred = &picture->forward_reference_frame[0][x_pred  + y_pred  * width];
		pred2 =&picture->backward_reference_frame[0][x_pred2 + y_pred2 * width];

		motion_comp_block_2mv_no_block(pred                ,pred2                , dst                , pitch);
		motion_comp_block_2mv_no_block(pred + 8            ,pred2 + 8            , dst + 8            , pitch);
		motion_comp_block_2mv_no_block(pred + width * 8    ,pred2 + width * 8    , dst + width * d    , pitch);
		motion_comp_block_2mv_no_block(pred + width * 8 + 8,pred2 + width * 8 + 8, dst + width * d + 8, pitch);

		x_pred  = (mb->f_motion_vectors[0][0] >> 2) + x * 8;
		y_pred  = (mb->f_motion_vectors[0][1] >> 2) + y * 8;
		x_pred2 = (mb->b_motion_vectors[0][0] >> 2) + x * 8;
		y_pred2 = (mb->b_motion_vectors[0][1] >> 2) + y * 8;

		//Do Cr component
		dst = &picture->current_frame[1][x * 8 + y * width/2 * 8];
		pred =&picture->forward_reference_frame[1][x_pred   + y_pred  * width/2];
		pred2=&picture->backward_reference_frame[1][x_pred2 + y_pred2 * width/2];
		motion_comp_block_2mv_no_block(pred, pred2,  dst, width/2);
		

		//Do Cb component
		dst = &picture->current_frame[2][x * 8 + y * width/2 * 8];
		pred =&picture->forward_reference_frame[2][x_pred + y_pred * width/2];
		pred2=&picture->backward_reference_frame[2][x_pred2 + y_pred2 * width/2];
		motion_comp_block_2mv_no_block(pred, pred2,  dst, width/2);
		exit(1);

	}
	else if(mb->macroblock_type & MACROBLOCK_MOTION_BACKWARD)
	{
		//fprintf(stderr,"(motion_comp) backward_mv %d,%d\n",mb->b_motion_vectors[0][0] >> 1,mb->b_motion_vectors[0][1] >> 1);
		x_pred = (mb->b_motion_vectors[0][0] >> 1) + x * 16;
		y_pred = (mb->b_motion_vectors[0][1] >> 1) + y * 16;

		//Do y component
		dst = &picture->current_frame[0][x * 16 + y * width * 16];
		pred =&picture->backward_reference_frame[0][x_pred + y_pred * width];

		motion_comp_block_1mv_no_block(pred                , dst                , pitch);
		motion_comp_block_1mv_no_block(pred + 8            , dst + 8            , pitch);
		motion_comp_block_1mv_no_block(pred + width * 8    , dst + width * d    , pitch);
		motion_comp_block_1mv_no_block(pred + width * 8 + 8, dst + width * d + 8, pitch);

		x_pred = (mb->b_motion_vectors[0][0] >> 2) + x * 8;
		y_pred = (mb->b_motion_vectors[0][1] >> 2) + y * 8;

		//Do Cr component
		dst = &picture->current_frame[1][x * 8 + y * width/2 * 8];
		pred =&picture->backward_reference_frame[1][x_pred + y_pred * width/2];
		motion_comp_block_1mv_no_block(pred,  dst, width/2);
		

		//Do Cb component
		dst = &picture->current_frame[2][x * 8 + y * width/2 * 8];
		pred =&picture->backward_reference_frame[2][x_pred + y_pred * width/2];
		motion_comp_block_1mv_no_block(pred,  dst, width/2);
	}
	else if(mb->macroblock_type & MACROBLOCK_MOTION_FORWARD)
	{
		//fprintf(stderr,"(motion_comp) forward_mv %d,%d\n",mb->f_motion_vectors[0][0] >> 1,mb->f_motion_vectors[0][1] >> 1);
		x_pred = (mb->f_motion_vectors[0][0] >> 1) + x * 16;
		y_pred = (mb->f_motion_vectors[0][1] >> 1) + y * 16;

		//Do y component
		dst = &picture->current_frame[0][x * 16 + y * width * 16];
		pred =&picture->forward_reference_frame[0][x_pred + y_pred * width];

		motion_comp_block_1mv_no_block(pred                , dst                , pitch);
		motion_comp_block_1mv_no_block(pred + 8            , dst + 8            , pitch);
		motion_comp_block_1mv_no_block(pred + width * 8    , dst + width * d    , pitch);
		motion_comp_block_1mv_no_block(pred + width * 8 + 8, dst + width * d + 8, pitch);

		x_pred = (mb->f_motion_vectors[0][0] >> 2) + x * 8;
		y_pred = (mb->f_motion_vectors[0][1] >> 2) + y * 8;

		//Do Cr component
		dst = &picture->current_frame[1][x * 8 + y * width/2 * 8];
		pred =&picture->forward_reference_frame[1][x_pred + y_pred * width/2];
		motion_comp_block_1mv_no_block(pred, dst, width/2);
		

		//Do Cb component
		dst = &picture->current_frame[2][x * 8 + y * width/2 * 8];
		pred =&picture->forward_reference_frame[2][x_pred + y_pred * width/2];
		motion_comp_block_1mv_no_block(pred,  dst, width/2);
	}
	else
	{
		//should never get here
		exit(1);

	}
}

inline void
motion_comp_block(picture_t *picture,macroblock_t * mb)
{
	uint_32 width,x,y;
	uint_32 mb_width;
	uint_32 pitch;
	uint_32 d;
	uint_8 *dst;
	uint_32 x_pred,y_pred;
	uint_8 *pred;
	uint_8 **reference_frame;

	width = picture->coded_picture_width;
	mb_width = picture->coded_picture_width >> 4;

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

	if(mb->macroblock_type & (MACROBLOCK_MOTION_BACKWARD | MACROBLOCK_MOTION_FORWARD))
	{
		//fprintf(stderr,"(motion_comp) backward_mv %d,%d\n",mb->b_motion_vectors[0][0] >> 1,mb->b_motion_vectors[0][1] >> 1);

		if(mb->macroblock_type == MACROBLOCK_MOTION_FORWARD)
		{
			x_pred = (mb->f_motion_vectors[0][0] >> 1) + x * 16;
			y_pred = (mb->f_motion_vectors[0][1] >> 1) + y * 16;
      reference_frame = picture->forward_reference_frame;
		}
		else
		{
			x_pred = (mb->b_motion_vectors[0][0] >> 1) + x * 16;
			y_pred = (mb->b_motion_vectors[0][1] >> 1) + y * 16;
      reference_frame = picture->backward_reference_frame;
		}

		//Do y component
		dst = &picture->current_frame[0][x * 16 + y * width * 16];
		pred =&reference_frame[0][x_pred + y_pred * width];

		if(mb->coded_block_pattern == 0x20)
			motion_comp_block_1mv_block(pred, mb->y_blocks, dst, pitch);
		else
			motion_comp_block_1mv_no_block(pred, dst, pitch);

		if(mb->coded_block_pattern == 0x10)
			motion_comp_block_1mv_block(pred + 8, mb->y_blocks + 64, dst + 8, pitch);
		else
			motion_comp_block_1mv_no_block(pred + 8, dst + 8, pitch);

		if(mb->coded_block_pattern == 0x08)
			motion_comp_block_1mv_block(pred + width * 8, mb->y_blocks + 2*64, dst + width * d    , pitch);
		else
			motion_comp_block_1mv_no_block(pred + width * 8, dst + width * d, pitch);

		if(mb->coded_block_pattern == 0x04)
			motion_comp_block_1mv_block(pred + width * 8 + 8, mb->y_blocks + 3*64, dst + width * d + 8, pitch);
		else
			motion_comp_block_1mv_no_block(pred + width * 8 + 8, dst + width * d + 8, pitch);

		if(mb->macroblock_type == MACROBLOCK_MOTION_FORWARD)
		{
			x_pred = (mb->f_motion_vectors[0][0] >> 2) + x * 8;
			y_pred = (mb->f_motion_vectors[0][1] >> 2) + y * 8;
		}
		else
		{
			x_pred = (mb->b_motion_vectors[0][0] >> 2) + x * 8;
			y_pred = (mb->b_motion_vectors[0][1] >> 2) + y * 8;
		}

		//Do Cr component
		dst = &picture->current_frame[1][x * 8 + y * width/2 * 8];
		pred = &reference_frame[1][x_pred + y_pred * width/2];
		if(mb->coded_block_pattern == 0x02)
			motion_comp_block_1mv_block(pred, mb->cr_blocks, dst, width/2);
		else
			motion_comp_block_1mv_no_block(pred, dst, width/2);

		//Do Cb component
		dst = &picture->current_frame[2][x * 8 + y * width/2 * 8];
		pred =&reference_frame[2][x_pred + y_pred * width/2];
		if(mb->coded_block_pattern == 0x01)
			motion_comp_block_1mv_block(pred, mb->cb_blocks, dst, width/2);
		else
			motion_comp_block_1mv_no_block(pred, dst, width/2);
	}
	else 
	{
		//Do y component
		dst = &picture->current_frame[0][x * 16 + y * width * 16];

		motion_comp_0mv_block(mb->y_blocks       , dst                , pitch);
		motion_comp_0mv_block(mb->y_blocks +   64, dst + 8            , pitch);
		motion_comp_0mv_block(mb->y_blocks + 2*64, dst + width * d    , pitch);
		motion_comp_0mv_block(mb->y_blocks + 3*64, dst + width * d + 8, pitch);

		//Do Cr component
		dst = &picture->current_frame[1][x * 8 + y * width/2 * 8];
		motion_comp_0mv_block(mb->cr_blocks, dst, width/2);
		

		//Do Cb component
		dst = &picture->current_frame[2][x * 8 + y * width/2 * 8];
		motion_comp_0mv_block(mb->cb_blocks, dst, width/2);
	}
}

void
motion_comp_c(picture_t *picture,mb_buffer_t *mb_buffer)
{
	macroblock_t *macroblocks   = mb_buffer->macroblocks,*mb;
	uint_32 num_blocks = mb_buffer->num_blocks;
	uint_32 i;
	uint_32 width,x,y;
	uint_32 mb_width;
	uint_32 pitch;
	uint_32 d;
	uint_8 *dst;
	uint_32 x_pred,y_pred;
	uint_8 *pred;
	uint_8 **reference_frame;

	width = picture->coded_picture_width;
	mb_width = picture->coded_picture_width >> 4;

	//just do backward prediction for now
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

		if(mb->macroblock_type & (MACROBLOCK_MOTION_BACKWARD | MACROBLOCK_MOTION_FORWARD))
		{
			//fprintf(stderr,"(motion_comp) backward_mv %d,%d\n",mb->b_motion_vectors[0][0] >> 1,mb->b_motion_vectors[0][1] >> 1);

			if(mb->macroblock_type & MACROBLOCK_MOTION_FORWARD)
			{
				x_pred = (mb->f_motion_vectors[0][0] >> 1) + x * 16;
				y_pred = (mb->f_motion_vectors[0][1] >> 1) + y * 16;
				reference_frame = picture->forward_reference_frame;
			}
			else
			{
				x_pred = (mb->b_motion_vectors[0][0] >> 1) + x * 16;
				y_pred = (mb->b_motion_vectors[0][1] >> 1) + y * 16;
				reference_frame = picture->backward_reference_frame;
			}

			//Do y component
			dst = &picture->current_frame[0][x * 16 + y * width * 16];
			pred =&reference_frame[0][x_pred + y_pred * width];

			if(mb->coded_block_pattern == 0x20)
				motion_comp_block_1mv_block(pred, mb->y_blocks, dst, pitch);
			else
				motion_comp_block_1mv_no_block(pred, dst, pitch);

			if(mb->coded_block_pattern == 0x10)
				motion_comp_block_1mv_block(pred + 8, mb->y_blocks + 64, dst + 8, pitch);
			else
				motion_comp_block_1mv_no_block(pred + 8, dst + 8, pitch);

			if(mb->coded_block_pattern == 0x08)
				motion_comp_block_1mv_block(pred + width * 8, mb->y_blocks + 2*64, dst + width * d    , pitch);
			else
				motion_comp_block_1mv_no_block(pred + width * 8, dst + width * d, pitch);

			if(mb->coded_block_pattern == 0x04)
				motion_comp_block_1mv_block(pred + width * 8 + 8, mb->y_blocks + 3*64, dst + width * d + 8, pitch);
			else
				motion_comp_block_1mv_no_block(pred + width * 8 + 8, dst + width * d + 8, pitch);

			if(mb->macroblock_type & MACROBLOCK_MOTION_FORWARD)
			{
				x_pred = (mb->f_motion_vectors[0][0] >> 2) + x * 8;
				y_pred = (mb->f_motion_vectors[0][1] >> 2) + y * 8;
			}
			else
			{
				x_pred = (mb->b_motion_vectors[0][0] >> 2) + x * 8;
				y_pred = (mb->b_motion_vectors[0][1] >> 2) + y * 8;
			}

			//Do Cr component
			dst = &picture->current_frame[1][x * 8 + y * width/2 * 8];
			pred = &reference_frame[1][x_pred + y_pred * width/2];
			if(mb->coded_block_pattern == 0x02)
				motion_comp_block_1mv_block(pred, mb->cr_blocks, dst, width/2);
			else
				motion_comp_block_1mv_no_block(pred, dst, width/2);

			//Do Cb component
			dst = &picture->current_frame[2][x * 8 + y * width/2 * 8];
			pred =&reference_frame[2][x_pred + y_pred * width/2];
			if(mb->coded_block_pattern == 0x01)
				motion_comp_block_1mv_block(pred, mb->cb_blocks, dst, width/2);
			else
				motion_comp_block_1mv_no_block(pred, dst, width/2);
		}
		else 
		{
			if(picture->picture_coding_type != I_TYPE)
				fprintf(stderr,"macroblock_type = %d\n",mb->macroblock_type);


			//Do y component
			dst = &picture->current_frame[0][x * 16 + y * width * 16];

			motion_comp_0mv_block(mb->y_blocks       , dst                , pitch);
			motion_comp_0mv_block(mb->y_blocks +   64, dst + 8            , pitch);
			motion_comp_0mv_block(mb->y_blocks + 2*64, dst + width * d    , pitch);
			motion_comp_0mv_block(mb->y_blocks + 3*64, dst + width * d + 8, pitch);

			//Do Cr component
			dst = &picture->current_frame[1][x * 8 + y * width/2 * 8];
			motion_comp_0mv_block(mb->cr_blocks, dst, width/2);
			

			//Do Cb component
			dst = &picture->current_frame[2][x * 8 + y * width/2 * 8];
			motion_comp_0mv_block(mb->cb_blocks, dst, width/2);
		}
	}
}


