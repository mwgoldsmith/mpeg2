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

#include "video_out.h"

#include "hw_bes.h"
#include "mmx.h"

static int _mga_close		(void *plugin);
static int _mga_setup		(vo_output_video_attr_t *attr);
static int _mga_draw_frame     (uint8_t *src[]);
static int _mga_draw_slice     (uint8_t *src[], uint32_t slice_num);
static void _mga_flip_page	(void);
static void _mga_free_image_buffer	(frame_t* image);
static frame_t *_mga_allocate_image_buffer (uint32_t height, uint32_t width, uint32_t format);

static struct mga_priv_s {
	int fd;
	mga_vid_config_t mga_vid_config;
	uint8_t *vid_data, *frame0, *frame1;
	int next_frame;
	uint32_t bespitch;
	uint32_t fullscreen;
	uint32_t x_org;
	uint32_t y_org;
} _mga_priv = {-1};


#ifndef __OMS__
static vo_info_t _mga_vo_info = 
{
    "Matrox Millennium G200/G400 (/dev/mgavid)",
    "mga",
    "Aaron Holtzman <aholtzma@ess.engr.uvic.ca>",
    ""
};
#endif __OMS__

LIBVO_EXTERN(_mga,"mga")

static int _mga_draw_frame     (uint8_t *src[]) {
  return 0;
}


static int _mga_draw_slice     (uint8_t *src[], uint32_t slice_num) {
  return 0;
}


#ifndef __i386__
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
//	_mga_flip_page();
	return 0;
}
#else

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
//	_mga_flip_page();

	emms ();

	return 0;
}
#endif

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
//	_mga_flip_page();
	return 0;
}

#ifndef __i386__
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
			movq_m2r (*y, mm0);
			movq_r2m (mm0, *dest);

			y += 8;
			dest += 8;
		}
		dest += _mga_priv.bespitch - _mga_priv.mga_vid_config.src_width;
	}

	dest = _mga_priv.vid_data +  _mga_priv.bespitch * _mga_priv.mga_vid_config.src_height + _mga_priv.bespitch * 8 * slice_num;
        
	for (h=0; h < 8; h++) {
		for(w=0; w < _mga_priv.mga_vid_config.src_width/16; w++) {
			movq_m2r (*cb, mm1);
			movq_r2r (mm1, mm2);
			movq_m2r (*cr, mm3);
			movq_r2r (mm3, mm4);

			punpcklbw_r2r (mm3, mm1);
			movq_r2m (mm1,*dest);
			punpckhbw_r2r (mm4,mm2);
			movq_r2m (mm2,*(dest+8));

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
			movq_m2r (*y, mm0);
			movq_r2m (mm0, *dest);
			y += 8;
			dest += 8;
		}
		dest += _mga_priv.bespitch - _mga_priv.mga_vid_config.src_width;
	}

	dest = _mga_priv.vid_data +  _mga_priv.bespitch * _mga_priv.mga_vid_config.src_height + _mga_priv.bespitch * 4 * slice_num;
	w_max = _mga_priv.mga_vid_config.src_width/16;

	for (h=0; h < 8; h++) {
		for (w=0; w < w_max; w++) {
			movq_m2r (*cb, mm0);
			movq_r2m (mm0, *dest);
			cb += 8;
			dest += 8;
		}
		dest += (_mga_priv.bespitch - _mga_priv.mga_vid_config.src_width)/2;
	}

	dest = _mga_priv.vid_data + _mga_priv.bespitch * _mga_priv.mga_vid_config.src_height + _mga_priv.bespitch * _mga_priv.mga_vid_config.src_height / 4 + _mga_priv.bespitch * 4 * slice_num;

	for (h=0; h < 8; h++) {
		for (w=0; w < w_max; w++) {
			movq_m2r (*cr, mm0);
			movq_r2m (mm0, *dest);
			cr += 8;
			dest += 8;
		}
		dest += (_mga_priv.bespitch - _mga_priv.mga_vid_config.src_width)/2;
	}

	emms ();
	return 0;
}
#endif


static void _mga_flip_page (void)
{
	ioctl (_mga_priv.fd, MGA_VID_FSEL, &_mga_priv.next_frame);

	_mga_priv.next_frame = 2 - _mga_priv.next_frame; // switch between fields A1 and B1

	if (_mga_priv.next_frame) 
		_mga_priv.vid_data = _mga_priv.frame1;
	else
		_mga_priv.vid_data = _mga_priv.frame0;

	emms ();
}


static int _mga_close (void *plugin)
{
	close (_mga_priv.fd);
	_mga_priv.fd = -1;

	return 0;
}


static int _mga_setup (vo_output_video_attr_t *attr)
{
    char *frame_mem;
    uint32_t frame_size;

    if (_mga_priv.fd < 0) {
	int fd;

	if ((fd = open ("/dev/mga_vid", O_RDWR)) < 0) {
		LOG (LOG_DEBUG, "Can't open /dev_mga_vid"); 
		return -1;
	}

	if (ioctl (fd, MGA_VID_ON, 0)) {
		LOG (LOG_DEBUG, "Can't ioctl /dev_mga_vid"); 
		close (fd);
		return -1;
	}

	_mga_priv.fd = fd;
	_mga_priv.x_org = 10;
	_mga_priv.y_org = 10;
    }


	_mga_priv.mga_vid_config.src_width	= attr->width;
	_mga_priv.mga_vid_config.src_height	= attr->height;

#if 0
	_mga_priv.mga_vid_config.dest_width	= attr->width;
	_mga_priv.mga_vid_config.dest_height	= attr->height;
#else
	_mga_priv.mga_vid_config.dest_width	= 1280;
	_mga_priv.mga_vid_config.dest_height	= 1024;
#endif

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
		video_out_mga.draw_frame = _mga_write_frame_g200_mmx;
		video_out_mga.draw_slice = _mga_write_slice_g200_mmx;
#else
		video_out_mga.draw_frame = _mga_write_frame_g200;
		video_out_mga.draw_slice = _mga_write_slice_g200;
#endif		
	} else {
#ifdef __i386__
		video_out_mga.draw_frame = _mga_write_frame_g400;
		video_out_mga.draw_slice = _mga_write_slice_g400_mmx;
#else
		video_out_mga.draw_frame = _mga_write_frame_g400;
		video_out_mga.draw_slice = _mga_write_slice_g400;
#endif		
	}

	return 0;
}


//FIXME this should allocate AGP memory via agpgart and then we
//can use AGP transfers to the framebuffer
static frame_t* _mga_allocate_image_buffer (int width, int height, uint32_t format)
{
    return libvo_common_alloc (width, height);

}


static void _mga_free_image_buffer(frame_t* image)
{
    libvo_common_free (image);
}

#endif
