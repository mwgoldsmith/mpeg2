//PLUGIN_INFO(INFO_NAME, "Matrox Millennium G200/G400 (/dev/mgavid)");
//PLUGIN_INFO(INFO_AUTHOR, "Aaron Holtzman <aholtzma@ess.engr.uvic.ca>");

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <oms/oms.h>
#include <oms/plugin/output_video.h>
#include "mga_vid.h"
#include "mmx.h"

static int _mga_open		(plugin_t *plugin, void *name);
static int _mga_close		(plugin_t *plugin);
static int _mga_setup		(plugin_output_video_attr_t *attr);
static void _mga_flip_page		(void);
static void free_image_buffer	(vo_image_buffer_t* image);
static vo_image_buffer_t *allocate_image_buffer (uint32_t height, uint32_t width, uint32_t format);


static struct mga_priv_s {
	mga_vid_config_t mga_vid_config;
	uint8_t *vid_data, *frame0, *frame1;
	int next_frame;
	int fd;
	uint32_t bespitch;
	uint32_t fullscreen;
	uint32_t x_org;
	uint32_t y_org;
} _mga_priv;

static plugin_output_video_t video_mga = {
	priv:		&_mga_priv,
	open:		_mga_open,
	close:		_mga_close,
	setup:		_mga_setup,
	flip_page:	_mga_flip_page,
	allocate_image_buffer:  allocate_image_buffer,
	free_image_buffer:      free_image_buffer
};


#ifndef __i386__
/**
 *
 **/

static int _mga_write_frame_g200 (uint8_t *src[])
{
	uint8_t *dest;
	uint32_t h,w;
	uint8_t *y = src[0];
	uint8_t *cb = src[1];
	uint8_t *cr = src[2];

	dest = _mga_priv.vid_data;

	for(h=0; h < _mga_priv.mga_vid_config.src_height; h++) {
		memcpy (dest, y, _mga_priv.mga_vid_config.src_width);
		y += _mga_priv.mga_vid_config.src_width;
		dest += _mga_priv.bespitch;
	}

	for (h=0; h < _mga_priv.mga_vid_config.src_height/2; h++) {
		for (w=0; w < _mga_priv.mga_vid_config.src_width/2; w++) {
			*dest++ = *cb++;
			*dest++ = *cr++;
		}
		dest += _mga_priv.bespitch - _mga_priv.mga_vid_config.src_width;
	}
	_mga_flip_page();
	return 0;
}
#else

/**
 *
 **/

static int _mga_write_frame_g200_mmx (uint8_t *src[])
{
	uint8_t *dest;
	uint32_t h,w;
	uint8_t *y = src[0];
	uint8_t *cb = src[1];
	uint8_t *cr = src[2];

	dest = _mga_priv.vid_data;

	for(h=0; h < _mga_priv.mga_vid_config.src_height; h++) {
		memcpy (dest, y, _mga_priv.mga_vid_config.src_width);
		y += _mga_priv.mga_vid_config.src_width;
		dest += _mga_priv.bespitch;
	}

	for (h=0; h < _mga_priv.mga_vid_config.src_height/2; h++) {
		for(w=0; w < _mga_priv.mga_vid_config.src_width/16; w++) {
			asm volatile (
			"movq 		   (%1),%%mm0\n\t"
			"movq 		  %%mm0,%%mm1\n\t"
			"punpcklbw         (%2),%%mm0\n\t" 
			"punpckhbw         (%2),%%mm1\n\t" 
			"movq 		  %%mm0,(%0)\n\t"
			"movq 		  %%mm1,8(%0)\n\t"
			: "+r" (dest) : "r" (cb), "r" (cr));

			dest += 16;
			cb += 8;
			cr += 8;
		}
		dest += _mga_priv.bespitch - _mga_priv.mga_vid_config.src_width;
	}
	_mga_flip_page();

	emms ();

	return 0;
}
#endif

/**
 *
 **/

