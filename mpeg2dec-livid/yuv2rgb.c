/* 
 * yuv2rgb.c, Software YUV to RGB coverter
 *
 *  Copyright (C) 1999, Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *  All Rights Reserved. 
 * 
 *  Functions broken out from display_x11.c and several new modes
 *  added by Håkan Hjort <d95hjort@dtek.chalmers.se>
 * 
 *  15 & 16 bpp support by Franck Sicard <Franck.Sicard@solsoft.fr>
 *
 *  This file is part of mpeg2dec, a free MPEG-2 video decoder
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
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "config.h"

#ifdef __i386__
#include "libmpeg2/mmx.h"
#endif 

#include "libmpeg2/mpeg2.h"
#include "libmpeg2/mpeg2_internal.h" // Only for config.flags
#include "yuv2rgb.h"
#include "yuv2rgb_mlib.h"

uint_32 matrix_coefficients = 6;

const sint_32 Inverse_Table_6_9[8][4] =
{
  {117504, 138453, 13954, 34903}, /* no sequence_display_extension */
  {117504, 138453, 13954, 34903}, /* ITU-R Rec. 709 (1990) */
  {104597, 132201, 25675, 53279}, /* unspecified */
  {104597, 132201, 25675, 53279}, /* reserved */
  {104448, 132798, 24759, 53109}, /* FCC */
  {104597, 132201, 25675, 53279}, /* ITU-R Rec. 624-4 System B, G */
  {104597, 132201, 25675, 53279}, /* SMPTE 170M */
  {117579, 136230, 16907, 35559}  /* SMPTE 240M (1987) */
};


yuv2rgb_fun yuv2rgb_c_init(uint_32 bpp, uint_32 mode);

yuv2rgb_fun yuv2rgb;

void yuv2rgb_init(uint_32 bpp, uint_32 mode) 
{
#ifdef __i386__  
  if(0 && mmx_ok())
    //yuv2rgb = yuv2rgb_mmx_init(bpp, mode);
		1;
  else
#endif
#ifdef HAVE_MLIB
  if(1 || config.flags & MPEG2_MLIB_ENABLE) // Fix me
    yuv2rgb = yuv2rgb_mlib_init(bpp, mode);
  else
#endif
    ;
  
  if( yuv2rgb == NULL ) {
    fprintf( stderr, "No accelerated colorspace coversion found\n" );
    yuv2rgb = yuv2rgb_c_init(bpp, mode);
  }
}





// Clamp to [0,255]
static uint_8 clip_tbl[1024]; /* clipping table */
static uint_8 *clip;

static uint_16* lookUpTable = NULL;


