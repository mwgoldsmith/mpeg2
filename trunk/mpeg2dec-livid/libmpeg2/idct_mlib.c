/*
 *  idct_mlib.c
 *
 *  Copyright (C) 1999, H�kan Hjort <d95hjort@dtek.chalmers.se>
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

#include "idct.h"
#include <mlib_types.h>
#include <mlib_status.h>
#include <mlib_sys.h>
#include <mlib_video.h>

void idct_block_copy_mlib (sint_16 * block, uint_8 * dest, int stride)
{
	int i;
	uint_8 * dest2;

	dest2 = dest;
	for (i = 0; i < 8; i++)
    {
		*((uint_32 *)dest2) = 0;
		*((uint_32 *)(dest2+4)) = 0;
		dest2 += stride;
    }

	// Should we use mlib_VideoIDCT_IEEE_S16_S16 here ??
	// it's ~30% slower.
	mlib_VideoIDCT8x8_S16_S16 (block, block);
	mlib_VideoAddBlock_U8_S16 (dest, block, stride);
}

void idct_block_add_mlib (sint_16 * block, uint_8 * dest, int stride)
{
	// Should we use mlib_VideoIDCT_IEEE_S16_S16 here ??
	// it's ~30% slower.
	mlib_VideoIDCT8x8_S16_S16 (block, block);
	mlib_VideoAddBlock_U8_S16 (dest, block, stride);
}