static int _mga_write_frame_g400 (uint8_t *src[])
{
	uint8_t *dest;
	int h;
	uint8_t *y = src[0];
	uint8_t *cb = src[1];
	uint8_t *cr = src[2];

	dest = _mga_priv.vid_data;

	for (h=0; h < _mga_priv.mga_vid_config.src_height; h++) {
		memcpy(dest, y, _mga_priv.mga_vid_config.src_width);
		y += _mga_priv.mga_vid_config.src_width;
		dest += _mga_priv.bespitch;
	}

	for (h=0; h < _mga_priv.mga_vid_config.src_height/2; h++) {
		memcpy(dest, cb, _mga_priv.mga_vid_config.src_width/2);
		cb += _mga_priv.mga_vid_config.src_width/2;
		dest += _mga_priv.bespitch/2;
	}

	for (h=0; h < _mga_priv.mga_vid_config.src_height/2; h++) {
		memcpy(dest, cr, _mga_priv.mga_vid_config.src_width/2);
		cr += _mga_priv.mga_vid_config.src_width/2;
		dest += _mga_priv.bespitch/2;
	}
	_mga_flip_page();
	return 0;
}

#ifndef __i386__
/**
 *
 **/

static int _mga_write_slice_g200 (uint8_t *src[], uint32_t slice_num)
{
	uint8_t *dest;
	int h,w;
	uint8_t *y = src[0];
	uint8_t *cb = src[1];
	uint8_t *cr = src[2];

	dest = _mga_priv.vid_data + _mga_priv.bespitch * 16 * slice_num;

	for (h=0; h < 16; h++) {
		memcpy (dest, y, _mga_priv.mga_vid_config.src_width);
		y += _mga_priv.mga_vid_config.src_width;
		dest += _mga_priv.bespitch;
	}

	dest = _mga_priv.vid_data +  _mga_priv.bespitch * _mga_priv.mga_vid_config.src_height + _mga_priv.bespitch * 8 * slice_num;

	for (h=0; h < 8; h++) {
		for (w=0; w < _mga_priv.mga_vid_config.src_width/2; w++) {
			*dest++ = *cb++;
			*dest++ = *cr++;
		}
		dest += _mga_priv.bespitch - _mga_priv.mga_vid_config.src_width;
	}

	return 0;
}

#else
/**
 *
 **/

static int _mga_write_slice_g200_mmx (uint8_t *src[], uint32_t slice_num)
{
	uint8_t *dest;
	int h,w;
	uint8_t *y = src[0];
	uint8_t *cb = src[1];
	uint8_t *cr = src[2];

	dest = _mga_priv.vid_data + _mga_priv.bespitch * 16 * slice_num;

	for (h=0; h < 16; h++) {
		for (w=0; w < _mga_priv.mga_vid_config.src_width/8; w++) {
			asm volatile (
				"movq       (%1),%%mm0\n\t"
				"movq       %%mm0,(%0)\n\t"
				: : "r" (dest), "r" (y));
			y += 8;
			dest += 8;
		}
		dest += _mga_priv.bespitch - _mga_priv.mga_vid_config.src_width;
	}

	dest = _mga_priv.vid_data +  _mga_priv.bespitch * _mga_priv.mga_vid_config.src_height + _mga_priv.bespitch * 8 * slice_num;
        
	for (h=0; h < 8; h++) {
		for(w=0; w < _mga_priv.mga_vid_config.src_width/16; w++) {
			asm volatile (
			"movq 		   (%1),%%mm0\n\t"
			"movq 		  %%mm0,%%mm1\n\t"
			"punpcklbw         (%2),%%mm0\n\t" 
			"punpckhbw         (%2),%%mm1\n\t" 
			"movq 		  %%mm0,(%0)\n\t"
			"movq 		  %%mm1,8(%0)\n\t"
			: "+r" (dest) : "r" (cb),"r" (cr));

			dest += 16;
			cb += 8;
			cr += 8;
		}
		dest += _mga_priv.bespitch - _mga_priv.mga_vid_config.src_width;
	}

	emms ();
	return 0;
}
#endif


