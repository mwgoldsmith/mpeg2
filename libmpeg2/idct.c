/*
 *  idct.c
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


/**********************************************************/
/* inverse two dimensional DCT, Chen-Wang algorithm       */
/* (cf. IEEE ASSP-32, pp. 803-816, Aug. 1984)             */
/* 32-bit integer arithmetic (8 bit coefficients)         */
/* 11 mults, 29 adds per DCT                              */
/*                                      sE, 18.8.91       */
/**********************************************************/
/* coefficients extended to 12 bit for IEEE1180-1990      */
/* compliance                           sE,  2.1.94       */
/**********************************************************/

/* this code assumes >> to be a two's-complement arithmetic */
/* right shift: (-2)>>1 == -1 , (-3)>>1 == -2               */

#include <stdio.h>
#include "config.h"
#include "mpeg2.h"
#include "mpeg2_internal.h"

#include "idct.h"
#include "idct_mmx.h"
#include "idct_mlib.h"


// Fixed Point
// to convert from 16.0 to 12.4 fixed point
#define FP 4

#define W1 2841 /* 2048*sqrt(2)*cos(1*pi/16) */
#define W2 2676 /* 2048*sqrt(2)*cos(2*pi/16) */
#define W3 2408 /* 2048*sqrt(2)*cos(3*pi/16) */
#define W5 1609 /* 2048*sqrt(2)*cos(5*pi/16) */
#define W6 1108 /* 2048*sqrt(2)*cos(6*pi/16) */
#define W7 565  /* 2048*sqrt(2)*cos(7*pi/16) */


// idct main entry point 
static void (*idct_block)(sint_16 *block);
static void (*idct_end)();

// private prototypes 
static void idct_row_c(sint_16 *blk);
static void idct_col_s16_c(sint_16 *blk);
static void idct_block_c(sint_16 *block);
static void idct_end_c(void);

// Clamp to [-256,255]
static sint_16 clip_tbl[1024]; /* clipping table */
static sint_16 *clip;

void 
idct_init(void)
{
	sint_32 i;

	clip = clip_tbl + 512;

	for (i= -512; i< 512; i++)
		clip[i] = (i < -256) ? -256 : ((i > 255) ? 255 : i);

#ifdef __i386__
	if(config.flags & MPEG2_MMX_ENABLE)
	{
		idct_block = idct_block_mmx;
		idct_end = idct_end_mmx;
	}
	else
#endif
#ifdef HAVE_MLIB
	if(config.flags & MPEG2_MLIB_ENABLE)
	{
		idct_block = idct_block_mlib;
		idct_end = idct_end_mlib;
	}
	else
#endif
	{
		idct_block = idct_block_c;
		idct_end = idct_end_c;
	}
}

/* row (horizontal) IDCT
 *
 *           7                       pi         1
 * dst[k] = sum c[l] * src[l] * cos( -- * ( k + - ) * l )
 *          l=0                      8          2
 *
 * where: c[0]    = 128
 *        c[1..7] = 128*sqrt(2)
 */

static void inline
idct_row_c(sint_16 *blk)
{
  sint_32 x0, x1, x2, x3, x4, x5, x6, x7, x8;

  x1 = blk[4]<<7; 
  x2 = blk[6]>>4; 
  x3 = blk[2]>>4;
  x4 = blk[1]>>4; 
  x5 = blk[7]>>4; 
  x6 = blk[5]>>4; 
  x7 = blk[3]>>4;

  /* shortcut */
  if (!(x1 | x2 | x3 | x4 | x5 | x6 | x7 ))
  {
    blk[0]=blk[1]=blk[2]=blk[3]=blk[4]=blk[5]=blk[6]=blk[7]=blk[0]>>1;
    return;
  }

  x0 = (blk[0]<<7) + 128; /* for proper rounding in the fourth stage */

  /* first stage */
  x8 = W7*(x4+x5);
  x4 = x8 + (W1-W7)*x4;
  x5 = x8 - (W1+W7)*x5;
  x8 = W3*(x6+x7);
  x6 = x8 - (W3-W5)*x6;
  x7 = x8 - (W3+W5)*x7;
  
  /* second stage */
  x8 = x0 + x1;
  x0 -= x1;
  x1 = W6*(x3+x2);
  x2 = x1 - (W2+W6)*x2;
  x3 = x1 + (W2-W6)*x3;
  x1 = x4 + x6;
  x4 -= x6;
  x6 = x5 + x7;
  x5 -= x7;
  
  /* third stage */
  x7 = x8 + x3;
  x8 -= x3;
  x3 = x0 + x2;
  x0 -= x2;
  x2 = (181*(x4+x5)+128)>>8;
  x4 = (181*(x4-x5)+128)>>8;
  
  /* fourth stage */
  blk[0] = (x7+x1)>>8;
  blk[1] = (x3+x2)>>8;
  blk[2] = (x0+x4)>>8;
  blk[3] = (x8+x6)>>8;
  blk[4] = (x8-x6)>>8;
  blk[5] = (x0-x4)>>8;
  blk[6] = (x3-x2)>>8;
  blk[7] = (x7-x1)>>8;
}


