/*
 * video_out.c
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

#include "config.h"

#include <stdlib.h>
#include <inttypes.h>

#include "video_out.h"
#include "video_out_internal.h"


/* Externally visible list of all vo drivers */

extern vo_setup_t vo_xvshm_setup;
extern vo_setup_t vo_xv_setup;
extern vo_setup_t vo_xshm_setup;
extern vo_setup_t vo_x11_setup;
extern vo_setup_t vo_sdl_setup;
extern vo_setup_t vo_mga_setup;
extern vo_setup_t vo_null_setup;
extern vo_setup_t vo_pgm_setup;
extern vo_setup_t vo_pgmpipe_setup;
extern vo_setup_t vo_md5_setup;

vo_driver_t video_out_drivers[] =
{
#ifdef LIBVO_XV
    {"xvshm", vo_xvshm_setup},
    {"xv", vo_xv_setup},
#endif
#ifdef LIBVO_XSHM
    {"xshm", vo_xshm_setup},
#endif
#ifdef LIBVO_X11
    {"x11", vo_x11_setup},
#endif
#ifdef LIBVO_MGA
    {"mga", vo_mga_setup},
#endif
#ifdef LIBVO_SDL
    {"sdl", vo_sdl_setup},
#endif
    {"null", vo_null_setup},
    {"pgm", vo_pgm_setup},
    {"pgmpipe", vo_pgmpipe_setup},
    {"md5", vo_md5_setup},
    {NULL, NULL}
};

int libvo_common_alloc_frame (frame_t * frame, int width, int height)
{
    /* we only know how to do 4:2:0 planar yuv right now. */
    frame->private = malloc (width * height * 3 / 2);
    if (!(frame->private))
	return 1;

    frame->base[0] = frame->private;
    frame->base[1] = frame->base[0] + width * height;
    frame->base[2] = frame->base[0] + width * height * 5 / 4;

    return 0;
}

static frame_t common_frame[3];

int libvo_common_alloc_frames (int (* alloc_frame) (frame_t *, int, int),
			       int width, int height)
{
    int i;

    for (i = 0; i < 3; i++)
	if (alloc_frame (common_frame + i, width, height))
	    return 1;

    return 0;
}

void libvo_common_free_frame (frame_t * frame)
{
    free (frame->private);
}

void libvo_common_free_frames (void (* free_frame) (frame_t *))
{
    free_frame (common_frame);
    free_frame (common_frame + 1);
    free_frame (common_frame + 2);
}

frame_t * libvo_common_get_frame (vo_instance_t * this, int prediction)
{
    static int prediction_index = 0;

    if (!prediction)
	return common_frame + 2;
    else {
	prediction_index ^= 1;
	return common_frame + prediction_index;
    }
}