#ifndef __i386__
/**
 *
 **/

static int _mga_write_slice_g400 (uint8_t *src[], uint32_t slice_num)
{
	uint8_t *dest;
	uint32_t h;
	uint8_t *y = src[0];
	uint8_t *cb = src[1];
	uint8_t *cr = src[2];

	dest = _mga_priv.vid_data + _mga_priv.bespitch * 16 * slice_num;

	for(h=0; h < 16; h++) {
		memcpy(dest, y, _mga_priv.mga_vid_config.src_width);
		y += _mga_priv.mga_vid_config.src_width;
		dest += _mga_priv.bespitch;
	}

	dest = _mga_priv.vid_data +  _mga_priv.bespitch * _mga_priv.mga_vid_config.src_height + _mga_priv.bespitch/2 * 8 * slice_num;

	for(h=0; h < 8; h++) {
		memcpy (dest, cb, _mga_priv.mga_vid_config.src_width/2);
		cb += _mga_priv.mga_vid_config.src_width/2;
		dest += _mga_priv.bespitch/2;
	}

	dest = _mga_priv.vid_data + _mga_priv.bespitch * _mga_priv.mga_vid_config.src_height + _mga_priv.bespitch * _mga_priv.mga_vid_config.src_height / 4 + _mga_priv.bespitch/2 * 8 * slice_num;

	for(h=0; h < 8; h++) {
		memcpy (dest, cr, _mga_priv.mga_vid_config.src_width/2);
		cr += _mga_priv.mga_vid_config.src_width/2;
		dest += _mga_priv.bespitch/2;
	}
	return 0;
}
#else


/**
 *
 **/

static int _mga_write_slice_g400_mmx (uint8_t *src[], uint32_t slice_num)
{
	uint8_t *dest;
	int h, w;
	int w_max;
	uint8_t *y = src[0];
	uint8_t *cb = src[1];
	uint8_t *cr = src[2];

	dest = _mga_priv.vid_data + _mga_priv.bespitch * 16 * slice_num;
	w_max = _mga_priv.mga_vid_config.src_width/8;

	for (h=0; h < 16; h++) {
		for (w=0; w < w_max; w++) {
			asm volatile (
				"movq       (%1),%%mm0\n\t"
				"movq       %%mm0,(%0)\n\t"
				: : "r" (dest), "r" (y));
			y += 8;
			dest += 8;
		}
		dest += _mga_priv.bespitch - _mga_priv.mga_vid_config.src_width;
	}

	dest = _mga_priv.vid_data +  _mga_priv.bespitch * _mga_priv.mga_vid_config.src_height + _mga_priv.bespitch/2 * 8 * slice_num;
	w_max = _mga_priv.mga_vid_config.src_width/16;

	for (h=0; h < 8; h++) {
		for (w=0; w < w_max; w++) {
			asm volatile (
				"movq       (%1),%%mm0\n\t"
				"movq       %%mm0,(%0)\n\t"
				: : "r" (dest), "r" (cb));
			cb += 8;
			dest += 8;
		}
		dest += (_mga_priv.bespitch - _mga_priv.mga_vid_config.src_width)/2;
	}

	dest = _mga_priv.vid_data + _mga_priv.bespitch * _mga_priv.mga_vid_config.src_height + _mga_priv.bespitch * _mga_priv.mga_vid_config.src_height / 4 + _mga_priv.bespitch/2 * 8 * slice_num;

	for (h=0; h < 8; h++) {
		for (w=0; w < w_max; w++) {
			asm volatile (
				"movq       (%1),%%mm0\n\t"
				"movq       %%mm0,(%0)\n\t"
				: : "r" (dest), "r" (cr));
			cr += 8;
			dest += 8;
		}
		dest += (_mga_priv.bespitch - _mga_priv.mga_vid_config.src_width)/2;
	}

	emms ();
	return 0;
}
#endif


/**
 *
 **/

