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

extern vo_open_t vo_xv_open;
extern vo_open_t vo_x11_open;
extern vo_open_t vo_sdl_open;
extern vo_open_t vo_mga_open;
extern vo_open_t vo_null_open;
extern vo_open_t vo_nullslice_open;
extern vo_open_t vo_pgm_open;
extern vo_open_t vo_pgmpipe_open;
extern vo_open_t vo_md5_open;

static vo_driver_t video_out_drivers[] =
{
#ifdef LIBVO_XV
    {"xv", vo_xv_open},
#endif
#ifdef LIBVO_X11
    {"x11", vo_x11_open},
#endif
#ifdef LIBVO_MGA
    {"mga", vo_mga_open},
#endif
#ifdef LIBVO_SDL
    {"sdl", vo_sdl_open},
#endif
    {"null", vo_null_open},
    {"nullslice", vo_nullslice_open},
    {"pgm", vo_pgm_open},
    {"pgmpipe", vo_pgmpipe_open},
    {"md5", vo_md5_open},
    {NULL, NULL}
};

vo_driver_t * vo_drivers (void)
{
    return video_out_drivers;
}

typedef struct common_instance_s {
    vo_instance_t vo;
    int prediction_index;
    vo_frame_t * frame_ptr[3];
} common_instance_t;

int libvo_common_alloc_frames (vo_instance_t * _instance,
			       int width, int height, int frame_size,
			       void (* copy) (vo_frame_t *, uint8_t **),
			       void (* field) (vo_frame_t *, int),
			       void (* draw) (vo_frame_t *))
{
    common_instance_t * instance;
    int size;
    uint8_t * alloc;
    int i;

    instance = (common_instance_t *) _instance;
    instance->prediction_index = 1;
    size = width * height / 4;
    alloc = malloc (18 * size);
    if (alloc == NULL)
	return 1;

    for (i = 0; i < 3; i++) {
	instance->frame_ptr[i] =
	    (vo_frame_t *) (((char *) instance) + sizeof (common_instance_t) +
			    i * frame_size);
	instance->frame_ptr[i]->base[0] = alloc;
	instance->frame_ptr[i]->base[1] = alloc + 4 * size;
	instance->frame_ptr[i]->base[2] = alloc + 5 * size;
	instance->frame_ptr[i]->copy = copy;
	instance->frame_ptr[i]->field = field;
	instance->frame_ptr[i]->draw = draw;
	instance->frame_ptr[i]->instance = (vo_instance_t *) instance;
	alloc += 6 * size;
    }

    return 0;
}

void libvo_common_free_frames (vo_instance_t * _instance)
{
    common_instance_t * instance;

    instance = (common_instance_t *) _instance;
    free (instance->frame_ptr[0]->base[0]);
}

vo_frame_t * libvo_common_get_frame (vo_instance_t * _instance, int flags)
{
    common_instance_t * instance;

    instance = (common_instance_t *)_instance;
    if (flags & VO_PREDICTION_FLAG) {
	instance->prediction_index ^= 1;
	return instance->frame_ptr[instance->prediction_index];
    } else
	return instance->frame_ptr[2];
}
