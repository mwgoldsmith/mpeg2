/*
 *  soft_video.c
 *
 *  Copyright (C) Martin Norbäck <d95mback@dtek.chalmers.se> - Jan 2000
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
 *  This should serve as a reference implementation. Optimized versions
 *  in assembler is a must for speed.
 *
 */

#define clip_to_u8(x) clip[x]

// Add a motion compensated 8x8 block to the current block
static inline void
soft_VideoAddBlock_U8_S16 (uint_8 *curr_block,
			   sint_16 *mc_block, 
			   sint_32 stride) 
{
  int x,y;
  int jump = stride - 8;

  for (y = 0; y < 8; y++) {
    for (x = 0; x < 8; x++)
      *curr_block++ = clip_to_u8(*curr_block + *mc_block++);
    curr_block += jump;
  }
}  

// VideoCopyRef - Copy block from reference block to current block
// ---------------------------------------------------------------
static inline void
soft_VideoCopyRefAve_U8_U8_16x16 (uint_8 *curr_block,
				  uint_8 *ref_block,
				  sint_32 stride,
				  sint_32 height)
{
  int x,y;
  int jump = stride - 16;
  
  for (y = 0; y < height; y++) {
    for (x = 0; x < 16; x++)
      *curr_block++ = clip_to_u8((*curr_block + *ref_block++)/2);
    ref_block += jump;
    curr_block += jump;
  }
}

static inline void
soft_VideoCopyRefAve_U8_U8_8x8 (uint_8 *curr_block,
				uint_8 *ref_block,
				sint_32 stride,
				sint_32 height)
{
  int x,y;
  int jump = stride - 8;

  for (y = 0; y < height; y++) {
    for (x = 0; x < 8; x++)
      *curr_block++ = clip_to_u8((*curr_block + *ref_block++)/2);
    ref_block += jump;
    curr_block += jump;
  }
}

static inline void
soft_VideoCopyRef_U8_U8 (uint_8 *curr_block,
			 uint_8 *ref_block,
			 sint_32 width,
			 sint_32 height,
			 sint_32 stride)
{
  int x, y;
  int jump = stride - width;
  
  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++)
      *curr_block++ = *ref_block++;
    ref_block += jump;
    curr_block += jump;
  }
}

static inline void
soft_VideoCopyRef_U8_U8_16x16 (uint_8 *curr_block,
			       uint_8 *ref_block,
			       sint_32 stride,
			       sint_32 height)
{
  int x,y;
  int jump = stride - 16;

  for (y = 0; y < height; y++) {
    for (x = 0; x < 16; x++)
      *curr_block++ = *ref_block++;
    ref_block += jump;
    curr_block += jump;
  }
}

static inline void
soft_VideoCopyRef_U8_U8_8x8 (uint_8 *curr_block,
			     uint_8 *ref_block,
			     sint_32 stride,
			     sint_32 height)
{
  int x,y;
  int jump = stride - 8;

  for (y = 0; y < height; y++) {
    for (x = 0; x < 8; x++)
      *curr_block++ = *ref_block++;
    ref_block += jump;
    curr_block += jump;
  }
}

// VideoInterp*X - Half pixel interpolation in the x direction
// ------------------------------------------------------------------
static inline void 
soft_VideoInterpAveX_U8_U8_16x16(uint_8 *curr_block, 
				 uint_8 *ref_block, 
				 sint_32 stride,   
				 sint_32 height) 
{
  int x, y;
  int jump = stride - 16;

  for (y = 0; y < height; y++) {
    for (x = 0; x < 16; x++)
      *curr_block++ = clip_to_u8((*curr_block + (*ref_block++ + *(ref_block + 1))/2)/2);
    ref_block += jump;
    curr_block += jump;
  }
}

static inline void 
soft_VideoInterpAveX_U8_U8_8x8(uint_8 *curr_block, 
			       uint_8 *ref_block, 
			       sint_32 stride,   
			       sint_32 height) 
{
  int x, y;
  int jump = stride - 8;

  for (y = 0; y < height; y++) {
    for (x = 0; x < 8; x++)
      *curr_block++ = clip_to_u8((*curr_block + (*ref_block++ + *(ref_block + 1))/2)/2);
    ref_block += jump;
    curr_block += jump;
  }
}

static inline void
soft_VideoInterpX_U8_U8_16x16(uint_8 *curr_block, 
			      uint_8 *ref_block, 
			      sint_32 stride,   
			      sint_32 height) 
{
  int x, y;
  int jump = stride - 16;

  for (y = 0; y < height; y++) {
    for (x = 0; x < 16; x++)
      *curr_block++ = clip_to_u8((*ref_block++ + *(ref_block + 1))/2);
    ref_block += jump;
    curr_block += jump;
  }
}

static inline void
soft_VideoInterpX_U8_U8_8x8(uint_8 *curr_block, 
			    uint_8 *ref_block, 
			    sint_32 stride,   
			    sint_32 height) 
{
  int x, y;
  int jump = stride - 8;

  for (y = 0; y < height; y++) {
    for (x = 0; x < 8; x++)
      *curr_block++ = clip_to_u8((*ref_block++ + *(ref_block + 1))/2);
    ref_block += jump;
    curr_block += jump;
  }
}

