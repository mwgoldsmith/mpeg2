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

mc_functions_t mc_functions;

void
motion_comp_init (void)
{

#ifdef __i386__
	if(config.flags & MPEG2_MMX_ENABLE)
	{
		fprintf (stderr, "Using MMX for motion compensation\n");
		mc_functions = mc_functions_mmx;
	}
	else
#endif
#if HAVE_MLIB
	if(config.flags & MPEG2_MLIB_ENABLE)
	{
		fprintf (stderr, "Using mlib for motion compensation\n");
		mc_functions = mc_functions_mlib;
	}
	else
#endif
	{
		fprintf (stderr, "No accelerated motion compensation found\n");
		mc_functions = mc_functions_c;
	}
}

void motion_block (void (** table) (uint8_t *, uint8_t *, int32_t, int32_t), 
				   int x_pred, int y_pred,
				   uint8_t * dest[3], int dest_offset,
				   uint8_t * src[3], int src_offset,
				   int stride, int height)
{
	uint32_t xy_half;
	uint8_t *src1, *src2;

	xy_half = ((y_pred & 1) << 1) | (x_pred & 1);

	src1 = src[0] + src_offset + (x_pred >> 1) + (y_pred >> 1) * stride;

	table[xy_half] (dest[0] + dest_offset, src1, stride, height);

	x_pred /= 2;
	y_pred /= 2;

	xy_half = ((y_pred & 1) << 1) | (x_pred & 1);
	stride >>= 1;
	height >>= 1;
	src_offset >>= 1;
	dest_offset >>= 1;

	src1 = src[1] + src_offset + (x_pred >> 1) + (y_pred >> 1) * stride;
	src2 = src[2] + src_offset + (x_pred >> 1) + (y_pred >> 1) * stride;

	table[4+xy_half] (dest[1] + dest_offset, src1, stride, height);
	table[4+xy_half] (dest[2] + dest_offset, src2, stride, height);
}

