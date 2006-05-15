/*
 *  motion_comp_mlib.c
 *
 *  Copyright (C) H�kan Hjort <d95hjort@dtek.chalmers.se> - Jan 2000
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
#include <inttypes.h>

#include "config.h"
#include "debug.h"
#include "mpeg2.h"
#include "mpeg2_internal.h"

#include "motion_comp.h"

#include <mlib_types.h>
#include <mlib_status.h>
#include <mlib_sys.h>
#include <mlib_video.h>


// mlib doesn't have this function...
void
motion_comp_idct_copy_mlib (uint8_t *dst, int16_t *block, uint32_t stride)
{
	uint32_t i;
	uint8_t *dst2 = dst;
  
	for (i = 0; i < 8; i++) {
		*((uint32_t *)dst2) = (uint32_t)0;
		*((uint32_t *)(dst2+4)) = (uint32_t)0;
		dst2 += stride;
	}

	mlib_VideoAddBlock_U8_S16 (dst, block, stride);
}

void
motion_comp_idct_add_mlib (uint8_t *dst, int16_t *block, uint32_t stride)
{
  mlib_VideoAddBlock_U8_S16 (dst, block, stride);
}

void
motion_comp_put_16x16_mlib (uint8_t *dst, uint8_t *block, int32_t stride, int32_t height)
{
  if (height == 16) 
    mlib_VideoCopyRef_U8_U8_16x16 (dst, block, stride);
  else
    mlib_VideoCopyRef_U8_U8_16x8 (dst, block, stride);
}

void
motion_comp_put_x_16x16_mlib (uint8_t *dst, uint8_t *block, int32_t stride, int32_t height)
{
  if (height == 16) 
    mlib_VideoInterpX_U8_U8_16x16 (dst, block, stride, stride);
  else
    mlib_VideoInterpX_U8_U8_16x8 (dst, block, stride, stride);
}

void
motion_comp_put_y_16x16_mlib (uint8_t *dst, uint8_t *block, int32_t stride, int32_t height)
{
  if (height == 16) 
    mlib_VideoInterpY_U8_U8_16x16 (dst, block, stride, stride);
  else
    mlib_VideoInterpY_U8_U8_16x8 (dst, block, stride, stride);
}

void
motion_comp_put_xy_16x16_mlib (uint8_t *dst, uint8_t *block, int32_t stride, int32_t height)
{
  if (height == 16) 
    mlib_VideoInterpXY_U8_U8_16x16 (dst, block, stride, stride);
  else
    mlib_VideoInterpXY_U8_U8_16x8 (dst, block, stride, stride);
}

void
motion_comp_put_8x8_mlib (uint8_t *dst, uint8_t *block, int32_t stride, int32_t height)
{
  if (height == 8) 
    mlib_VideoCopyRef_U8_U8_8x8 (dst, block, stride);
  else
    mlib_VideoCopyRef_U8_U8_8x4 (dst, block, stride);
}

void
motion_comp_put_x_8x8_mlib (uint8_t *dst, uint8_t *block, int32_t stride, int32_t height)
{
  if (height == 8) 
    mlib_VideoInterpX_U8_U8_8x8 (dst, block, stride, stride);
  else
    mlib_VideoInterpX_U8_U8_8x4 (dst, block, stride, stride);
}

void
motion_comp_put_y_8x8_mlib (uint8_t *dst, uint8_t *block, int32_t stride, int32_t height)
{
  if (height == 8) 
    mlib_VideoInterpY_U8_U8_8x8 (dst, block, stride, stride);
  else
    mlib_VideoInterpY_U8_U8_8x4 (dst, block, stride, stride);
}

void
motion_comp_put_xy_8x8_mlib (uint8_t *dst, uint8_t *block, int32_t stride, int32_t height)
{
  if (height == 8) 
    mlib_VideoInterpXY_U8_U8_8x8 (dst, block, stride, stride);
  else
    mlib_VideoInterpXY_U8_U8_8x4 (dst, block, stride, stride);
}

void
motion_comp_avg_16x16_mlib (uint8_t *dst, uint8_t *block, int32_t stride, int32_t height)
{
  if (height == 16) 
    mlib_VideoCopyRefAve_U8_U8_16x16 (dst, block, stride);
  else
    mlib_VideoCopyRefAve_U8_U8_16x8 (dst, block, stride);
}

void
motion_comp_avg_x_16x16_mlib (uint8_t *dst, uint8_t *block, int32_t stride, int32_t height)
{
  if (height == 16) 
    mlib_VideoInterpAveX_U8_U8_16x16 (dst, block, stride, stride);
  else
    mlib_VideoInterpAveX_U8_U8_16x8 (dst, block, stride, stride);
}

void
motion_comp_avg_y_16x16_mlib (uint8_t *dst, uint8_t *block, int32_t stride, int32_t height)
{
  if (height == 16) 
    mlib_VideoInterpAveY_U8_U8_16x16 (dst, block, stride, stride);
  else
    mlib_VideoInterpAveY_U8_U8_16x8 (dst, block, stride, stride);
}

void
motion_comp_avg_xy_16x16_mlib (uint8_t *dst, uint8_t *block, int32_t stride, int32_t height)
{
  if (height == 16) 
    mlib_VideoInterpAveXY_U8_U8_16x16 (dst, block, stride, stride);
  else
    mlib_VideoInterpAveXY_U8_U8_16x8 (dst, block, stride, stride);
}

void
motion_comp_avg_8x8_mlib (uint8_t *dst, uint8_t *block, int32_t stride, int32_t height)
{
  if (height == 8) 
    mlib_VideoCopyRefAve_U8_U8_8x8 (dst, block, stride);
  else
    mlib_VideoCopyRefAve_U8_U8_8x4 (dst, block, stride);
}

void
motion_comp_avg_x_8x8_mlib (uint8_t *dst, uint8_t *block, int32_t stride, int32_t height)
{
  if (height == 8) 
    mlib_VideoInterpAveX_U8_U8_8x8 (dst, block, stride, stride);
  else
    mlib_VideoInterpAveX_U8_U8_8x4 (dst, block, stride, stride);
}

void
motion_comp_avg_y_8x8_mlib (uint8_t *dst, uint8_t *block, int32_t stride, int32_t height)
{
  if (height == 8) 
    mlib_VideoInterpAveY_U8_U8_8x8 (dst, block, stride, stride);
  else
    mlib_VideoInterpAveY_U8_U8_8x4 (dst, block, stride, stride);
}

void
motion_comp_avg_xy_8x8_mlib (uint8_t *dst, uint8_t *block, int32_t stride, int32_t height)
{
  if (height == 8) 
    mlib_VideoInterpAveXY_U8_U8_8x8 (dst, block, stride, stride);
  else
    mlib_VideoInterpAveXY_U8_U8_8x4 (dst, block, stride, stride);
}

//MOTION_COMP_EXTERN(mlib)
