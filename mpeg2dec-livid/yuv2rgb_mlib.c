/* 
 *  yuv2rgb_mlib.c, Software YUV to RGB coverter
 *
 *
 *  Copyright (C) 1999, Håkan Hjort <d95hjort@dtek.chalmers.se>
 *  All Rights Reserved. 
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
 * 
 * Functions broken out from display_x11.c and several new modes
 * added by Håkan Hjort <d95hjort@dtek.chalmers.se>
 * 
 * 15 & 16 bpp support by Franck Sicard <Franck.Sicard@solsoft.fr>
 */

#include <stdio.h>
#include <stdlib.h>

#include "mpeg2.h"
#include "yuv2rgb.h"
#include <mlib_types.h>
#include <mlib_status.h>
#include <mlib_sys.h>
#include <mlib_video.h>


void mlib_YUV2ARGB420_32(uint_8* image, const uint_8* py, const uint_8* pu,
			 const uint_8* pv, const int h_size,
			 const int v_size, const int rgb_stride,
			 const int y_stride, const int uv_stride) 
{
  mlib_VideoColorYUV2ARGB420(image, py, pu, pv, h_size,
			     v_size, rgb_stride, y_stride, uv_stride);
}

void mlib_YUV2ABGR420_32(uint_8* image, const uint_8* py, const uint_8* pu,
			 const uint_8* pv, const int h_size,
			 const int v_size, const int rgb_stride,
			 const int y_stride, const int uv_stride)
{
  mlib_VideoColorYUV2ABGR420(image, py, pu, pv, h_size,
			     v_size, rgb_stride, y_stride, uv_stride);
}

void mlib_YUV2RGB420_24(uint_8* image, const uint_8* py, const uint_8* pu,
			const uint_8* pv, const int h_size,
			const int v_size, const int rgb_stride,
			const int y_stride, const int uv_stride)
{
  mlib_VideoColorYUV2RGB420(image, py, pu, pv, h_size,
			    v_size, rgb_stride, y_stride, uv_stride);
}



yuv2rgb_fun yuv2rgb_mlib_init(int bpp, int mode) 
{  
  fprintf(stderr, "Using mediaLib for colorspace convertion\n");
 
  if( bpp == 24 ) {
    if( mode == MODE_RGB )
      return mlib_YUV2RGB420_24;
  }
  
  if( bpp == 32 ) {
    if( mode == MODE_RGB )
      return mlib_YUV2ARGB420_32;
    else if( mode == MODE_BGR )
      return mlib_YUV2ABGR420_32;
  }
  
  return NULL; // Fallback to C.
}