static void YUV2ARGB420_32(uint_8* image, const uint_8* py, const uint_8* pu,
			   const uint_8* pv, const uint_32 h_size,
			   const uint_32 v_size, const uint_32 rgb_stride,
			   const uint_32 y_stride, const uint_32 uv_stride) 
{
  sint_32 Y,U,V;
  sint_32 g_common,b_common,r_common;
  uint_32 x,y;
  
  uint_32 *dst_line_1;
  uint_32 *dst_line_2;
  const uint_8* py_line_1;
  const uint_8* py_line_2;
  volatile char prefetch;
  
  sint_32 crv,cbu,cgu,cgv;
  
  /* matrix coefficients */
  crv = Inverse_Table_6_9[matrix_coefficients][0];
  cbu = Inverse_Table_6_9[matrix_coefficients][1];
  cgu = Inverse_Table_6_9[matrix_coefficients][2];
  cgv = Inverse_Table_6_9[matrix_coefficients][3];
	
  dst_line_1 = (uint_32 *)(image);
  dst_line_2 = (uint_32 *)(image + rgb_stride);
  
  py_line_1 = py;
  py_line_2 = py + y_stride;
  
  for (y = 0; y < v_size / 2; y++) 
    {
      for (x = 0; x < h_size / 2; x++) 
	{
	  uint_32 pixel1,pixel2,pixel3,pixel4;

	  //Common to all four pixels
	  prefetch = pu[32];
	  U = (*pu++) - 128;
	  prefetch = pv[32];
	  V = (*pv++) - 128;

	  r_common = crv * V + 32768;
	  g_common = cgu * U + cgu * V - 32768;
	  b_common = cbu * U + 32768;

	  prefetch = py_line_1[32];
	  //Pixel I
	  Y = 76309 * ((*py_line_1++) - 16);
	  pixel1 = 
	    clip[(Y+r_common)>>16]<<16 |
	    clip[(Y-g_common)>>16]<<8 |
	    clip[(Y+b_common)>>16];
	  *dst_line_1++ = pixel1;
		  
	  //Pixel II
	  Y = 76309 * ((*py_line_1++) - 16);
	  pixel2 = 
	    clip[(Y+r_common)>>16]<<16 |
	    clip[(Y-g_common)>>16]<<8 |
	    clip[(Y+b_common)>>16];
	  *dst_line_1++ = pixel2;

	  //Pixel III
	  prefetch = py_line_2[32];
	  Y = 76309 * ((*py_line_2++) - 16);
	  pixel3 = 
	    clip[(Y+r_common)>>16]<<16 |
	    clip[(Y-g_common)>>16]<<8 |
	    clip[(Y+b_common)>>16];
	  *dst_line_2++ = pixel3;

	  //Pixel IV
	  Y = 76309 * ((*py_line_2++) - 16);
	  pixel4 = 
	    clip[(Y+r_common)>>16]<<16 |
	    clip[(Y-g_common)>>16]<<8|
	    clip[(Y+b_common)>>16];
	  *dst_line_2++ = pixel4;
	}

      py_line_1 += y_stride;
      py_line_2 += y_stride;
      pu += uv_stride - h_size/2;
      pv += uv_stride - h_size/2;
      dst_line_1 += rgb_stride/4;
      dst_line_2 += rgb_stride/4;
    }
}

static void YUV2ABGR420_32(uint_8* image, const uint_8* py, const uint_8* pu,
			   const uint_8* pv, const uint_32 h_size,
			   const uint_32 v_size, const uint_32 rgb_stride,
			   const uint_32 y_stride, const uint_32 uv_stride)
{
  sint_32 Y,U,V;
  sint_32 g_common,b_common,r_common;
  uint_32 x,y;
  
  uint_32 *dst_line_1;
  uint_32 *dst_line_2;
  const uint_8* py_line_1;
  const uint_8* py_line_2;
  volatile char prefetch;
  
  sint_32 crv,cbu,cgu,cgv;
  
  /* matrix coefficients */
  crv = Inverse_Table_6_9[matrix_coefficients][0];
  cbu = Inverse_Table_6_9[matrix_coefficients][1];
  cgu = Inverse_Table_6_9[matrix_coefficients][2];
  cgv = Inverse_Table_6_9[matrix_coefficients][3];
	
  dst_line_1 = (uint_32 *)(image);
  dst_line_2 = (uint_32 *)(image + rgb_stride);
  
  py_line_1 = py;
  py_line_2 = py + y_stride;
  
  for (y = 0; y < v_size / 2; y++) 
    {
      for (x = 0; x < h_size / 2; x++) 
	{
	  uint_32 pixel1,pixel2,pixel3,pixel4;

	  //Common to all four pixels
	  prefetch = pu[32];
	  U = (*pu++) - 128;
	  prefetch = pv[32];
	  V = (*pv++) - 128;

	  r_common = crv * V + 32768;
	  g_common = cgu * U + cgu * V - 32768;
	  b_common = cbu * U + 32768;

	  prefetch = py_line_1[32];
	  //Pixel I
	  Y = 76309 * ((*py_line_1++) - 16);
	  pixel1 = 
	    clip[(Y+b_common)>>16]<<16 |
	    clip[(Y-g_common)>>16]<<8 |
	    clip[(Y+r_common)>>16];
	  *dst_line_1++ = pixel1;
		  
	  //Pixel II
	  Y = 76309 * ((*py_line_1++) - 16);
	  pixel2 = 
	    clip[(Y+b_common)>>16]<<16 |
	    clip[(Y-g_common)>>16]<<8 |
	    clip[(Y+r_common)>>16];
	  *dst_line_1++ = pixel2;

	  //Pixel III
	  prefetch = py_line_2[32];
	  Y = 76309 * ((*py_line_2++) - 16);
	  pixel3 = 
	    clip[(Y+b_common)>>16]<<16 |
	    clip[(Y-g_common)>>16]<<8 |
	    clip[(Y+r_common)>>16];
	  *dst_line_2++ = pixel3;

	  //Pixel IV
	  Y = 76309 * ((*py_line_2++) - 16);
	  pixel4 = 
	    clip[(Y+b_common)>>16]<<16 |
	    clip[(Y-g_common)>>16]<<8|
	    clip[(Y+r_common)>>16];
	  *dst_line_2++ = pixel4;
	}

      py_line_1 += y_stride;
      py_line_2 += y_stride;
      pu += uv_stride - h_size/2;
      pv += uv_stride - h_size/2;
      dst_line_1 += rgb_stride/4;
      dst_line_2 += rgb_stride/4;
    }
}

