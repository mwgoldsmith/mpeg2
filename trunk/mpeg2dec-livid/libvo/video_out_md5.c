/*
 * video_out_md5.c
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
#include <inttypes.h>

#include "video_out.h"
#include "video_out_internal.h"

LIBVO_EXTERN (md5)

#include "video_out_porting_old.h"

static vo_info_t vo_info = 
{
	"MD5",
	"md5",
	"walken",
	""
};

extern vo_functions_t video_out_pgm;

static FILE * md5_file;
static int framenum = -2;


static uint32_t
setup(vo_output_video_attr_t * attr, void* user_data)
{    
     md5_file = fopen ("md5", "w");
     
     return video_out_pgm.setup (attr,user_data);
}

static const vo_info_t*
get_info2(void* user_data)
{
    return &vo_info;
}

static void flip_page2 (void* user_data)
{
}

static uint32_t draw_slice2(uint8_t * src[], int slice_num, void* user_data)
{
    return 0;
}

extern uint32_t output_pgm_frame (char * fname, uint8_t * src[]);

static uint32_t draw_frame2(uint8_t * src[], void* user_data)
{
    char buf[100];
    char buf2[100];
    FILE * f;
    int i;

    if (++framenum < 0)
	return 0;

    sprintf (buf, "%d.pgm", framenum);
    output_pgm_frame (buf, src);

    sprintf (buf2, "md5sum %s", buf);
    f = popen (buf2, "r");
    i = fread (buf2, 1, sizeof(buf2), f);
    pclose (f);
    fwrite (buf2, 1, i, md5_file);

    remove (buf);

    return 0;
}

static vo_image_buffer_t* 
allocate_image_buffer2(uint32_t height, uint32_t width, 
		       uint32_t format, void* user_data)
{
    return video_out_pgm.allocate_image_buffer ();
}

static void	
free_image_buffer2(vo_image_buffer_t* image, void* user_data)
{
    return video_out_pgm.free_image_buffer (image);
}
