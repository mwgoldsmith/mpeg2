/*
 * video_out_pgm.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "video_out.h"
#include "video_out_internal.h"

typedef struct pgm_instance_s {
    vo_instance_t vo;
    int prediction_index;
    vo_frame_t frame[3];
    int width;
    int height;
    int framenum;
    char header[1024];
    char filename[128];
} pgm_instance_t;

extern vo_instance_t pgm_vo_instance;

static vo_instance_t * internal_setup (vo_instance_t * _this,
				       int width, int height,
				       void (* draw_frame) (vo_frame_t *))
{
    pgm_instance_t * this;

    this = (pgm_instance_t *)_this;
    if (this == NULL) {
	this = malloc (sizeof (pgm_instance_t));
	if (this == NULL)
	    return NULL;
    }

    if (libvo_common_alloc_frames ((vo_instance_t *)this, width, height,
				   draw_frame))
	return NULL;

    this->vo = pgm_vo_instance;
    this->width = width;
    this->height = height;
    this->framenum = -2;
    sprintf (this->header, "P5\n\n%d %d\n255\n", width, height * 3 / 2);
    
    return (vo_instance_t *)this;
}

static void internal_draw_frame (pgm_instance_t * this, FILE * file,
				 vo_frame_t * frame)
{
    int i;

    fwrite (this->header, strlen (this->header), 1, file);
    fwrite (frame->base[0], this->width, this->height, file);
    for (i = 0; i < this->height >> 1; i++) {
	fwrite (frame->base[1]+i*this->width/2, this->width/2, 1, file);
	fwrite (frame->base[2]+i*this->width/2, this->width/2, 1, file);
    }
}

static void pgm_draw_frame (vo_frame_t * frame)
{
    pgm_instance_t * this;
    FILE * file;

    this = (pgm_instance_t *) frame->this;
    if (++(this->framenum) < 0)
	return;
    sprintf (this->filename, "%d.pgm", this->framenum);
    file = fopen (this->filename, "wb");
    if (!file)
	return;
    internal_draw_frame (this, file, frame);
    fclose (file);
}

vo_instance_t * vo_pgm_setup (vo_instance_t * this, int width, int height)
{
    return internal_setup (this, width, height, pgm_draw_frame);
}

static vo_instance_t pgm_vo_instance = {
    vo_pgm_setup, libvo_common_free_frames, libvo_common_get_frame
};

static void pgmpipe_draw_frame (vo_frame_t * frame)
{
    pgm_instance_t * this;

    this = (pgm_instance_t *)frame->this;
    if (++(this->framenum) >= 0)
	internal_draw_frame (this, stdout, frame);
}

vo_instance_t * vo_pgmpipe_setup (vo_instance_t * this, int width, int height)
{
    return internal_setup (this, width, height, pgmpipe_draw_frame);
}

static void md5_draw_frame (vo_frame_t * frame)
{
    pgm_instance_t * this;
    char buf[100];

    this = (pgm_instance_t *)frame->this;
    pgm_draw_frame (frame);
    if (this->framenum < 0)
	return;
    sprintf (buf, "md5sum -b %s", this->filename);
    system (buf);
    remove (this->filename);
}

vo_instance_t * vo_md5_setup (vo_instance_t * this, int width, int height)
{
    return internal_setup (this, width, height, md5_draw_frame);
}
