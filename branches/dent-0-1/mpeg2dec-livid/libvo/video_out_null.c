//PLUGIN_INFO(INFO_NAME, "NULL video output");
//PLUGIN_INFO(INFO_AUTHOR, "Aaron Holtzman <aholtzma@ess.engr.uvic.ca>");

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


#include <stdlib.h>

#include <oms/oms.h>
#include <oms/plugin/output_video.h>

static int _null_open		(plugin_t *plugin, void *name);
static int _null_close		(plugin_t *plugin);
static int _null_setup		(plugin_output_video_attr_t *attr);
static int _null_draw_frame	(uint8_t *src[]);
static int _null_draw_slice	(uint8_t *src[], uint32_t slice_num);
static void _null_flip_page	(void);
static void _null_free_image_buffer   (vo_image_buffer_t* image);
static vo_image_buffer_t* _null_allocate_image_buffer(uint32_t height, uint32_t width, uint32_t format);

static plugin_output_video_t video_null = {
	open:		_null_open,
	close:		_null_close,
	setup:		_null_setup,
	draw_frame:	_null_draw_frame,
	draw_slice:	_null_draw_slice,
	flip_page:	_null_flip_page,
	allocate_image_buffer:	_null_allocate_image_buffer,
	free_image_buffer:	_null_free_image_buffer
};


/**
 *
 **/

static int _null_open (plugin_t *plugin, void *name)
{
        return 0;
}


/**
 *
 **/

static int _null_close (plugin_t *plugin)
{
	return 0;
}


/**
 *
 **/

static int _null_draw_slice (uint8_t *src[], uint32_t slice_num)
{
	return 0;
}


/**
 *
 **/

static int _null_draw_frame (uint8_t *src[])
{
	return 0;
}


/**
 *
 **/

static void _null_flip_page(void)
{
}


/**
 *
 **/

static int _null_setup (plugin_output_video_attr_t *attr)
{
	return 0;
}


/**
 *
 **/

static vo_image_buffer_t* _null_allocate_image_buffer (uint32_t height, uint32_t width, uint32_t format)
{
	vo_image_buffer_t *image;

	if (!(image = malloc (sizeof (vo_image_buffer_t))))
		return NULL;

	image->height   = height;
	image->width    = width;
	image->format   = format;

	//we only know how to do 4:2:0 planar yuv right now.
	if (!(image->base = malloc (width * height * 3 / 2))) {
		free(image);
		return NULL;
	}

	return image;
}


/**
 *
 **/

static void _null_free_image_buffer (vo_image_buffer_t* image)
{
	free (image->base);
	free (image);
}

/**
 * Initialize Plugin.
 **/

void *plugin_init (char *whoami)
{
	pluginRegister (whoami,
		PLUGIN_ID_OUTPUT_VIDEO,
		"null",
		&video_null);

	return &video_null;
}


/**
 * Cleanup Plugin.
 **/

void plugin_exit (void)
{
}