static void YUV2RGB420_24(uint_8* image, const uint_8* py, const uint_8* pu,
			  const uint_8* pv, const uint_32 h_size,
			  const uint_32 v_size, const uint_32 rgb_stride,
			  const uint_32 y_stride, const uint_32 uv_stride)
{
  sint_32 Y,U,V;
  sint_32 g_common,b_common,r_common;
  uint_32 x,y;
  
  uint_8 *dst_line_1;
  uint_8 *dst_line_2;
  const uint_8* py_line_1;
  const uint_8* py_line_2;
  volatile char prefetch;
  
  sint_32 crv,cbu,cgu,cgv;
  
  /* matrix coefficients */
  crv = Inverse_Table_6_9[matrix_coefficients][0];
  cbu = Inverse_Table_6_9[matrix_coefficients][1];
  cgu = Inverse_Table_6_9[matrix_coefficients][2];
  cgv = Inverse_Table_6_9[matrix_coefficients][3];
	
  dst_line_1 = image;
  dst_line_2 = image + rgb_stride;
  
  py_line_1 = py;
  py_line_2 = py + y_stride;
  
  for (y = 0; y < v_size / 2; y++) 
    {
      for (x = 0; x < h_size / 2; x++) 
	{

	  //Common to all four pixels
	  prefetch = pu[32];
	  U = (*pu++) - 128;
	  prefetch = pv[32];
	  V = (*pv++) - 128;

	  r_common = crv * V + 32768;
	  g_common = cgu * U + cgu * V - 32768;
	  b_common = cbu * U + 32768;

	  prefetch = py_line_1[32];
	  //Pixel I
	  Y = 76309 * ((*py_line_1++) - 16);
	  *dst_line_1++ = clip[(Y+r_common)>>16];
	  *dst_line_1++ = clip[(Y-g_common)>>16];
	  *dst_line_1++ = clip[(Y+b_common)>>16];
		  
	  //Pixel II
	  Y = 76309 * ((*py_line_1++) - 16);
	  *dst_line_1++ = clip[(Y+r_common)>>16];
	  *dst_line_1++ = clip[(Y-g_common)>>16];
	  *dst_line_1++ = clip[(Y+b_common)>>16];

	  //Pixel III
	  prefetch = py_line_2[32];
	  Y = 76309 * ((*py_line_2++) - 16);
	  *dst_line_2++ = clip[(Y+r_common)>>16];
	  *dst_line_2++ = clip[(Y-g_common)>>16];
	  *dst_line_2++ = clip[(Y+b_common)>>16];

	  //Pixel IV
	  Y = 76309 * ((*py_line_2++) - 16);
	  *dst_line_2++ = clip[(Y+r_common)>>16];
	  *dst_line_2++ = clip[(Y-g_common)>>16];
	  *dst_line_2++ = clip[(Y+b_common)>>16];
	}

      py_line_1 += y_stride;
      py_line_2 += y_stride;
      pu += uv_stride - h_size/2;
      pv += uv_stride - h_size/2;
      dst_line_1 += rgb_stride;
      dst_line_2 += rgb_stride;
    }
}

