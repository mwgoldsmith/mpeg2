/*
 *  codec.c
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
#include <oms/oms.h>
#include <oms/plugin/output_video.h>

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
static picture_t picture;

//global config struct
mpeg2_config_t config;

//pointers to the display interface functions
plugin_output_video_t *video_out;

//
//the current start code chunk we are working on
//we max out at 65536 bytes as that is the largest
//slice we will see in MP@ML streams.
//(we make no pretenses ofdecoding anything more than that)
static u_int8_t chunk_buffer[65536 + 4];
static u_int8_t *chunk_end = chunk_buffer;
static u_int32_t shift = 0;
static u_int32_t has_sync = 0;
static u_int8_t is_display_initialized = 0;
static u_int8_t is_sequence_needed = 1;

static int _mpeg2dec_open	(void *foo);
static int _mpeg2dec_close	(plugin_t *plugin);
static int _mpeg2dec_read	(plugin_codec_video_t *plugin, buf_t *buf, buf_entry_t *buf_entry);

static plugin_codec_video_t codec_mpeg2dec = {
        NULL,			// priv
        _mpeg2dec_open,		// open
        _mpeg2dec_close,	// close
        _mpeg2dec_read,		// read
        NULL,			// display (will be inserted automagically)
	NULL,
	NULL
};


/************************************************/


/**
 *
 **/

static int _mpeg2dec_open (void *foo)
{
	chunk_end	= chunk_buffer;
	shift		= 0;
	has_sync	= 0;

	is_display_initialized = 0;
	is_sequence_needed = 1;

	//copy the display interface function pointers
	video_out = (plugin_output_video_t *) foo;

	//FIXME setup config properly
	config.flags = MPEG2_MMX_ENABLE;
	//config.flags = 0;

	//intialize the decoder state 
	header_state_init (&picture);
	slice_init ();
	idct_init ();
	motion_comp_init ();

	return 0;
}


/**
 *
 **/

int _mpeg2dec_close (plugin_t *plugin)
{
	return 0;
}


/**
 *
 **/

static void decode_allocate_image_buffers (picture_t *pic)
{
	u_int32_t frame_size;
	u_int32_t slice_size;
	vo_image_buffer_t *img_buf;

	frame_size = pic->coded_picture_width * pic->coded_picture_height;
	slice_size = pic->coded_picture_width * 16;

	//FIXME the next step is to give a vo_image_buffer_t* to dislay_slice (or eqiv)
	img_buf = video_out->allocate_image_buffer(16,pic->coded_picture_width,0);
	pic->throwaway_frame[0] = img_buf->base;
	pic->throwaway_frame[1] = pic->throwaway_frame[0] + slice_size;
	pic->throwaway_frame[2] = pic->throwaway_frame[1] + slice_size/4;

	img_buf = video_out->allocate_image_buffer(pic->coded_picture_height,pic->coded_picture_width,0);
	pic->backward_reference_frame[0] = img_buf->base;
	pic->backward_reference_frame[1] = pic->backward_reference_frame[0] + frame_size;
	pic->backward_reference_frame[2] = pic->backward_reference_frame[1] + frame_size/4;

	img_buf = video_out->allocate_image_buffer(pic->coded_picture_height,pic->coded_picture_width,0);
	pic->forward_reference_frame[0] = img_buf->base;
	pic->forward_reference_frame[1] = pic->forward_reference_frame[0] + frame_size;
	pic->forward_reference_frame[2] = pic->forward_reference_frame[1] + frame_size/4;
}


/**
 * Buffer up to one the next start code. We process data one
 * "start code chunk" at a time
 **/

static u_int32_t decode_buffer_chunk (u_int8_t **start, u_int8_t *end)
{
	static u_int8_t *cur;

	cur = *start;

// Find an initial start code if we need to
	if(!has_sync) {
		while ((shift & 0xffffff) != 0x000001) {
			shift <<= 8;
			shift |= *cur++;

			if (cur >= end)
				goto done;
		}
		has_sync = 1;

		chunk_end = chunk_buffer;
		shift = 0;
	}

// Find next start code
	do {
		if (cur >= end)
			goto done;

		shift <<= 8;
		shift |= *cur;

		*chunk_end++ = *cur++;
	} while ((shift & 0xffffff) != 0x000001);
	 
	chunk_end = chunk_buffer;
	*start = cur;
	return 1;

done:
	*start = cur;
	return 0;
}


/**
 *
 **/

