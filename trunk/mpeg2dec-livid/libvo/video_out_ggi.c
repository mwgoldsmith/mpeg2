/* 
 *  video_out_ggi.c
 *
 *  Copyright 2000 Marcus Sundberg     [marcus@ggi-project.org]
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

#ifdef LIBVO_GGI

#include <stdlib.h>

#include <oms/oms.h>
#include <oms/plugin/output_video.h>

#include <ggi/ggi.h>
#include "yuv2rgb.h"


static int _ggi_open		(void *plugin, void *name);
static int _ggi_close		(void *plugin);
static int _ggi_setup		(vo_output_video_attr_t *attr);
static int _ggi_draw_frame	(uint8_t *src[]);
static int _ggi_draw_slice	(uint8_t *src[], uint32_t slice_num);
static void _ggi_flip_page	(void);
static void _ggi_free_image_buffer   (frame_t* image);
static frame_t* _ggi_allocate_image_buffer(uint32_t height, uint32_t width, uint32_t format);


static ggi_visual_t vis = NULL;
static ggi_mode mode;
static int doublebuffer = 0;
static const ggi_directbuffer *db[2] = { NULL, NULL };
static int curdb = 0;
static int pixelsize;
static uint_32 image_width, image_height;


static vo_output_video_t video_ggi = {
	open:		_ggi_open,
	close:		_ggi_close,
	setup:		_ggi_setup,
	draw_frame:	_ggi_draw_frame,
	draw_slice:	_ggi_draw_slice,
	flip_page:	_ggi_flip_page,
	allocate_image_buffer:	_ggi_allocate_image_buffer,
	free_image_buffer:	_ggi_free_image_buffer
};


static void ggi_cleanup(void)
{
	if (vis) {
		ggiClose(vis);
		vis = NULL;
	}
	ggiExit();
}


static int ggi_start (uint_32 width, uint_32 height, uint_32 fullscreen, char *title)
{
	if (ggiInit() < 0) {
		fprintf(stderr, "GGI: Unable to initialize LibGGI.\n");
		return -1;
	}

	if (!(vis = ggiOpen(NULL))) {
		fprintf(stderr, "GGI: Unable to open default visual.\n");
		ggiExit();
		return -1;
	}
	ggiSetFlags (vis, GGIFLAG_ASYNC);

	/* Get suitable mode */
	if (ggiCheckSimpleMode(vis, width, height, 2, GT_16BIT, &mode) != 0) {
		if (mode.visible.x >= width && mode.visible.y >= height
			&& GT_SIZE(mode.graphtype) >= 16 &&
			GT_SCHEME(mode.graphtype) == GT_TRUECOLOR) {
		} else {
			fprintf(stderr, "GGI: Unable to find mode.\n");
			ggi_cleanup();
			return -1;
		}
	}

	if (ggiSetMode(vis, &mode) != 0) {
		fprintf(stderr, "GGI: Unable to set checked mode!?\n");
		ggi_cleanup();
		return -1;
	}

	if (mode.frames >= 2) {
		doublebuffer = 1;
	} else {
		doublebuffer = 0;
	}

	db[0] = ggiDBGetBuffer(vis, 0);
	db[1] = ggiDBGetBuffer(vis, 1);

	if (db[0] == NULL || !(db[0]->type & GGI_DB_SIMPLE_PLB) ||
		(doublebuffer &&
		(db[1] == NULL || !(db[1]->type & GGI_DB_SIMPLE_PLB)))) {
		fprintf(stderr, "GGI: Suitable DirectBuffer not available.\n");
		ggi_cleanup();
		return -1;
	}

	curdb = 0;

	return 0;
}


static void do_blit(uint_8 *src[], uint_32 slice_num, int lines)
{
	uint_8 *dst;
	int stride;

	if (ggiResourceAcquire(db[curdb]->resource, GGI_ACTYPE_WRITE))
		return;

	stride = db[curdb]->buffer.plb.stride;
	dst = db[curdb]->write;
	dst += stride * lines * slice_num;
	yuv2rgb(dst, src[0], src[1], src[2], image_width, lines,
		stride, image_width, image_width/2);
	ggiResourceRelease(db[curdb]->resource);
}


static int _ggi_open (void *plugin, void *name)
{
	ggi_cleanup ();
        return 0;
}


static int _ggi_close (void *plugin)
{
	return 0;
}


static int _ggi_draw_slice (uint8_t *src[], uint32_t slice_num)
{
	do_blit (src, slice_num, 16);
	return 0;
}


static int _ggi_draw_frame (uint8_t *src[])
{
	do_blit (src, 0, image_height);
	return 0;
}


static void _ggi_flip_page(void)
{
	struct timeval tv = { 0, 0 };

	while (ggiEventPoll(vis, emKey, &tv)) {
		ggi_event ev;

		ggiEventRead(vis, &ev, emKey);

		switch (ev.any.type) {
		case evKeyPress:
			if (ev.key.label == 'Q' ||
				ev.key.label == GIIUC_Escape) {
				ggi_cleanup();
				exit(0);
			}
			break;
		default:
			break;
		}
	}

	if (doublebuffer) {
		ggiSetDisplayFrame(vis, curdb);
		curdb = (curdb + 1) & 1;
		ggiSetWriteFrame(vis, curdb);
	}

	ggiFlush(vis);
}


static int _ggi_setup (vo_output_video_attr_t *attr)
{
       static int started = 0;
       int rgbtype;

       if (started) {
               fprintf(stderr, "GGI: Already running!\n");
               exit(1);
       }

	if (ggi_start (attr->width, attr->height, attr->fullscreen, attr->title))
		return -1;

	started = 1;

	pixelsize = GT_SIZE (mode.graphtype);
	image_height = attr->height;
	image_width = attr->width;

	if (pixelsize == 24) {
		rgbtype = MODE_BGR;
	} else {
		rgbtype = MODE_RGB;
	}

	yuv2rgb_init (pixelsize, rgbtype);

	return 0;
}


static frame_t* _ggi_allocate_image_buffer (uint32_t height, uint32_t width, uint32_t format)
{
    return libvo_common_alloc (width, height);
}


static void _ggi_free_image_buffer (frame_t* image)
{
    libvo_common_free (image);
}


int PLUGIN_INIT (video_out_ggi) (char *whoami)
{
	pluginRegister (whoami,
		PLUGIN_ID_OUTPUT_VIDEO,
		"ggi",
		NULL,
		NULL,
		&video_ggi);

	return 0;
}


void PLUGIN_EXIT (video_out_ggi) (void)
{
}

#endif
