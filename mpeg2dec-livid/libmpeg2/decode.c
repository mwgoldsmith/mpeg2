/*
 * decode.c
 *
 * Copyright (C) Aaron Holtzman <aholtzma@ess.engr.uvic.ca> - Nov 1999
 *
 * Decodes an MPEG-2 video stream.
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 *	
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Make; see the file COPYING. If not, write to
 * the Free Software Foundation, 
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#ifdef __OMS__
#include <oms/oms.h>
#include <oms/plugin/codec_video.h>

#define vo_functions_t	plugin_codec_video_t
#endif
 
#include "config.h"
#include "mpeg2.h"
#include "mpeg2_internal.h"

#include "motion_comp.h"
#include "idct.h"
#include "header.h"
#include "slice.h"
#include "stats.h"

#ifdef __i386__
#include "mmx.h"
#endif

//this is where we keep the state of the decoder
picture_t picture;

//global config struct
mpeg2_config_t config;

//
//the current start code chunk we are working on
//we max out at 65536 bytes as that is the largest
//slice we will see in MP@ML streams.
// (we make no pretenses ofdecoding anything more than that)
static uint8_t chunk_buffer[65536 + 4];
static uint32_t shift = 0;

static uint32_t is_display_initialized = 0;
static uint32_t is_sequence_needed = 1;


void mpeg2_init (void)
{
    //FIXME setup config properly
    config.flags = MPEG2_MMX_ENABLE | MPEG2_MLIB_ENABLE;
    //config.flags = 0;

    //intialize the decoder state 
    shift = 0;
    is_sequence_needed = 1;

    header_state_init (&picture);
    idct_init ();
    motion_comp_init ();
}


static void decode_allocate_image_buffers (vo_functions_t *video_out, picture_t * picture)
{
    int frame_size;
    int slice_size;
    vo_image_buffer_t * tmp;

    frame_size = picture->coded_picture_width * picture->coded_picture_height;
    slice_size = picture->coded_picture_width * 16;

    //FIXME the next step is to give a vo_image_buffer_t* to dislay_slice (or eqiv)
    tmp = video_out->allocate_image_buffer (16,picture->coded_picture_width,0);
    picture->throwaway_frame[0] = tmp->base;
    picture->throwaway_frame[1] = picture->throwaway_frame[0] + slice_size;
    picture->throwaway_frame[2] = picture->throwaway_frame[1] + slice_size/4;

    tmp = video_out->allocate_image_buffer (picture->coded_picture_height,picture->coded_picture_width,0);
    picture->backward_reference_frame[0] = tmp->base;
    picture->backward_reference_frame[1] = picture->backward_reference_frame[0] + frame_size;
    picture->backward_reference_frame[2] = picture->backward_reference_frame[1] + frame_size/4;

    tmp = video_out->allocate_image_buffer (picture->coded_picture_height,picture->coded_picture_width,0);
    picture->forward_reference_frame[0] = tmp->base;
    picture->forward_reference_frame[1] = picture->forward_reference_frame[0] + frame_size;
    picture->forward_reference_frame[2] = picture->forward_reference_frame[1] + frame_size/4;
}


static void decode_reorder_frames (void)
{
    if (picture.picture_coding_type != B_TYPE) {

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

    } else {

	picture.current_frame[0] = picture.throwaway_frame[0];
	picture.current_frame[1] = picture.throwaway_frame[1];
	picture.current_frame[2] = picture.throwaway_frame[2];

    }
}


static int parse_chunk (vo_functions_t *video_out, int code, uint8_t *buffer)
{
    int is_frame_done = 0;

    if (is_sequence_needed && code != 0xb3)	/* b3 = sequence_header_code */
	return 0;

    stats_header (code, buffer);

    switch (code) {
    case 0x00:	/* picture_start_code */
	header_process_picture_header (&picture, buffer);
	decode_reorder_frames ();
	break;

    case 0xb3:	/* sequence_header_code */
	header_process_sequence_header (&picture, buffer);
	is_sequence_needed = 0;

	if (!is_display_initialized) {
	    video_out->init (picture.coded_picture_width,picture.coded_picture_height,0,0);
	    decode_allocate_image_buffers (video_out, &picture);
	    is_display_initialized = 1;
	}
	break;

    case 0xb5:	/* extension_start_code */
	header_process_extension (&picture, buffer);
	break;

    default:
	if ((code < 0xb0) && code) {
	    is_frame_done = slice_process (&picture, code, buffer);

	    if (picture.picture_coding_type == B_TYPE) {
		video_out->draw_slice (picture.throwaway_frame, code - 1);

		picture.current_frame[0] = picture.throwaway_frame[0] -
		    code * 16 * picture.coded_picture_width;
		picture.current_frame[1] = picture.throwaway_frame[1] -
		    code * 8 * picture.coded_picture_width/2;
		picture.current_frame[2] = picture.throwaway_frame[2] -
		    code * 8 * picture.coded_picture_width/2;
	    } else {
		uint8_t *foo[3];

		foo[0] = picture.forward_reference_frame[0] + (code-1) * 16 *
		    picture.coded_picture_width;
		foo[1] = picture.forward_reference_frame[1] + (code-1) * 8 *
		    picture.coded_picture_width/2;
		foo[2] = picture.forward_reference_frame[2] + (code-1) * 8 *
		    picture.coded_picture_width/2;

		video_out->draw_slice (foo, code-1);
		//video_out->draw_frame (picture.forward_reference_frame);
	    }

	    if (is_frame_done)
		video_out->flip_page ();
	}
    }

    return is_frame_done;
}


int mpeg2_decode_data (vo_functions_t *video_out, uint8_t * current, uint8_t * end) 
{
    static uint8_t code = 0xff;
    //static uint8_t chunk_buffer[65536];
    static uint8_t *chunk_ptr = chunk_buffer;
    //static uint32_t shift = 0;

    uint8_t byte;
    int ret = 0;

    while (current != end) {
	while (1) {
	    byte = *current++;
	    if (shift == 0x00000100)
		break;
	    *chunk_ptr++ = byte;
	    shift = (shift | byte) << 8;

	    if (current == end)
		return ret;
	}

	/* found start_code following chunk */

	ret += parse_chunk (video_out, code, chunk_buffer);

	/* done with header or slice, prepare for next one */

	code = byte;
	chunk_ptr = chunk_buffer;
	shift = 0xffffff00;
    }

    return ret;
}
