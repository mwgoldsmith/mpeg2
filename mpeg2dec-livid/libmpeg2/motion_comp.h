/*
 *  motion_comp.h
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

void motion_comp_init(void);
void motion_comp_c_init(void);
void motion_comp (picture_t * picture, macroblock_t *mb);

typedef struct mc_functions_s
{
    void (* idct_copy) (uint_8 *dst, sint_16 *block, uint_32 stride);
    void (* idct_add) (uint_8 *dst, sint_16 *block, uint_32 stride);
    void (* put [8]) (uint_8 *dst, uint_8 *, sint_32, sint_32);
    void (* avg [8]) (uint_8 *dst, uint_8 *, sint_32, sint_32);
} mc_functions_t;

#define MOTION_COMP_EXTERN(x) mc_functions_t mc_functions_##x =\
{\
	motion_comp_idct_copy_##x,    motion_comp_idct_add_##x,\
	{motion_comp_put_16x16_##x,   motion_comp_put_x_16x16_##x,\
	 motion_comp_put_y_16x16_##x, motion_comp_put_xy_16x16_##x,\
	 motion_comp_put_8x8_##x,     motion_comp_put_x_8x8_##x,\
	 motion_comp_put_y_8x8_##x,   motion_comp_put_xy_8x8_##x},\
	{motion_comp_avg_16x16_##x,   motion_comp_avg_x_16x16_##x,\
	 motion_comp_avg_y_16x16_##x, motion_comp_avg_xy_16x16_##x,\
	 motion_comp_avg_8x8_##x,     motion_comp_avg_x_8x8_##x,\
	 motion_comp_avg_y_8x8_##x,   motion_comp_avg_xy_8x8_##x}\
};

extern mc_functions_t mc_functions_c;
extern mc_functions_t mc_functions_mmx;

