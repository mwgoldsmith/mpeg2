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
extern vo_setup_t vo_sdlsw_setup;
extern vo_setup_t vo_sdlaa_setup;
extern vo_setup_t vo_mga_setup;
extern vo_setup_t vo_null_setup;
extern vo_setup_t vo_nullslice_setup;
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
    {"sdlsw", vo_sdlsw_setup},
    {"sdlaa", vo_sdlaa_setup},
#endif
    {"null", vo_null_setup},
    {"nullslice", vo_nullslice_setup},
    {"pgm", vo_pgm_setup},
    {"pgmpipe", vo_pgmpipe_setup},
    {"md5", vo_md5_setup},
    {NULL, NULL}
};

typedef struct common_instance_s {
    vo_instance_t vo;
    int prediction_index;
    vo_frame_t * frame_ptr[3];
    uint8_t private_data[0];
} common_instance_t;

int libvo_common_alloc_frames (vo_instance_t * _this, int width, int height,
			       int frame_size,
			       void (* draw) (vo_frame_t * frame))
{
    common_instance_t * this;
    int size;
    uint8_t * alloc;
    int i;

    this = (common_instance_t *)_this;
    this->prediction_index = 1;
    size = width * height / 4;
    alloc = malloc (18 * size);
    if (alloc == NULL)
	return 1;

    for (i = 0; i < 3; i++) {
	this->frame_ptr[i] =
	    (vo_frame_t *)(this->private_data + i * frame_size);
	this->frame_ptr[i]->base[0] = alloc;
	this->frame_ptr[i]->base[1] = alloc + 4 * size;
	this->frame_ptr[i]->base[2] = alloc + 5 * size;
	this->frame_ptr[i]->copy = NULL;
	this->frame_ptr[i]->field = NULL;
	this->frame_ptr[i]->draw = draw;
	this->frame_ptr[i]->this = (vo_instance_t *)this;
	alloc += 6 * size;
    }

    return 0;
}

void libvo_common_free_frames (vo_instance_t * _this)
{
    common_instance_t * this;

    this = (common_instance_t *)_this;
    free (this->frame_ptr[0]->base[0]);
}

vo_frame_t * libvo_common_get_frame (vo_instance_t * _this, int flags)
{
    common_instance_t * this;

    this = (common_instance_t *)_this;
    if (flags & VO_PREDICTION_FLAG) {
	this->prediction_index ^= 1;
	return this->frame_ptr[this->prediction_index];
    } else
	return this->frame_ptr[2];
}