static void YUV2BGR420_24(uint_8* image, const uint_8* py, const uint_8* pu,
			  const uint_8* pv, const uint_32 h_size,
			  const uint_32 v_size, const uint_32 rgb_stride,
			  const uint_32 y_stride, const uint_32 uv_stride)
{
  sint_32 Y,U,V;
  sint_32 g_common,b_common,r_common;
  uint_32 x,y;
  
  uint_8 *dst_line_1;
  uint_8 *dst_line_2;
  const uint_8* py_line_1;
  const uint_8* py_line_2;
  volatile char prefetch;
  
  sint_32 crv,cbu,cgu,cgv;
  
  /* matrix coefficients */
  crv = Inverse_Table_6_9[matrix_coefficients][0];
  cbu = Inverse_Table_6_9[matrix_coefficients][1];
  cgu = Inverse_Table_6_9[matrix_coefficients][2];
  cgv = Inverse_Table_6_9[matrix_coefficients][3];
	
  dst_line_1 = image;
  dst_line_2 = image + rgb_stride;
  
  py_line_1 = py;
  py_line_2 = py + y_stride;
  
  for (y = 0; y < v_size / 2; y++) 
    {
      for (x = 0; x < h_size / 2; x++) 
	{

	  //Common to all four pixels
	  prefetch = pu[32];
	  U = (*pu++) - 128;
	  prefetch = pv[32];
	  V = (*pv++) - 128;

	  r_common = crv * V + 32768;
	  g_common = cgu * U + cgu * V - 32768;
	  b_common = cbu * U + 32768;

	  prefetch = py_line_1[32];
	  //Pixel I
	  Y = 76309 * ((*py_line_1++) - 16);
	  *dst_line_1++ = clip[(Y+b_common)>>16];
	  *dst_line_1++ = clip[(Y-g_common)>>16];
	  *dst_line_1++ = clip[(Y+r_common)>>16];
		  
	  //Pixel II
	  Y = 76309 * ((*py_line_1++) - 16);
	  *dst_line_1++ = clip[(Y+b_common)>>16];
	  *dst_line_1++ = clip[(Y-g_common)>>16];
	  *dst_line_1++ = clip[(Y+r_common)>>16];

	  //Pixel III
	  prefetch = py_line_2[32];
	  Y = 76309 * ((*py_line_2++) - 16);
	  *dst_line_2++ = clip[(Y+b_common)>>16];
	  *dst_line_2++ = clip[(Y-g_common)>>16];
	  *dst_line_2++ = clip[(Y+r_common)>>16];

	  //Pixel IV
	  Y = 76309 * ((*py_line_2++) - 16);
	  *dst_line_2++ = clip[(Y+b_common)>>16];
	  *dst_line_2++ = clip[(Y-g_common)>>16];
	  *dst_line_2++ = clip[(Y+r_common)>>16];
	}

      py_line_1 += y_stride;
      py_line_2 += y_stride;
      pu += uv_stride - h_size/2;
      pv += uv_stride - h_size/2;
      dst_line_1 += rgb_stride;
      dst_line_2 += rgb_stride;
    }
}

/* do 16 and 15 bpp output */
static void YUV2RGB420_16(uint_8* image, const uint_8* py, const uint_8* pu,
			  const uint_8* pv, const uint_32 h_size,
			  const uint_32 v_size, const uint_32 rgb_stride,
			  const uint_32 y_stride, const uint_32 uv_stride) 
{
  uint_32 U,V;
  uint_32 pixel_idx;
  uint_32 x,y;
  
  uint_16* dst_line_1;
  uint_16* dst_line_2;
  const uint_8* py_line_1;
  const uint_8* py_line_2;
  
  dst_line_1 = (uint_16*)(image);
  dst_line_2 = (uint_16*)(image + rgb_stride);
  
  py_line_1 = py;
  py_line_2 = py + y_stride;
  
  for (y = 0; y < v_size / 2; y++) 
    {
      for (x = 0; x < h_size / 2; x++) 
	{
	  //Common to all four pixels
	  U = (*pu++)>>2;
	  V = (*pv++)>>2;
	  pixel_idx = U<<6 | V<<12;
	  
	  //Pixel I
	  *dst_line_1++ = lookUpTable[(*py_line_1++)>>2 | pixel_idx];
	  
	  //Pixel II
	  *dst_line_1++ = lookUpTable[(*py_line_1++)>>2 | pixel_idx];
	  
	  //Pixel III
	  *dst_line_2++ = lookUpTable[(*py_line_2++)>>2 | pixel_idx];
	  
	  //Pixel IV
	  *dst_line_2++ = lookUpTable[(*py_line_2++)>>2 | pixel_idx];
	}
      py_line_1 += y_stride;
      py_line_2 += y_stride;
      pu += uv_stride - h_size/2;
      pv += uv_stride - h_size/2;
      dst_line_1 += rgb_stride/2;
      dst_line_2 += rgb_stride/2;
    }
}

