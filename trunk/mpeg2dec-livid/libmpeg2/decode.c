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
#include "display.h"

#ifdef __i386__
#include "mmx.h"
#endif

//this is where we keep the state of the decoder
static picture_t picture;

//global config struct
mpeg2_config_t config;

//pointers to the display interface functions
static mpeg2_display_t mpeg2_display;

//the current start code chunk we are working on
uint_8 chunk_buffer[65536 + 4];
uint_8 *chunk_end = chunk_buffer;
uint_32 shift = 0;
uint_32 has_sync = 0;

static uint_32 is_display_initialized = 0;
static uint_32 is_sequence_needed = 1;

//FIXME obsolete need to clean up
//frame structure to pass back to caller
static mpeg2_frame_t mpeg2_frame;

void
mpeg2_init(mpeg2_display_t *foo)
{
	uint_32 frame_size;


	//FIXME this should go somewhere after we discover how big
	//the frame is, or size it so that it will be big enough for
	//all cases

	// We also make sure the frames are 16 byte aligned
	frame_size = 720 * 576;

	picture.throwaway_frame[0] = 
		(uint_8*)(((uint_32)malloc(frame_size  + frame_size/2 + 16) + 15) & ~0xf);
	picture.throwaway_frame[1] = picture.throwaway_frame[0] + frame_size;
	picture.throwaway_frame[2] = picture.throwaway_frame[1] + frame_size/4;

	picture.backward_reference_frame[0] = 
		(uint_8*)(((uint_32)malloc(3 * frame_size + 16) + 15) & ~0xf);
	picture.backward_reference_frame[1] = picture.backward_reference_frame[0] + frame_size;
	picture.backward_reference_frame[2] = picture.backward_reference_frame[1] + frame_size/4;
	picture.forward_reference_frame[0] = picture.backward_reference_frame[2] + frame_size/4;
	picture.forward_reference_frame[1] = picture.forward_reference_frame[0] + frame_size;
	picture.forward_reference_frame[2] = picture.forward_reference_frame[1] + frame_size/4;

	//FIXME setup config properly
	config.flags = MPEG2_MMX_ENABLE;
	//config.flags = 0;

	//copy the display interface function pointers
	mpeg2_display = *foo;

	//intialize the decoder state 
	header_state_init(&picture);
	slice_init();
	idct_init();
	motion_comp_init();
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
	uint_8 *tmp[3];

	if(picture.picture_coding_type != B_TYPE)
	{
		//reuse the soon to be outdated forward reference frame
		picture.current_frame[0] = picture.forward_reference_frame[0];
		picture.current_frame[1] = picture.forward_reference_frame[1];
		picture.current_frame[2] = picture.forward_reference_frame[2];

		//make the backward reference frame the new forward reference frame
		tmp[0] = picture.forward_reference_frame[0];
		tmp[1] = picture.forward_reference_frame[1];
		tmp[2] = picture.forward_reference_frame[2];
		picture.forward_reference_frame[0] = picture.backward_reference_frame[0];
		picture.forward_reference_frame[1] = picture.backward_reference_frame[1];
		picture.forward_reference_frame[2] = picture.backward_reference_frame[2];
		picture.backward_reference_frame[0] = tmp[0];
		picture.backward_reference_frame[1] = tmp[1];
		picture.backward_reference_frame[2] = tmp[2];
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
		}
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
			is_frame_done = slice_process(&picture,chunk_buffer);

		//FIXME blah
#ifdef __i386__
		emms();
#endif

		if(is_frame_done)
		{
			//decide which frame to send to the display
			if(picture.picture_coding_type == B_TYPE)
			{
				mpeg2_frame.frame[0] = picture.throwaway_frame[0];
				mpeg2_frame.frame[1] = picture.throwaway_frame[1];
				mpeg2_frame.frame[2] = picture.throwaway_frame[2];
			}
			else
			{
				mpeg2_frame.frame[0] = picture.forward_reference_frame[0];
				mpeg2_frame.frame[1] = picture.forward_reference_frame[1];
				mpeg2_frame.frame[2] = picture.forward_reference_frame[2];
			}

			mpeg2_frame.width = picture.coded_picture_width;
			mpeg2_frame.height = picture.coded_picture_height;

			if(bitstream_show(32) == SEQUENCE_END_CODE)
				is_sequence_needed = 1;

			is_frame_done = 0;

			//FIXME
			//we can't initialize the display until we know how big the picture is
			if(!is_display_initialized)
			{
				mpeg2_display.init(mpeg2_frame.width,mpeg2_frame.height);
				is_display_initialized = 1;
			}
			mpeg2_display.draw_frame(mpeg2_frame.frame);
			ret++;
		}
	}

	return ret;
}


