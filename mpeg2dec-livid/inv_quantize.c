/*
 *  inv_quantize.c
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
#include "inv_quantize.h"

static uint_8 default_intra_quantization_matrix[64] = 
{
   8, 16, 19, 22, 26, 27, 29, 34,
  16, 16, 22, 24, 27, 29, 34, 37,
  19, 22, 26, 27, 29, 34, 34, 38,
  22, 22, 26, 27, 29, 34, 37, 40,
  22, 26, 27, 29, 32, 35, 40, 48,
  26, 27, 29, 32, 35, 40, 48, 58,
  26, 27, 29, 34, 38, 46, 56, 69,
  27, 29, 35, 38, 46, 56, 69, 83
};

static uint_8 default_non_intra_quantization_matrix[64] = 
{
	16, 16, 16, 16, 16, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 16, 16 
};


//FIXME this should go
//it stays until I get this working
static inline sint_16 clip(sint_16 x) 
{ 

	if(x > 2047) 
		x = 2047; 
	else if(x < -2048) 
		x = -2048;

//	if (x != 0)
//		x = (x - 1) | 1; 

	return x;
}


static void
inv_quantize_intra_mb(const picture_t *picture,macroblock_t *mb)
{
	uint_8 *quant_matrix;
	uint_32 i;
	sint_16 *block;

	//choose the appropriate quantization matrix
	if(picture->use_custom_intra_quantizer_matrix)
		quant_matrix = picture->custom_intra_quantization_matrix;
	else 
		quant_matrix = default_intra_quantization_matrix;

	block = mb->y_blocks;
	for(i=1;i<64;i++)
	{
		block[i] = (block[i] * (sint_16) quant_matrix[i]) / 16;
		block[i] = clip(block[i]);
	}


	block = mb->y_blocks + 64;
	for(i=1;i<64;i++)
	{
		block[i] = (block[i] * quant_matrix[i]) / 16;
		block[i] = clip(block[i]);
	}


	block = mb->y_blocks + 64 * 2;
	for(i=1;i<64;i++)
	{
		block[i] = (block[i] * quant_matrix[i]) / 16;
		block[i] = clip(block[i]);
	}

	block = mb->y_blocks + 64 * 3;
	for(i=1;i<64;i++)
	{
		block[i] = (block[i] * quant_matrix[i]) / 16;
		block[i] = clip(block[i]);
	}

	block = mb->cr_blocks;
	for(i=1;i<64;i++)
	{
		block[i] = (block[i] * quant_matrix[i]) / 16;
		block[i] = clip(block[i]);
	}

	block = mb->cb_blocks;
	for(i=1;i<64;i++)
	{
		block[i] = (block[i] * quant_matrix[i]) / 16;
		block[i] = clip(block[i]);
	}

}

static void
inv_quantize_non_intra_mb(picture_t *picture,macroblock_t *mb)
{
	uint_8 *quant_matrix;
	uint_32 i;
	sint_16 *block;

	//choose the appropriate quantization matrix
	if(picture->use_custom_non_intra_quantizer_matrix)
		quant_matrix = picture->custom_non_intra_quantization_matrix;
	else 
		quant_matrix = default_non_intra_quantization_matrix;

	//FIXME we can optimize this so that we load the quantization
	//coeff once and use it on all 6 blocks at once
	block = mb->y_blocks;
	for(i=0;i<64;i++)
		block[i] = (block[i] * quant_matrix[i]) / 32;

	block = mb->y_blocks + 64;
	for(i=0;i<64;i++)
		block[i] = (block[i] * quant_matrix[i]) / 32;

	block = mb->y_blocks + 64 * 2;
	for(i=0;i<64;i++)
		block[i] = (block[i] * quant_matrix[i]) / 32;

	block = mb->y_blocks + 64 * 3;
	for(i=0;i<64;i++)
		block[i] = (block[i] * quant_matrix[i]) / 32;

	block = mb->cr_blocks;
	for(i=0;i<64;i++)
		block[i] = (block[i] * quant_matrix[i]) / 32;

	block = mb->cb_blocks;
	for(i=0;i<64;i++)
		block[i] = (block[i] * quant_matrix[i]) / 32;
}

void
inv_quantize(picture_t *picture,mb_buffer_t *mb_buffer)
{
	uint_32 i;
	macroblock_t *mb = mb_buffer->macroblocks;
	uint_32 num_blocks = mb_buffer->num_blocks;

	for(i=0;i<num_blocks;i++)
	{
#if 1
		sint_16 *block;
		uint_32 j;
		block = mb[i].y_blocks;
		if(i == 0)
		{
			fprintf(stderr,"block_before\n");
			for(j=0;j<64;j++)
			{
				fprintf(stderr,"%06d, ",block[j]);
				if(!((j+1)%8))
					fprintf(stderr,"\n");
			}
			fprintf(stderr,"\n\n");
		}
#endif

		if(mb[i].macroblock_type & MACROBLOCK_INTRA)
			inv_quantize_intra_mb(picture,&mb[i]);
		else
			inv_quantize_non_intra_mb(picture,&mb[i]);

#if 1
		if(i == 0)
		{
			fprintf(stderr,"block_after\n");
			for(j=0;j<64;j++)
			{
				fprintf(stderr,"%06d, ",block[j]);
				if(!((j+1)%8))
					fprintf(stderr,"\n");
			}
			fprintf(stderr,"\n\n");
		}
#endif
	}
}
