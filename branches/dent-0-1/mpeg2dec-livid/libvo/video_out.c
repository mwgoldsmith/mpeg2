/* 
 * video_out.c, 
 *
 * Copyright (C) Aaron Holtzman - June 2000
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

//
// Externally visible list of all vo drivers
//

extern vo_functions_t video_out_mga;
extern vo_functions_t video_out_x11;
extern vo_functions_t video_out_sdl;
extern vo_functions_t video_out_3dfx;

vo_functions_t* video_out_drivers[] = 
{
#ifdef HAVE_X11
	&video_out_x11,
#endif
#ifdef HAVE_MGA
	&video_out_mga,
#endif
#ifdef HAVE_3DFX
	&video_out_3dfx,
#endif
#ifdef HAVE_SDL
	&video_out_sdl,
#endif
	NULL
};


//
// Here are the generic fallback routines that could
// potentially be used by more than one display driver
//


//FIXME this should allocate AGP memory via agpgart and then we
//can use AGP transfers to the framebuffer
vo_image_buffer_t* 
allocate_image_buffer_common(uint_32 width, uint_32 height, uint_32 format)
{
	vo_image_buffer_t *image;
	uint_32 image_size;

	image = malloc(sizeof(vo_image_buffer_t));

	if(!image)
		return NULL;

	image->height = height;
	image->width = width;
	image->format = format;
	
	//we only know how to do 4:2:0 planar yuv right now.
	image_size = width * height * 3 / 2;
	image->base = malloc(image_size);

	if(!image->base)
	{
		free(image);
		return NULL;
	}

	return image;
}

void	
free_image_buffer_common(vo_image_buffer_t* image)
{
	free(image->base);
	free(image);
}
