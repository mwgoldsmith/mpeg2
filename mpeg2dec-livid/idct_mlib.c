/*
 *  idct_mlib.c
 *
 *  Copyright (C) 1999, Håkan Hjort <d95hjort@dtek.chalmers.se>
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
#include <mlib_types.h>
#include <mlib_status.h>
#include <mlib_sys.h>
#include <mlib_video.h>

void
idct_mlib(mb_buffer_t *mb_buffer)
{
  uint_32 k;
  macroblock_t *mb = mb_buffer->macroblocks;
  uint_32 num_blocks = mb_buffer->num_blocks;
  
  for(k=0; k<num_blocks; k++)
    {
      if(mb[k].skipped)
	continue;
      
      // Should we use mlib_VideoIDCT_IEEE_S16_S16 here ??
      // it's ~30% slower.

      //XXX only 4:2:0 supported here
      if(mb[k].coded_block_pattern & 0x20)
	mlib_VideoIDCT8x8_S16_S16(mb[k].y_blocks + 64*0, 
				  mb[k].y_blocks + 64*0);
      if(mb[k].coded_block_pattern & 0x10)
	mlib_VideoIDCT8x8_S16_S16(mb[k].y_blocks + 64*1, 
				  mb[k].y_blocks + 64*1);
      if(mb[k].coded_block_pattern & 0x08)
	mlib_VideoIDCT8x8_S16_S16(mb[k].y_blocks + 64*2, 
				  mb[k].y_blocks + 64*2);
      if(mb[k].coded_block_pattern & 0x04)
	mlib_VideoIDCT8x8_S16_S16(mb[k].y_blocks + 64*3, 
				  mb[k].y_blocks + 64*3);
      
      if(mb[k].coded_block_pattern & 0x2)
	mlib_VideoIDCT8x8_S16_S16(mb[k].cr_blocks,
				  mb[k].cr_blocks);
      
      if(mb[k].coded_block_pattern & 0x1)
	mlib_VideoIDCT8x8_S16_S16(mb[k].cb_blocks,
				  mb[k].cb_blocks);
    }
}


