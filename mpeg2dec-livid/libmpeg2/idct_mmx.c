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
#include <mmx.h>
#include "mpeg2.h"
#include "mpeg2_internal.h"

#include "idct.h"

void idct_block_mmx (int16_t * block);

void idct_block_copy_mmx (int16_t * block, uint8_t * dest, int stride)
{
	int i;

	idct_block_mmx (block);

	i = 8;
	do {
		movq_m2r (*block, mm1);
		movq_m2r (*(block+4), mm2);
		packuswb_r2r (mm2, mm1);
		movq_r2m (mm1, *dest);

		block += 8;
		dest += stride;
	} while (--i);
}

void idct_block_add_mmx (int16_t * block, uint8_t * dest, int stride)
{
	int i;

	idct_block_mmx (block);

	pxor_r2r (mm0, mm0);
	i = 8;
	do {
		movq_m2r (*dest, mm1);
		movq_r2r (mm1, mm2);
		punpcklbw_r2r (mm0, mm1);
		punpckhbw_r2r (mm0, mm2);
		movq_m2r (*block, mm3);
		paddsw_r2r (mm3, mm1);
		movq_m2r (*(block+4), mm4);
		paddsw_r2r (mm4, mm2);
		packuswb_r2r (mm2, mm1);
		movq_r2m (mm1, *dest);

		block += 8;
		dest += stride;
	} while (--i);
}
