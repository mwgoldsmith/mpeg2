/* 
 *    video_out_mga.c
 *
 *	Copyright (C) Aaron Holtzman - Aug 1999
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

#include "config.h"

#ifdef LIBVO_MGA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <inttypes.h>

#include "video_out.h"
#include "video_out_internal.h"

#include "hw_bes.h"
#include "attributes.h"
#include "mmx.h"

static void yuvinterleave (uint8_t * dst, uint8_t * pu, uint8_t * pv,
			   int width)
{
    width >>= 3;
    do {
	dst[0] = pu[0];
	dst[1] = pv[0];
	dst[2] = pu[1];
	dst[3] = pv[1];
	dst[4] = pu[2];
	dst[5] = pv[2];
	dst[6] = pu[3];
	dst[7] = pv[3];
	dst += 8;
	pu += 4;
	pv += 4;
    } while (--width);
}

static void yuv2g200_c (uint8_t * dst, uint8_t * py,
			uint8_t * pu, uint8_t * pv,
			int width, int height,
			int bes_stride, int y_stride, int uv_stride)
{
    int i;

    i = height;
    do {
	memcpy (dst, py, width);
	py += y_stride;
	dst += bes_stride;
    } while (--i);

    i = height >> 1;
    do {
	yuvinterleave (dst, pu, pv, width);
	pu += uv_stride;
	pv += uv_stride;
	dst += bes_stride;
    } while (--i);
}

static void yuv2g400_c (uint8_t * dst, uint8_t * py,
			uint8_t * pu, uint8_t * pv,
			int width, int height,
			int bes_stride, int y_stride, int uv_stride)
{
    int i;

    i = height;
    do {
	memcpy (dst, py, width);
	py += y_stride;
	dst += bes_stride;
    } while (--i);

    width >>= 1;
    bes_stride >>= 1;
    i = height >> 1;
    do {
	memcpy (dst, pu, width);
	pu += uv_stride;
	dst += bes_stride;
    } while (--i);

    i = height >> 1;
    do {
	memcpy (dst, pv, width);
	pv += uv_stride;
	dst += bes_stride;
    } while (--i);
}

static struct mga_priv_s {
    int fd;
    mga_vid_config_t mga_vid_config;
    uint8_t * vid_data;
    uint8_t * frame0;
    uint8_t * frame1;
    int next_frame;
    int stride;
} mga_priv = {-1};

static int mga_setup (int width, int height)
{
    struct mga_priv_s * priv = &mga_priv;
    char *frame_mem;
    int frame_size;

    if (priv->fd < 0) {
	priv->fd = open ("/dev/mga_vid", O_RDWR);
	if (priv->fd < 0) {
	    //LOG (LOG_DEBUG, "Can't open /dev_mga_vid"); 
	    return -1;
	}

	if (ioctl (priv->fd, MGA_VID_ON, 0)) {
	    //LOG (LOG_DEBUG, "Can't ioctl /dev_mga_vid"); 
	    close (priv->fd);
	    return -1;
	}
    }

    priv->mga_vid_config.src_width = width;
    priv->mga_vid_config.src_height = height;
    priv->mga_vid_config.dest_width = width;
    priv->mga_vid_config.dest_height = height;
    priv->mga_vid_config.x_org = 10;
    priv->mga_vid_config.y_org = 10;
    priv->mga_vid_config.colkey_on = 0; // 1;

    if (ioctl (priv->fd, MGA_VID_CONFIG, &priv->mga_vid_config))
	perror ("Error in priv->mga_vid_config ioctl");
    ioctl (priv->fd, MGA_VID_ON, 0);

    priv->stride = (width + 31) & ~31;
    frame_size = priv->stride * height * 3 / 2;
    frame_mem = (char*)mmap (0, frame_size*2, PROT_WRITE, MAP_SHARED, priv->fd, 0);
    priv->frame0 = frame_mem;
    priv->frame1 = frame_mem + frame_size;
    priv->vid_data = frame_mem;
    priv->next_frame = 0;

    libvo_common_alloc_frames (libvo_common_alloc_frame, width, height);

    return 0;
}

static int mga_close (void)
{
    struct mga_priv_s * priv = &mga_priv;

    close (priv->fd);
    libvo_common_free_frames (libvo_common_free_frame);

    return 0;
}

static void mga_draw_frame (frame_t * frame)
{
    struct mga_priv_s * priv = &mga_priv;

    yuv2g400_c (priv->vid_data,
		frame->base[0], frame->base[1], frame->base[2],
		priv->mga_vid_config.src_width,
		priv->mga_vid_config.src_height,
		priv->stride, priv->mga_vid_config.src_width,
		priv->mga_vid_config.src_width >> 1);

    ioctl (priv->fd, MGA_VID_FSEL, &priv->next_frame);

    priv->next_frame ^= 2; // switch between fields A1 and B1
    if (priv->next_frame) 
	priv->vid_data = priv->frame1;
    else
	priv->vid_data = priv->frame0;
}

//FIXME this should allocate AGP memory via agpgart and then we
//can use AGP transfers to the framebuffer

vo_output_video_t video_out_mga = {
    "mga",
    mga_setup, mga_close, libvo_common_get_frame, mga_draw_frame
};

#endif