static void _mga_flip_page (void)
{
	ioctl (_mga_priv.fd, MGA_VID_FSEL, &_mga_priv.next_frame);

	_mga_priv.next_frame = 2 - _mga_priv.next_frame; // switch between fields A1 and B1

	if (_mga_priv.next_frame) 
		_mga_priv.vid_data = _mga_priv.frame1;
	else
		_mga_priv.vid_data = _mga_priv.frame0;
}


/**
 *
 **/

static int _mga_open (plugin_t *plugin, void *name)
{
	_mga_priv.x_org = 10;
	_mga_priv.y_org = 10;

	if ((_mga_priv.fd = open ((char *) name, O_RDWR)) < 0) {
		LOG (LOG_ERROR, "Can't open %s\n", (char *) name); 
		return -1;
	}

	return 0;
}


/**
 *
 **/

static int _mga_close (plugin_t *plugin)
{
	close (_mga_priv.fd);
	_mga_priv.fd = -1;

	return 0;
}


/**
 *
 **/

static int _mga_setup (plugin_output_video_attr_t *attr)
{
	char *frame_mem;
	uint32_t frame_size;

	_mga_priv.mga_vid_config.src_width	= attr->width;
	_mga_priv.mga_vid_config.src_height	= attr->height;
	_mga_priv.mga_vid_config.dest_width	= attr->width;
	_mga_priv.mga_vid_config.dest_height	= attr->height;

	//_mga_priv.mga_vid_config.dest_width	= 1280;
	//_mga_priv.mga_vid_config.dest_height	= 1024;

	_mga_priv.mga_vid_config.x_org		= _mga_priv.x_org;
	_mga_priv.mga_vid_config.y_org		= _mga_priv.y_org;

	_mga_priv.mga_vid_config.colkey_on	= 1;

	if (ioctl (_mga_priv.fd, MGA_VID_CONFIG, &_mga_priv.mga_vid_config))
		perror ("Error in _mga_priv.mga_vid_config ioctl");

	ioctl (_mga_priv.fd, MGA_VID_ON, 0);

	_mga_priv.bespitch = (attr->width + 31) & ~31;
	frame_size	= _mga_priv.bespitch * attr->height * 3 / 2;
	frame_mem	= (char*)mmap (0, frame_size*2, PROT_WRITE, MAP_SHARED, _mga_priv.fd, 0);
	_mga_priv.frame0	= frame_mem;
	_mga_priv.frame1	= frame_mem + frame_size;
	_mga_priv.vid_data	= frame_mem;
	_mga_priv.next_frame	= 0;

	//clear the buffer
	memset (frame_mem, 0x80, frame_size*2);

	//set up drawing functions
	if (_mga_priv.mga_vid_config.card_type == MGA_G200) {
#ifdef __i386__
// FIXME: check if we REALLY do have MMX ops available
		video_mga.draw_frame = _mga_write_frame_g200_mmx;
		video_mga.draw_slice = _mga_write_slice_g200_mmx;
#else
		video_mga.draw_frame = _mga_write_frame_g200;
		video_mga.draw_slice = _mga_write_slice_g200;
#endif		
	} else {
#ifdef __i386__
		video_mga.draw_frame = _mga_write_frame_g400;
		video_mga.draw_slice = _mga_write_slice_g400_mmx;
#else
		video_mga.draw_frame = _mga_write_frame_g400;
		video_mga.draw_slice = _mga_write_slice_g400;
#endif		
	}

	return 0;
}


/**
 *
 **/

//FIXME this should allocate AGP memory via agpgart and then we
//can use AGP transfers to the framebuffer
static vo_image_buffer_t* allocate_image_buffer (uint32_t height, uint32_t width, uint32_t format)
{
	vo_image_buffer_t *image;

	if (!(image = malloc (sizeof (vo_image_buffer_t))))
		return NULL;

	image->height	= height;
	image->width	= width;
	image->format	= format;

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

static void free_image_buffer(vo_image_buffer_t* image)
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
		0,
		&video_mga);

	return &video_mga;
}


/**
 * Cleanup Plugin.
 **/

void plugin_exit (void)
{
}

