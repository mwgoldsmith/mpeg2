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

//
// Externally visible list of all vo drivers
//

extern vo_output_video_t video_out_xvshm;
extern vo_output_video_t video_out_xv;
extern vo_output_video_t video_out_xshm;
extern vo_output_video_t video_out_x11;
extern vo_output_video_t video_out_sdl;
extern vo_output_video_t video_out_null;
extern vo_output_video_t video_out_pgm;
extern vo_output_video_t video_out_md5;

vo_output_video_t* video_out_drivers[] = 
{
#ifdef LIBVO_XVSHM
	&video_out_xvshm,
#endif
#ifdef LIBVO_XV
	&video_out_xv,
#endif
#ifdef LIBVO_XSHM
	&video_out_xshm,
#endif
#ifdef LIBVO_X11
	&video_out_x11,
#endif
#ifdef LIBVO_SDL
	&video_out_sdl,
#endif
	&video_out_null,
	&video_out_pgm,
	&video_out_md5,
	NULL
};

static void alloc_frame (frame_t * frame, int width, int height)
{
    /* we only know how to do 4:2:0 planar yuv right now. */
    frame->private = malloc (width * height * 3 / 2);
    if (!(frame->private))
	exit (1);

    frame->base[0] = frame->private;
    frame->base[1] = frame->base[0] + width * height;
    frame->base[2] = frame->base[0] + width * height * 5 / 4;
}

static frame_t common_frame[3];

void libvo_common_alloc_frames (int width, int height)
{
    alloc_frame (common_frame, width, height);
    alloc_frame (common_frame + 1, width, height);
    alloc_frame (common_frame + 2, width, height);
}

void libvo_common_free_frames (void)
{
    free (common_frame[0].private);
    free (common_frame[1].private);
    free (common_frame[2].private);
}

frame_t * libvo_common_get_frame (int prediction)
{
    static int prediction_index = 0;

    if (!prediction)
	return common_frame + 2;
    else {
	prediction_index ^= 1;
	return common_frame + prediction_index;
    }
}
