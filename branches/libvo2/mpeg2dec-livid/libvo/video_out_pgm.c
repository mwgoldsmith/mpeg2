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
#include <string.h>
#include <inttypes.h>

#include "video_out.h"
#include "video_out_internal.h"

static int image_width;
static int image_height;
static char header[1024];
static int framenum = -2;

static int pgm_close (void * plugin)
{
    libvo_common_close();
    return 0;
}

static int pgm_draw_slice (uint8_t * src[], int slice_num)
{
    return 0;
}

static int pgm_draw_frame (frame_t * frame)
{
    char filename[100];
    FILE *file;
    int i;

    if (++framenum < 0)
	return 0;

    sprintf (filename, "%d.pgm", framenum);
    if (!(file = fopen (filename, "wb")))
	return -1;

    fwrite (header, strlen (header), 1, file);
    fwrite (frame->base[0], image_width, image_height, file);
    for (i = 0; i < image_height/2; i++) {
	fwrite (frame->base[1]+i*image_width/2, image_width/2, 1, file);
	fwrite (frame->base[2]+i*image_width/2, image_width/2, 1, file);
    }
    fclose (file);

    return 0;
}

static void pgm_flip_page (void)
{
}

static int pgm_setup (vo_output_video_attr_t *attr)
{
    libvo_common_setup (attr);

    image_width = attr->width;
    image_height = attr->height;

    sprintf (header, "P5\n\n%d %d\n255\n", attr->width, attr->height*3/2);

    return 0;
}

static frame_t * pgm_allocate_image_buffer (int width, int height, uint32_t format)
{
    return libvo_common_alloc (width, height);
}

static void pgm_free_image_buffer (frame_t* frame)
{
    libvo_common_free (frame);
}

LIBVO_EXTERN (pgm, "pgm")