/* column (vertical) IDCT
 *
 *             7                         pi         1
 * dst[8*k] = sum c[l] * src[8*l] * cos( -- * ( k + - ) * l )
 *            l=0                        8          2
 *
 * where: c[0]    = 1/1024
 *        c[1..7] = (1/1024)*sqrt(2)
 */

static void inline
idct_col_s16_c(sint_16 *blk)
{
  sint_32 x0, x1, x2, x3, x4, x5, x6, x7, x8;

  /* shortcut */
  x1 = blk[8*4]<<8; 
  x2 = blk[8*6]; 
  x3 = blk[8*2];
  x4 = blk[8*1];
  x5 = blk[8*7]; 
  x6 = blk[8*5];
  x7 = blk[8*3];

  if (!(x1  | x2 | x3 | x4 | x5 | x6 | x7 ))
  {
    blk[8*0]=blk[8*1]=blk[8*2]=blk[8*3]=blk[8*4]=blk[8*5]=blk[8*6]=blk[8*7]=
      clip[(blk[8*0]+32)>>6];
    return;
  }

  x0 = (blk[8*0]<<8) + 8192;

  /* first stage */
  x8 = W7*(x4+x5) + 4;
  x4 = (x8+(W1-W7)*x4)>>3;
  x5 = (x8-(W1+W7)*x5)>>3;
  x8 = W3*(x6+x7) + 4;
  x6 = (x8-(W3-W5)*x6)>>3;
  x7 = (x8-(W3+W5)*x7)>>3;
  
  /* second stage */
  x8 = x0 + x1;
  x0 -= x1;
  x1 = W6*(x3+x2) + 4;
  x2 = (x1-(W2+W6)*x2)>>3;
  x3 = (x1+(W2-W6)*x3)>>3;
  x1 = x4 + x6;
  x4 -= x6;
  x6 = x5 + x7;
  x5 -= x7;
  
  /* third stage */
  x7 = x8 + x3;
  x8 -= x3;
  x3 = x0 + x2;
  x0 -= x2;
  x2 = (181*(x4+x5)+128)>>8;
  x4 = (181*(x4-x5)+128)>>8;
  
  /* fourth stage */
  blk[8*0] = clip[(x7+x1)>>14];
  blk[8*1] = clip[(x3+x2)>>14];
  blk[8*2] = clip[(x0+x4)>>14];
  blk[8*3] = clip[(x8+x6)>>14];
  blk[8*4] = clip[(x8-x6)>>14];
  blk[8*5] = clip[(x0-x4)>>14];
  blk[8*6] = clip[(x3-x2)>>14];
  blk[8*7] = clip[(x7-x1)>>14];
}

void
idct_block_c(sint_16 *block)
{
	uint_32 i;

	for (i=0; i<8; i++)
		idct_row_c(block + 8*i);

	for (i=0; i<8; i++)
		idct_col_s16_c(block + i);
}
 
void
idct_end_c(void)
{
}
 
void
idct(macroblock_t *mb)
{
	//XXX only 4:2:0 supported here
	if(mb->coded_block_pattern & 0x20)
		idct_block(&mb->blocks[0*64]);
	if(mb->coded_block_pattern & 0x10)
		idct_block(&mb->blocks[1*64]);
	if(mb->coded_block_pattern & 0x08)
		idct_block(&mb->blocks[2*64]);
	if(mb->coded_block_pattern & 0x04)
		idct_block(&mb->blocks[3*64]);
	if(mb->coded_block_pattern & 0x2)
		idct_block(&mb->blocks[4*64]);
	if(mb->coded_block_pattern & 0x1)
		idct_block(&mb->blocks[5*64]);

	idct_end();
}

