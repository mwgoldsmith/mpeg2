/*
 *  decode.c
 *
 *  Copyright (C) Aaron Holtzman <aholtzma@ess.engr.uvic.ca> - Nov 1999
 *
 *  Decodes an MPEG-2 video stream.
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
 *  the Free Software Foundation, 
 *
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "config.h"
#include "mpeg2.h"
#include "mpeg2_internal.h"

#include "motion_comp.h"
#include "bitstream.h"
#include "idct.h"
#include "header.h"
#include "slice.h"

#ifdef __i386__
#include "mmx.h"
#endif

//this is where we keep the state of the decoder
picture_t picture;

//global config struct
mpeg2_config_t config;

//pointers to the display interface functions
vo_functions_t video_out;

//
//the current start code chunk we are working on
//we max out at 65536 bytes as that is the largest
//slice we will see in MP@ML streams.
//(we make no pretenses ofdecoding anything more than that)
static uint_8 chunk_buffer[65536 + 4];
static uint_8 *chunk_end = chunk_buffer;
static uint_32 shift = 0;
static uint_32 has_sync = 0;

static uint_32 is_display_initialized = 0;
static uint_32 is_sequence_needed = 1;

void
mpeg2_init(vo_functions_t *foo)
{
	//copy the display interface function pointers
	video_out = *foo;

	//FIXME setup config properly
	config.flags = MPEG2_MMX_ENABLE | MPEG2_MLIB_ENABLE;
	//config.flags = 0;

	//intialize the decoder state 
	header_state_init(&picture);
	slice_init();
	idct_init();
	motion_comp_init();
}

static void 
decode_allocate_image_buffers(picture_t *picture)
{
	uint_32 frame_size;
	uint_32 slice_size;
	vo_image_buffer_t *tmp;

	frame_size = picture->coded_picture_width * picture->coded_picture_height;
	slice_size = picture->coded_picture_width * 16;

	//FIXME the next step is to give a vo_image_buffer_t* to dislay_slice (or eqiv)
	tmp = video_out.allocate_image_buffer(16,picture->coded_picture_width,0);
	picture->throwaway_frame[0] = tmp->base;
	picture->throwaway_frame[1] = picture->throwaway_frame[0] + slice_size;
	picture->throwaway_frame[2] = picture->throwaway_frame[1] + slice_size/4;

	tmp = video_out.allocate_image_buffer(picture->coded_picture_height,picture->coded_picture_width,0);
	picture->backward_reference_frame[0] = tmp->base;
	picture->backward_reference_frame[1] = picture->backward_reference_frame[0] + frame_size;
	picture->backward_reference_frame[2] = picture->backward_reference_frame[1] + frame_size/4;

	tmp = video_out.allocate_image_buffer(picture->coded_picture_height,picture->coded_picture_width,0);
	picture->forward_reference_frame[0] = tmp->base;
	picture->forward_reference_frame[1] = picture->forward_reference_frame[0] + frame_size;
	picture->forward_reference_frame[2] = picture->forward_reference_frame[1] + frame_size/4;
}

//
// Buffer up to one the next start code. We process data one
// "start code chunk" at a time
//
uint_32
decode_buffer_chunk(uint_8 **start,uint_8 *end)
{
	uint_8 *cur;
	uint_32 ret = 0;

	cur = *start;

	//Find an initial start code if we need to
	if(!has_sync)
	{
		while((shift & 0xffffff) != 0x000001)
		{
			shift = (shift << 8) + *cur++;

			if(cur >= end)
				goto done;
		}
		has_sync = 1;

		chunk_end = chunk_buffer;	
		shift = 0;
	}

	do
	{
		if(cur >= end)
			goto done;

		*chunk_end++ = *cur;
		shift = (shift << 8) + *cur++;
	}
	while((shift & 0xffffff) != 0x000001);
	 
	//FIXME remove
	//printf("done chunk %d bytes\n",chunk_end - &chunk_buffer[0]);
	chunk_end = chunk_buffer;	
	ret = 1;

done:
	*start = cur;

	return ret;
}

void
decode_reorder_frames(void)
{
	if(picture.picture_coding_type != B_TYPE)
	{
		//reuse the soon to be outdated forward reference frame
		picture.current_frame[0] = picture.forward_reference_frame[0];
		picture.current_frame[1] = picture.forward_reference_frame[1];
		picture.current_frame[2] = picture.forward_reference_frame[2];

		//make the backward reference frame the new forward reference frame
		picture.forward_reference_frame[0] = picture.backward_reference_frame[0];
		picture.forward_reference_frame[1] = picture.backward_reference_frame[1];
		picture.forward_reference_frame[2] = picture.backward_reference_frame[2];
		picture.backward_reference_frame[0] = picture.current_frame[0];
		picture.backward_reference_frame[1] = picture.current_frame[1];
		picture.backward_reference_frame[2] = picture.current_frame[2];
	}
	else
	{
		picture.current_frame[0] = picture.throwaway_frame[0];
		picture.current_frame[1] = picture.throwaway_frame[1];
		picture.current_frame[2] = picture.throwaway_frame[2];
	}
}



uint_32
mpeg2_decode_data(uint_8 *data_start,uint_8 *data_end) 
{
	uint_32 code;
	uint_32 ret = 0;
	uint_32 is_frame_done = 0;

	while(!(is_frame_done) && decode_buffer_chunk(&data_start,data_end))
	{
		code = chunk_buffer[0];

		if(is_sequence_needed && code != SEQUENCE_HEADER_CODE)
			continue;

		bitstream_init(chunk_buffer);
		bitstream_get(8);

		//deal with the header otherwise
		if (code == SEQUENCE_HEADER_CODE)
		{
			header_process_sequence_header(&picture); 
			is_sequence_needed = 0;

			if(!is_display_initialized)
			{
				video_out.init(picture.coded_picture_width,picture.coded_picture_height,0,0);
				decode_allocate_image_buffers(&picture);
				is_display_initialized = 1;
			}
		}
		else if(code == SEQUENCE_END_CODE)
			is_sequence_needed = 1;
		else if (code == PICTURE_START_CODE)
		{
			header_process_picture_header(&picture);
			decode_reorder_frames();
		}
		else if (code == EXTENSION_START_CODE)
			header_process_extension(&picture);
		else if (code == GROUP_START_CODE)
			header_process_gop_header(&picture);
		else if (code == USER_DATA_START_CODE)
			header_process_user_data();
		else if (code >= SLICE_START_CODE_MIN && code <= SLICE_START_CODE_MAX)
		{
			is_frame_done = slice_process(&picture,chunk_buffer); 

			if(picture.picture_coding_type == B_TYPE)
			{
				video_out.draw_slice(picture.throwaway_frame,chunk_buffer[0] - 1);

				picture.current_frame[0] = picture.throwaway_frame[0] - 
					(chunk_buffer[0]) * 16 * picture.coded_picture_width;
				picture.current_frame[1] = picture.throwaway_frame[1] - 
					(chunk_buffer[0]) * 8 * picture.coded_picture_width/2;
				picture.current_frame[2] = picture.throwaway_frame[2] - 
					(chunk_buffer[0]) * 8 * picture.coded_picture_width/2;
			}
			else
			{
				uint_8 *foo[3];

				foo[0] = picture.forward_reference_frame[0] + (chunk_buffer[0]-1) * 16 *
					picture.coded_picture_width;
				foo[1] = picture.forward_reference_frame[1] + (chunk_buffer[0]-1) * 8 *
					picture.coded_picture_width/2;
				foo[2] = picture.forward_reference_frame[2] + (chunk_buffer[0]-1) * 8 *
					picture.coded_picture_width/2;
				video_out.draw_slice(foo,chunk_buffer[0]-1);
				//video_out.draw_frame(picture.forward_reference_frame);
			}
		}

		//FIXME blah
#ifdef __i386__
		emms();
#endif

		if(is_frame_done)
		{
			video_out.flip_page();

			is_frame_done = 0;

			ret++;
		}
	}

	return ret;
}
