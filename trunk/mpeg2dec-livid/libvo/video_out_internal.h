/*
 *  video_out_internal.h
 *
 *	Copyright (C) Aaron Holtzman - Aug 1999
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
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

static uint32_t init (int width, int height, int fullscreen,
		      char * title, uint32_t format);
static const vo_info_t* get_info (void);
static uint32_t draw_frame (uint8_t *src[]);
static uint32_t draw_slice (uint8_t *src[], int slice_num);
static void flip_page (void);
static vo_image_buffer_t * allocate_image_buffer ();
static void free_image_buffer (vo_image_buffer_t * image);

#define LIBVO_EXTERN(x) vo_functions_t video_out_##x =\
{\
    init,\
    get_info,\
    draw_frame,\
    draw_slice,\
    flip_page,\
    allocate_image_buffer,\
    free_image_buffer\
};

//
// Generic fallback routines used by some drivers
//
vo_image_buffer_t * allocate_image_buffer_common (int width, int height, uint32_t format);
void free_image_buffer_common (vo_image_buffer_t * image);
