/*
 *  idct_mmx.c
 *
 *  Copyright (C) Aaron Holtzman <aholtzma@ess.engr.uvic.ca> - Nov 1999
 *
 *  Portions of this code are from the MPEG software simulation group
 *  idct implementation. This code will be replaced with a new
 *  implementation soon.
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

#include <stdio.h>
#include "mpeg2.h"
#include "mpeg2_internal.h"

#include "mb_buffer.h"
#include "idct.h"

void idct_block_mmx(sint_16* foo);

void
idct_mmx(mb_buffer_t *mb_buffer)
{
	uint_32 k;
	macroblock_t *mb = mb_buffer->macroblocks;
	uint_32 num_blocks = mb_buffer->num_blocks;
	
	for(k=0;k<num_blocks;k++)
	{
		if(mb[k].skipped)
			continue;

		//XXX only 4:2:0 supported here
		if(mb[k].coded_block_pattern & 0x20)
			idct_block_mmx(mb[k].y_blocks + 64*0);
		if(mb[k].coded_block_pattern & 0x10)
			idct_block_mmx(mb[k].y_blocks + 64*1);
		if(mb[k].coded_block_pattern & 0x08)
			idct_block_mmx(mb[k].y_blocks + 64*2);
		if(mb[k].coded_block_pattern & 0x04)
			idct_block_mmx(mb[k].y_blocks + 64*3);

		if(mb[k].coded_block_pattern & 0x2)
			idct_block_mmx(mb[k].cr_blocks);

		if(mb[k].coded_block_pattern & 0x1)
			idct_block_mmx(mb[k].cb_blocks);
	}
	asm volatile("emms\n\t");
}