void decode_reorder_frames (picture_t *pic)
{
	if(pic->picture_coding_type != B_TYPE) {
		//reuse the soon to be outdated forward reference frame
		pic->current_frame[0] = pic->forward_reference_frame[0];
		pic->current_frame[1] = pic->forward_reference_frame[1];
		pic->current_frame[2] = pic->forward_reference_frame[2];

		//make the backward reference frame the new forward reference frame
		pic->forward_reference_frame[0] = pic->backward_reference_frame[0];
		pic->forward_reference_frame[1] = pic->backward_reference_frame[1];
		pic->forward_reference_frame[2] = pic->backward_reference_frame[2];

		pic->backward_reference_frame[0] = pic->current_frame[0];
		pic->backward_reference_frame[1] = pic->current_frame[1];
		pic->backward_reference_frame[2] = pic->current_frame[2];
	} else {
		pic->current_frame[0] = pic->throwaway_frame[0];
		pic->current_frame[1] = pic->throwaway_frame[1];
		pic->current_frame[2] = pic->throwaway_frame[2];
	}
}


/**
 *
 **/

static int _mpeg2dec_read (plugin_codec_video_t *plugin, buf_t *buf, buf_entry_t *buf_entry)
{
	uint_8 *data_start = buf_entry->data;
	uint_8 *data_end = data_start+buf_entry->data_len;
	u_int32_t is_frame_done = 0;
	u_int32_t ret = 0;

	while ((is_frame_done <= 0) && decode_buffer_chunk (&data_start, data_end)) {
		u_int32_t code = chunk_buffer[0];

		if (is_sequence_needed && code != SEQUENCE_HEADER_CODE)
			continue;

		bitstream_init (chunk_buffer);
		bitstream_get (8);

//deal with the header otherwise
		switch (code) {
		case SEQUENCE_HEADER_CODE:
			header_process_sequence_header (&picture); 
			is_sequence_needed = 0;

			if(!is_display_initialized) {
				video_out->setup (picture.coded_picture_width, picture.coded_picture_height, 0, 0);
				decode_allocate_image_buffers (&picture);
				is_display_initialized = 1;
			}
			break;
		case SEQUENCE_END_CODE:
			is_sequence_needed = 1;
			break;
		case PICTURE_START_CODE:
			header_process_picture_header (&picture);
			decode_reorder_frames (&picture);
			break;
		case EXTENSION_START_CODE:
			header_process_extension (&picture);
			break;
		case GROUP_START_CODE:
			header_process_gop_header (&picture);
			break;
		case USER_DATA_START_CODE:
			header_process_user_data ();
			break;
		default:	
			if (code >= SLICE_START_CODE_MIN && code <= SLICE_START_CODE_MAX) {
				is_frame_done = slice_process (&picture,chunk_buffer);
#if 0
				if ((is_frame_done = slice_process (&picture,chunk_buffer)) < 0) {
					has_sync = 0;	// resync
					continue;
				}
#endif

				if(picture.picture_coding_type == B_TYPE) {
					video_out->draw_slice (picture.throwaway_frame,chunk_buffer[0] - 1);

					picture.current_frame[0] = picture.throwaway_frame[0] - 
						(chunk_buffer[0]) * 16 * picture.coded_picture_width;
					picture.current_frame[1] = picture.throwaway_frame[1] - 
						(chunk_buffer[0]) * 8 * picture.coded_picture_width/2;
					picture.current_frame[2] = picture.throwaway_frame[2] - 
						(chunk_buffer[0]) * 8 * picture.coded_picture_width/2;
				} else {
					u_int8_t *foo[3];

					foo[0] = picture.forward_reference_frame[0] + (chunk_buffer[0]-1) * 16 *
						picture.coded_picture_width;
					foo[1] = picture.forward_reference_frame[1] + (chunk_buffer[0]-1) * 8 *
						picture.coded_picture_width/2;
					foo[2] = picture.forward_reference_frame[2] + (chunk_buffer[0]-1) * 8 *
						picture.coded_picture_width/2;
					video_out->draw_slice (foo, chunk_buffer[0]-1);
//					video_out->draw_frame (picture.forward_reference_frame);
				}
			}
		}

		//FIXME blah
#ifdef __i386__
		emms ();
#endif

		if (is_frame_done > 0) {
			video_out->flip_page ();

			is_frame_done = 0;

			ret++;
		}
	}

	return ret;
}

/*****************************************/


/**
 * Initialize Plugin.
 **/


void *plugin_init (char *whoami)
{
	pluginRegister (whoami,
		PLUGIN_ID_CODEC_VIDEO,
		"mpeg2",
		&codec_mpeg2dec);

	return &codec_mpeg2dec;
}


/**
 * Cleanup Plugin.
 **/

void plugin_exit (void)
{
}
