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

#include "config.h"
#include "video_out.h"
#include "video_out_internal.h"

LIBVO_EXTERN(mga)

#ifdef HAVE_MGA
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "drivers/mga_vid.h"

static mga_vid_config_t mga_vid_config;
static uint_8 *vid_data, *frame0, *frame1;
static int next_frame = 0;
static int f;

void
write_frame_g200(uint_8 *y,uint_8 *cr, uint_8 *cb)
{
	uint_8 *dest;
	uint_32 bespitch,h,w;

	dest = vid_data;
	bespitch = (mga_vid_config.src_width + 31) & ~31;

	for(h=0; h < mga_vid_config.src_height; h++)
	{
		memcpy(dest, y, mga_vid_config.src_width);
		y += mga_vid_config.src_width;
		dest += bespitch;
	}

	for(h=0; h < mga_vid_config.src_height/2; h++)
	{
		for(w=0; w < mga_vid_config.src_width/2; w++)
		{
			*dest++ = *cb++;
			*dest++ = *cr++;
		}
		dest += bespitch - mga_vid_config.src_width;
	}
}

void
write_frame_g400(uint_8 *y,uint_8 *cr, uint_8 *cb)
{
	uint_8 *dest;
	uint_32 bespitch,h;

	dest = vid_data;
	bespitch = (mga_vid_config.src_width + 31) & ~31;

	for(h=0; h < mga_vid_config.src_height; h++) 
	{
		memcpy(dest, y, mga_vid_config.src_width);
		y += mga_vid_config.src_width;
		dest += bespitch;
	}

	for(h=0; h < mga_vid_config.src_height/2; h++) 
	{
		memcpy(dest, cb, mga_vid_config.src_width/2);
		cb += mga_vid_config.src_width/2;
		dest += bespitch/2;
	}

	for(h=0; h < mga_vid_config.src_height/2; h++) 
	{
		memcpy(dest, cr, mga_vid_config.src_width/2);
		cr += mga_vid_config.src_width/2;
		dest += bespitch/2;
	}
}

static void
write_slice_g200(uint_8 *y,uint_8 *cr, uint_8 *cb,uint_32 slice_num)
{
	uint_8 *dest;
	uint_32 bespitch,h,w;

	bespitch = (mga_vid_config.src_width + 31) & ~31;
	dest = vid_data + bespitch * 16 * slice_num;

	for(h=0; h < 16; h++) 
	{
		memcpy(dest, y, mga_vid_config.src_width);
		y += mga_vid_config.src_width;
		dest += bespitch;
	}

	dest = vid_data +  bespitch * mga_vid_config.src_height + 
		bespitch * 8 * slice_num;

	for(h=0; h < 8; h++)
	{
		for(w=0; w < mga_vid_config.src_width/2; w++)
		{
			*dest++ = *cb++;
			*dest++ = *cr++;
		}
		dest += bespitch - mga_vid_config.src_width;
	}
}

static void
write_slice_g400(uint_8 *y,uint_8 *cr, uint_8 *cb,uint_32 slice_num)
{
	uint_8 *dest;
	uint_32 bespitch,h;

	bespitch = (mga_vid_config.src_width + 31) & ~31;
	dest = vid_data + bespitch * 16 * slice_num;

	for(h=0; h < 16; h++) 
	{
		memcpy(dest, y, mga_vid_config.src_width);
		y += mga_vid_config.src_width;
		dest += bespitch;
	}

	dest = vid_data +  bespitch * mga_vid_config.src_height + 
		bespitch/2 * 8 * slice_num;

	for(h=0; h < 8; h++) 
	{
		memcpy(dest, cb, mga_vid_config.src_width/2);
		cb += mga_vid_config.src_width/2;
		dest += bespitch/2;
	}

	dest = vid_data +  bespitch * mga_vid_config.src_height + 
		+ bespitch * mga_vid_config.src_height / 4 + bespitch/2 * 8 * slice_num;

	for(h=0; h < 8; h++) 
	{
		memcpy(dest, cr, mga_vid_config.src_width/2);
		cr += mga_vid_config.src_width/2;
		dest += bespitch/2;
	}
}

uint_32
draw_slice(uint_8 *src[], uint_32 slice_num)
{
	if (mga_vid_config.card_type == MGA_G200)
		write_slice_g200(src[0],src[2],src[1],slice_num);
	else
		write_slice_g400(src[0],src[2],src[1],slice_num);

	return 0;
}

void
flip_page(void)
{
	ioctl(f,MGA_VID_FSEL,&next_frame);

	next_frame = 2 - next_frame; // switch between fields A1 and B1

	if (next_frame) 
		vid_data = frame1;
	else
		vid_data = frame0;
}

uint_32
draw_frame(uint_8 *src[])
{
	if (mga_vid_config.card_type == MGA_G200)
		write_frame_g200(src[0], src[2], src[1]);
	else
		write_frame_g400(src[0], src[2], src[1]);

	flip_page();
	return 0;
}

uint_32
init(uint_32 width, uint_32 height, uint_32 fullscreen, char *title)
{
	char *frame_mem;
	uint_32 frame_size;

	f = open("/dev/mga_vid",O_RDWR);

	if(f == -1)
	{
		fprintf(stderr,"Couldn't open /dev/mga_vid\n"); 
		return(-1);
	}

	mga_vid_config.src_width = width;
	mga_vid_config.src_height= height;
	mga_vid_config.dest_width = width;
	mga_vid_config.dest_height= height;
	//mga_vid_config.dest_width = 1280;
	//mga_vid_config.dest_height= 1024;
	mga_vid_config.x_org= 16;
	mga_vid_config.y_org= 16;

	if (ioctl(f,MGA_VID_CONFIG,&mga_vid_config))
	{
		perror("Error in mga_vid_config ioctl");
	}
	ioctl(f,MGA_VID_ON,0);

	frame_size = ((width + 31) & ~31) * height + (((width + 31) & ~31) * height) / 2;
	frame_mem = (char*)mmap(0,frame_size*2,PROT_WRITE,MAP_SHARED,f,0);
	frame0 = frame_mem;
	frame1 = frame_mem + frame_size;
	vid_data = frame0;
	next_frame = 0;

	//clear the buffer
	memset(frame_mem,0x80,frame_size*2);

  return 0;
}


//FIXME this should allocate AGP memory via agpgart and then we
//can use AGP transfers to the framebuffer
static vo_image_buffer_t* 
allocate_image_buffer(uint_32 width, uint_32 height, uint_32 format)
{
	//use the generic fallback
	return allocate_image_buffer_common(width,height,format);
}

static void	
free_image_buffer(vo_image_buffer_t* image)
{
	//use the generic fallback
	free_image_buffer_common(image);
}

#else /* HAVE_MGA */

LIBVO_DUMMY_FUNCTIONS(mga);

#endif