/* CreateCLUT have already taken care of 15/16bit and RGB BGR issues. */
#define YUV2BGR420_16 YUV2RGB420_16


/* We don't have any 8bit support yet. */
static void YUV2RGB420_8(uint_8* image, const uint_8* py, const uint_8* pu,
		  const uint_8* pv, const uint_32 h_size,
		  const uint_32 v_size, const uint_32 rgb_stride,
		  const uint_32 y_stride, const uint_32 uv_stride)
{
  fprintf( stderr, "No support for 8bit displays.\n" );
  exit(1);
}


/* Not sure if this is a win using the LUT. Can someone try
   the direct calculation (like in the 32bpp) and compare? */
static void createCLUT(uint_32 bpp, uint_32 mode) 
{
  int i, j, k;
  uint_8 r, g, b;
  
  if (lookUpTable == NULL) {
    lookUpTable = malloc((1<<18)*sizeof(uint_16));
    
    for (i = 0; i<(1<<6); ++i) { /* Y */
      int Y = i<<2;
      
      for(j = 0; j < (1<<6); ++j) { /* Cr */
	int Cb = j<<2;
	
	for(k = 0; k < (1<<6); k++) { /* Cb */
	  int Cr = k<<2;
	  
	  /*
	    R = clp[(int)(*Y + 1.371*(*Cr-128))];  
	    V = clp[(int)(*Y - 0.698*(*Cr-128) - 0.336*(*Cr-128))]; 
	    B = clp[(int)(*Y++ + 1.732*(*Cb-128))];
	  */
	  r = clip[(Y*1000 + 1371*(Cr-128))/1000]>>3;
	  g = clip[(Y*1000 - 698*(Cr-128) - 336*(Cr-128))/1000]>>(bpp==16?2:3);
	  b = clip[(Y*1000 + 1732*(Cb-128))/1000] >> 3;
	  if( mode == MODE_RGB )
	    lookUpTable[i|(j<<6)|(k<<12)] = (r<<(bpp==16?11:10)) | (g<<5) | b;
	  else
	    lookUpTable[i|(j<<6)|(k<<12)] = (b<<(bpp==16?11:10)) | (g<<5) | r;
	}
      }
    }
  }  
}

yuv2rgb_fun yuv2rgb_c_init(uint_32 bpp, uint_32 mode) 
{  
  int i;
  
  clip = clip_tbl + 384;
  for (i= -384; i< 640; i++)
    clip[i] = (i < 0) ? 0 : ((i > 255) ? 255 : i);
  
  if( bpp == 8 ) {
    return YUV2RGB420_8;
  }
  
  if( bpp == 15 || bpp == 16 ) {
    createCLUT( bpp, mode );
    if( mode == MODE_RGB )
      return YUV2RGB420_16;
    else if( mode == MODE_BGR )
      return YUV2BGR420_16;
  }
  
  if( bpp == 24 ) {
    if( mode == MODE_RGB )
      return YUV2RGB420_24;
    else if( mode == MODE_BGR )
      return YUV2BGR420_24;
  }
  
  if( bpp == 32 ) {
    if( mode == MODE_RGB )
      return YUV2ARGB420_32;
    else if( mode == MODE_BGR )
      return YUV2ABGR420_32;
  }
  
  fprintf( stderr, "%ibpp not supported by yuv2rgb\n", bpp );
  exit(1);
}

