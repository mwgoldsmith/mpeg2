/* 
 * video_out_null.c
 * Copyright (C) 1999-2001 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
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

#include "config.h"

#include <stdlib.h>	/* only used for NULL */
#include <inttypes.h>

#include "video_out.h"
#include "video_out_internal.h"

typedef struct null_instance_s {
    vo_instance_t vo;
    int prediction_index;
    vo_frame_t * frame_ptr[3];
    vo_frame_t frame[3];
} null_instance_t;

static void null_draw_frame (vo_frame_t * frame)
{
}

static int null_setup (vo_instance_t * instance, int width, int height)
{
    return libvo_common_alloc_frames (instance, width, height,
				      sizeof (vo_frame_t),
				      NULL, NULL, null_draw_frame);
}

vo_instance_t * vo_null_open (void)
{
    null_instance_t * instance;

    instance = malloc (sizeof (null_instance_t));
    if (instance == NULL)
	return NULL;

    instance->vo.setup = null_setup;
    instance->vo.close = libvo_common_free_frames;
    instance->vo.get_frame = libvo_common_get_frame;

    return (vo_instance_t *) instance;
}

static void null_copy_slice (vo_frame_t * frame, uint8_t ** src)
{
}

static int nullslice_setup (vo_instance_t * instance, int width, int height)
{
    return libvo_common_alloc_frames (instance, width, height,
				      sizeof (vo_frame_t),
				      null_copy_slice, NULL, null_draw_frame);
}

vo_instance_t * vo_nullslice_open (void)
{
    null_instance_t * instance;

    instance = malloc (sizeof (null_instance_t));
    if (instance == NULL)
	return NULL;

    instance->vo.setup = nullslice_setup;
    instance->vo.close = libvo_common_free_frames;
    instance->vo.get_frame = libvo_common_get_frame;

    return (vo_instance_t *) instance;
}
