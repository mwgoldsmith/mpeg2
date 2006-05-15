/*
 *  mb_buffer.c
 *
 *  Copyright (C) Aaron Holtzman <aholtzma@ess.engr.uvic.ca> - Nov 1999
 *
 *  Decodes an MPEG-2 video stream.
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



//FIXME dynamically set this
#define MACROBLOCK_BUFFER_SIZE 100

macroblock_t *macroblocks;
uint_32 num_blocks = 0;

macroblock_t*
mb_buffer_init(uint_32 chroma_format)
{
	uint_32 i;

	num_blocks = 0;

	macroblocks = malloc(MACROBLOCK_BUFFER_SIZE * sizeof(macroblock_t));

	if(!macroblocks)
		return 0;

	macroblocks[0].y_blocks  = malloc(sizeof(sint_16) *  64 * 4 * MACROBLOCK_BUFFER_SIZE);
	macroblocks[0].cr_blocks = malloc(sizeof(sint_16) *  64 * MACROBLOCK_BUFFER_SIZE);
	macroblocks[0].cb_blocks = malloc(sizeof(sint_16) *  64 * MACROBLOCK_BUFFER_SIZE);

	if((!macroblocks[0].y_blocks) || (!macroblocks[0].cr_blocks) || (!macroblocks[0].cr_blocks))
		return 0;

	for(i=1;i < MACROBLOCK_BUFFER_SIZE;i++)
	{
		macroblocks[i].y_blocks  = macroblocks[i - 1].y_blocks  + 64 * 4;
		macroblocks[i].cr_blocks = macroblocks[i - 1].cr_blocks + 64;
		macroblocks[i].cb_blocks = macroblocks[i - 1].cb_blocks + 64;
	}
	
	return macroblocks;
}

macroblock_t*
mb_buffer_increment()
{
	num_blocks++;

	if (num_blocks == MACROBLOCK_BUFFER_SIZE)
		return 0;

	return &macroblocks[num_blocks];
}


void
mb_buffer_flush(mb_buffer_t *mb_buffer)
{
	mb_buffer->macroblocks = macroblocks;
	mb_buffer->num_blocks = num_blocks;
	num_blocks = 0;
}
