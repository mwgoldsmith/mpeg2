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

#include <stdlib.h>
#include <stdio.h>
#include "mpeg2_internal.h"
#include "mb_buffer.h"
#include "motion_comp.h"


static uint_8 clip_lut[1024], *clip;

void
motion_comp_init(void)
{
	sint_32 i;

	clip = clip_lut + 384;

	for(i=-384;i<640;i++)
		clip[i] = i < 0 ? 0 : (i > 255 ? 255 : i);
}

//Combine the prediction and intra coded block into current frame
static void
motion_comp_block(uint_8 *pred,sint_16 *block,uint_8 *dst,uint_32 pitch)
{
	uint_32 i;

	pitch = pitch - 8;

	for(i=0;i<64;i++)
	{
		//FIXME put clipping back in
		*dst++ = block[i];
		if(!((i+1)%8))
			dst += pitch;
	}
}

void
motion_comp(picture_t *picture,mb_buffer_t *mb_buffer)
{
	macroblock_t *mb   = mb_buffer->macroblocks;
	uint_32 num_blocks = mb_buffer->num_blocks;
	uint_32 i;
	uint_32 width,x,y;
	uint_32 mb_width;
	uint_8 *dst;
	uint_8 *pred;
	
	//FIXME make -Werror happy
	pred = 0;

	width = picture->coded_picture_width;
	mb_width = picture->coded_picture_width >> 4;

	for(i=0;i<num_blocks;i++)
	{
		//XXX again 4:2:0 only here. Is there much point in supporting anything
		//else? I'd like to see a windows player handle a 4:2:2 15Mbps stream.
		
		if(mb[i].skipped)
			continue;

		x = mb[i].mba % mb_width;
		y = mb[i].mba / mb_width;

		//Do y component
		dst = &picture->current_frame[0][x * 16 + y * width * 16];

		motion_comp_block(0, mb[i].y_blocks       , dst                 , width);
		motion_comp_block(0, mb[i].y_blocks +   64, dst + 8             , width);
		motion_comp_block(0, mb[i].y_blocks + 2*64, dst + width * 8    , width);
		motion_comp_block(0, mb[i].y_blocks + 3*64, dst + width * 8 + 8, width);

		//Do Cr component
		dst = &picture->current_frame[1][x * 8 + y * width/2 * 8];
		motion_comp_block(0, mb[i].cr_blocks, dst, width/2);
		

		//Do Cb component
		dst = &picture->current_frame[2][x * 8 + y * width/2 * 8];
		motion_comp_block(0, mb[i].cb_blocks, dst, width/2);

		//FIXME debug stuff
	  //display_frame(picture->current_frame);
		//getchar();
	}
}

