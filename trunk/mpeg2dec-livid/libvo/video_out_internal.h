/*
 * video_out_internal.h
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

static uint32_t setup (vo_output_video_attr_t * attr, void *user_data);
static const vo_info_t* get_info (void);
static const vo_info_t* get_info2 (void *user_data);

static uint32_t draw_frame (uint8_t *src[]);
static uint32_t draw_frame2(uint8_t *src[], void *user_data);

static uint32_t draw_slice (uint8_t *src[], int slice_num);
static uint32_t draw_slice2 (uint8_t *src[], int slice_num, void* user_data);

static void flip_page (void);
static void flip_page2 (void* user_data);

static vo_image_buffer_t * allocate_image_buffer ();
static vo_image_buffer_t * allocate_image_buffer2 (uint32_t height, 
						   uint32_t width, 
						   uint32_t format, 
						   void *user_data);

static void free_image_buffer (vo_image_buffer_t * image);
static void free_image_buffer2 (vo_image_buffer_t* image, void *user_data);


#define LIBVO_EXTERN(x) vo_functions_t video_out_##x =\
{\
    setup,\
    get_info,\
    get_info2,\
    draw_frame,\
    draw_frame2,\
    draw_slice,\
    draw_slice2,\
    flip_page,\
    flip_page2,\
    allocate_image_buffer,\
    allocate_image_buffer2,\
    free_image_buffer,\
    free_image_buffer2\
};

//
// Generic fallback routines used by some drivers
//
vo_image_buffer_t * allocate_image_buffer_common (int width, int height, uint32_t format);
void free_image_buffer_common (vo_image_buffer_t * image);
