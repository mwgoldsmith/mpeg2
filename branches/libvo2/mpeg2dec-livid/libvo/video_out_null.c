/* 
 *  video_out_null.c
 *
 *	Copyright (C) Aaron Holtzman - June 2000
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

#include <inttypes.h>

#include "video_out.h"
#include "video_out_internal.h"

static int null_close (void * plugin)
{
    libvo_common_close();
    return 0;
}

static int null_draw_slice (uint8_t * src[], int slice_num)
{
    return 0;
}

static int null_draw_frame (frame_t * frame)
{
    return 0;
}

static void null_flip_page (void)
{
}

static int null_setup (vo_output_video_attr_t *attr)
{
    libvo_common_setup (attr);
    return 0;
}

static frame_t * null_allocate_image_buffer (int width, int height,
					     uint32_t format)
{
    return libvo_common_alloc (width, height);
}

static void null_free_image_buffer (frame_t * frame)
{
    libvo_common_free (frame);
}

LIBVO_EXTERN (null, "null")
