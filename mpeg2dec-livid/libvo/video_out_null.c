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

#include <stdlib.h>	/* only used for NULL */
#include <inttypes.h>

#include "video_out.h"
#include "video_out_internal.h"

typedef struct null_instance_s {
    vo_instance_t vo;
    int prediction_index;
    vo_frame_t frame[3];
} null_instance_t;

extern vo_instance_t null_vo_instance;

static void null_draw_frame (vo_frame_t * frame)
{
}

vo_instance_t * vo_null_setup (vo_instance_t * _this, int width, int height)
{
    null_instance_t * this;

    this = (null_instance_t *)_this;
    if (this == NULL) {
	this = malloc (sizeof (null_instance_t));
	if (this == NULL)
	    return NULL;
    }

    if (libvo_common_alloc_frames ((vo_instance_t *)this, width, height,
				   null_draw_frame))
	return NULL;

    this->vo = null_vo_instance;

    return (vo_instance_t *)this;
}

static vo_instance_t null_vo_instance = {
    vo_null_setup, libvo_common_free_frames, libvo_common_get_frame
};