// VideoInterp*XY - half pixel interpolation in both x and y directions
// --------------------------------------------------------------------
static inline void 
soft_VideoInterpAveXY_U8_U8_16x16(uint_8 *curr_block, 
				  uint_8 *ref_block, 
				  sint_32 stride,   
				  sint_32 height) 
{
  int x,y;
  int jump = stride - 16;
  uint_8 *ref_block_next = ref_block+stride;

  for (y = 0; y < height; y++) {
    for (x = 0; x < 16; x++)
      *curr_block++ = clip_to_u8((*curr_block + (*ref_block++ + *(ref_block + 1) + *ref_block_next++ + *(ref_block_next + 1))/4)/2);
    curr_block += jump;
    ref_block += jump;
    ref_block_next += jump;
  }
}
     
static inline void 
soft_VideoInterpAveXY_U8_U8_8x8(uint_8 *curr_block, 
				uint_8 *ref_block, 
				sint_32 stride,   
				sint_32 height) 
{
  int x,y;
  int jump = stride - 8;
  uint_8 *ref_block_next = ref_block+stride;

  for (y = 0; y < height; y++) {
    for (x = 0; x < 8; x++)
      *curr_block++ = clip_to_u8((*curr_block + (*ref_block++ + *(ref_block + 1) + *ref_block_next++ + *(ref_block_next + 1))/4)/2);
    curr_block += jump;
    ref_block += jump;
    ref_block_next += jump;
  }
}
     
static inline void 
soft_VideoInterpXY_U8_U8_16x16(uint_8 *curr_block, 
			       uint_8 *ref_block, 
			       sint_32 stride,   
			       sint_32 height) 
{
  int x,y;
  int jump = stride - 16;
  uint_8 *ref_block_next = ref_block+stride;

  for (y = 0; y < height; y++) {
    for (x = 0; x < 16; x++)
      *curr_block++ = clip_to_u8((*ref_block++ + *(ref_block + 1) + *ref_block_next++ + *(ref_block_next + 1))/4);
    curr_block += jump;
    ref_block += jump;
    ref_block_next += jump;
  }
}
     
static inline void 
soft_VideoInterpXY_U8_U8_8x8(uint_8 *curr_block, 
			     uint_8 *ref_block, 
			     sint_32 stride,   
			     sint_32 height) 
{
  int x,y;
  int jump = stride - 8;
  uint_8 *ref_block_next = ref_block+stride;

  for (y = 0; y < height; y++) {
    for (x = 0; x < 8; x++)
      *curr_block++ = clip_to_u8((*ref_block++ + *(ref_block + 1) + *ref_block_next++ + *(ref_block_next + 1))/4);
    curr_block += jump;
    ref_block += jump;
    ref_block_next += jump;
  }
}

// VideoInterp*Y - half pixel interpolation in the y direction
// -----------------------------------------------------------
static inline void 
soft_VideoInterpAveY_U8_U8_16x16(uint_8 *curr_block, 
				 uint_8 *ref_block, 
				 sint_32 stride,   
				 sint_32 height) 
{
  int x,y;
  int jump = stride - 16;
  uint_8 *ref_block_next = ref_block+stride;
  
  for (y = 0; y < height; y++) {
    for (x = 0; x < 16; x++)
      *curr_block++ = clip_to_u8((*curr_block + (*ref_block++ + *ref_block_next++)/2)/2);
    curr_block     += jump;
    ref_block      += jump;
    ref_block_next += jump;
  }
}
     
static inline void 
soft_VideoInterpAveY_U8_U8_8x8(uint_8 *curr_block, 
			       uint_8 *ref_block, 
			       sint_32 stride,   
			       sint_32 height) 
{
  int x,y;
  int jump = stride - 8;
  uint_8 *ref_block_next = ref_block+stride;
  
  for (y = 0; y < height; y++) {
    for (x = 0; x < 8; x++)
      *curr_block++ = clip_to_u8((*curr_block + (*ref_block++ + *ref_block_next++)/2)/2);
    curr_block     += jump;
    ref_block      += jump;
    ref_block_next += jump;
  }
}

static inline void 
soft_VideoInterpY_U8_U8_16x16(uint_8 *curr_block, 
			      uint_8 *ref_block, 
			      sint_32 stride,   
			      sint_32 height) 
{
  int x,y;
  int jump = stride - 16;
  uint_8 *ref_block_next = ref_block+stride;
  
  for (y = 0; y < height; y++) {
    for (x = 0; x < 16; x++)
      *curr_block++ = clip_to_u8((*ref_block++ + *ref_block_next++)/2);
    curr_block     += jump;
    ref_block      += jump;
    ref_block_next += jump;
  }
}
     
     
static inline void 
soft_VideoInterpY_U8_U8_8x8(uint_8 *curr_block, 
			    uint_8 *ref_block, 
			    sint_32 stride,   
			    sint_32 height) 
{
  int x,y;
  int jump = stride - 8;
  uint_8 *ref_block_next = ref_block+stride;
  
  for (y = 0; y < height; y++) {
    for (x = 0; x < 8; x++)
      *curr_block++ = clip_to_u8((*ref_block++ + *ref_block_next++)/2);
    curr_block     += jump;
    ref_block      += jump;
    ref_block_next += jump;
  }
}
