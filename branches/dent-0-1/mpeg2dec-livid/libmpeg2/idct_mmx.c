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
#include <inttypes.h>

#include "mpeg2.h"
#include "mpeg2_internal.h"

#include "idct.h"

void idct_block_mmx(int16_t * foo);

void
idct_end_mmx()
{
}
