/*
 *  motion_comp_mmx.c
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
#include "mpeg2.h"
#include "mpeg2_internal.h"

#include "mb_buffer.h"
#include "motion_comp.h"
#include "motion_comp_mmx.h"

//Combine the prediction and intra coded block into current frame
static void
motion_comp_block(uint_8 *pred,sint_16 *block,uint_8 *dest,uint_32 pitch)
{
	asm volatile (
	"movq 		  (%1),%%mm0\n\t"
	"packuswb	8(%1),%%mm0\n\t"
	"movq 		 %%mm0,(%0)\n\t"
	"addl			%2,%0\n\t"

	"movq 		 16(%1),%%mm0\n\t"
	"packuswb	24(%1),%%mm0\n\t"
	"movq 		 %%mm0,(%0)\n\t"
	"addl			%2,%0\n\t"

	"movq 		 32(%1),%%mm0\n\t"
	"packuswb	40(%1),%%mm0\n\t"
	"movq 		 %%mm0,(%0)\n\t"
	"addl			%2,%0\n\t"

	"movq 		 48(%1),%%mm0\n\t"
	"packuswb	56(%1),%%mm0\n\t"
	"movq 		 %%mm0,(%0)\n\t"
	"addl			%2,%0\n\t"

	"movq 		 64(%1),%%mm0\n\t"
	"packuswb	72(%1),%%mm0\n\t"
	"movq 		 %%mm0,(%0)\n\t"
	"addl			%2,%0\n\t"

	"movq 		 80(%1),%%mm0\n\t"
	"packuswb	88(%1),%%mm0\n\t"
	"movq 		 %%mm0,(%0)\n\t"
	"addl			%2,%0\n\t"

	"movq 		 96(%1),%%mm0\n\t"
	"packuswb	104(%1),%%mm0\n\t"
	"movq 		 %%mm0,(%0)\n\t"
	"addl			%2,%0\n\t"

	"movq 		 112(%1),%%mm0\n\t"
	"packuswb	120(%1),%%mm0\n\t"
	"movq 		 %%mm0,(%0)\n\t"
	:"+r" (dest): "r" (block),"r" (pitch));
}

void
motion_comp_mmx(picture_t *picture,mb_buffer_t *mb_buffer)
{
	macroblock_t *mb   = mb_buffer->macroblocks;
	uint_32 num_blocks = mb_buffer->num_blocks;
	uint_32 i;
	uint_32 width,x,y;
	uint_32 mb_width;
	uint_32 pitch;
	uint_32 d;
	uint_8 *dst;
	
	width = picture->coded_picture_width;
	mb_width = picture->coded_picture_width >> 4;

	for(i=0;i<num_blocks;i++)
	{
		if(mb[i].skipped)
			continue;

		//handle interlaced blocks
		if (mb[i].dct_type) 
		{
			d = 1;
			pitch = width *2;
		}
		else 
		{
			d = 8;
			pitch = width;
		}

		x = mb[i].mba % mb_width;
		y = mb[i].mba / mb_width;

		//Do y component
		dst = &picture->current_frame[0][x * 16 + y * width * 16];

		motion_comp_block(0, mb[i].y_blocks       , dst                , pitch);
		motion_comp_block(0, mb[i].y_blocks +   64, dst + 8            , pitch);
		motion_comp_block(0, mb[i].y_blocks + 2*64, dst + width * d    , pitch);
		motion_comp_block(0, mb[i].y_blocks + 3*64, dst + width * d + 8, pitch);

		//Do Cr component
		dst = &picture->current_frame[1][x * 8 + y * width/2 * 8];
		motion_comp_block(0, mb[i].cr_blocks, dst, width/2);
		

		//Do Cb component
		dst = &picture->current_frame[2][x * 8 + y * width/2 * 8];
		motion_comp_block(0, mb[i].cb_blocks, dst, width/2);
	}
	asm volatile("emms\n\t");
}
