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

static int image_width;
static int image_height;
static char header[1024];
static int framenum = -2;
static char filename[100];

static vo_instance_t * pgm_setup (vo_instance_t * this, int width, int height)
{
    image_width = width;
    image_height = height;

    sprintf (header, "P5\n\n%d %d\n255\n", width, height * 3 / 2);
    
    if (libvo_common_alloc_frames (libvo_common_alloc_frame, width, height))
	return NULL;
    return (vo_instance_t *)1;
}

static int pgm_close (vo_instance_t * this)
{
    libvo_common_free_frames (libvo_common_free_frame);
    return 0;
}

static void internal_draw_frame (FILE * file, frame_t * frame)
{
    int i;

    fwrite (header, strlen (header), 1, file);
    fwrite (frame->base[0], image_width, image_height, file);
    for (i = 0; i < image_height/2; i++) {
	fwrite (frame->base[1]+i*image_width/2, image_width/2, 1, file);
	fwrite (frame->base[2]+i*image_width/2, image_width/2, 1, file);
    }
}

static void pgm_draw_frame (frame_t * frame)
{
    FILE * file;

    if (++framenum < 0)
	return;
    sprintf (filename, "%d.pgm", framenum);
    file = fopen (filename, "wb");
    if (!file)
	return;
    internal_draw_frame (file, frame);
    fclose (file);
}

vo_output_video_t video_out_pgm = {
    "pgm",
    pgm_setup, pgm_close, libvo_common_get_frame, pgm_draw_frame
};

static void pgmpipe_draw_frame (frame_t * frame)
{
    if (++framenum >= 0)
	internal_draw_frame (stdout, frame);
}

vo_output_video_t video_out_pgmpipe = {
    "pgmpipe",
    pgm_setup, pgm_close, libvo_common_get_frame, pgmpipe_draw_frame
};

static void md5_draw_frame (frame_t * frame)
{
    char buf[100];

    pgm_draw_frame (frame);
    if (framenum < 0)
	return;
    sprintf (buf, "md5sum -b %s", filename);
    system (buf);
    remove (filename);
}

vo_output_video_t video_out_md5 = {
    "md5",
    pgm_setup, pgm_close, libvo_common_get_frame, md5_draw_frame
};
