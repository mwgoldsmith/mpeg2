/*
 * decode.c
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
#include <stdlib.h>
#include <inttypes.h>

#ifdef __OMS__
#include <oms/oms.h>
#include <oms/plugin/output_video.h>
#endif

#ifndef __OMS__
#include "video_out.h"
#endif

#include "mpeg2.h"
#include "mpeg2_internal.h"
#include "mm_accel.h"
#include "attributes.h"
#include "mmx.h"

//
// This (temporary?) hack makes the new libvo interface work
// with the non-reentrant OMS interface
// 1. If we compile for OMS we do not add the user_data pointer
// 2. we set the initstructure types
#ifdef __OMS__
#define USERDATA
#define INITTYPE    plugin_output_video_t
#define ATTR_TYPE   plugin_output_video_attr_t 
#else
static void* user_data=NULL;

#define USERDATA    ,(user_data)
#define INITTYPE    vo_functions_t
#define ATTR_TYPE   vo_output_video_attr_t
#endif

//this is where we keep the state of the decoder
picture_t picture;

//global config struct
mpeg2_config_t config;

// the maximum chunk size is determined by vbv_buffer_size which is 224K for
// MP@ML streams. (we make no pretenses ofdecoding anything more than that)
static uint8_t chunk_buffer[224 * 1024 + 4];
static uint32_t shift = 0;

static int is_display_initialized = 0;
static int is_sequence_needed = 1;
static int drop_flag = 0;
static int drop_frame = 0;
static int in_slice = 0;

void mpeg2_init (void)
{
    config.flags = mm_accel () | MM_ACCEL_MLIB;
	/*try this if you have problems: replace mm_accel () with one of
	MM_ACCEL_MLIB
	MM_ACCEL_X86_MMX
	MM_ACCEL_X86_3DNOW
	MM_ACCEL_X86_MMXEXT
	this is only temporary...
	*/

    //intialize the decoder state 
    shift = 0;
    is_sequence_needed = 1;

    header_state_init (&picture);
    idct_init ();
    motion_comp_init ();
}

static void decode_allocate_image_buffers (INITTYPE * output, 
					   picture_t * picture)
{
    int frame_size;
    vo_image_buffer_t *tmp = NULL;

    frame_size = picture->coded_picture_width * picture->coded_picture_height;

    // allocate images in YV12 format
    tmp = output->allocate_image_buffer (picture->coded_picture_width, 
					  picture->coded_picture_height, 
					  0x32315659 USERDATA );

    picture->throwaway_frame[0] = tmp->base;
    picture->throwaway_frame[1] = tmp->base + frame_size * 5 / 4;
    picture->throwaway_frame[2] = tmp->base + frame_size;

    tmp = output->allocate_image_buffer (picture->coded_picture_width, 
					  picture->coded_picture_height, 
					  0x32315659 USERDATA );
    picture->backward_reference_frame[0] = tmp->base;
    picture->backward_reference_frame[1] = tmp->base + frame_size * 5 / 4;
    picture->backward_reference_frame[2] = tmp->base + frame_size;
    
    tmp = output->allocate_image_buffer (picture->coded_picture_width, 
					  picture->coded_picture_height, 
					  0x32315659 USERDATA );
    picture->forward_reference_frame[0] = tmp->base;
    picture->forward_reference_frame[1] = tmp->base + frame_size * 5 / 4;
    picture->forward_reference_frame[2] = tmp->base + frame_size;
}


static void decode_reorder_frames (void)
{
    if (picture.picture_coding_type != B_TYPE) {

	//reuse the soon to be outdated forward reference frame
	picture.current_frame[0] = picture.forward_reference_frame[0];
	picture.current_frame[1] = picture.forward_reference_frame[1];
	picture.current_frame[2] = picture.forward_reference_frame[2];

	//make the backward reference frame the new forward reference frame
	picture.forward_reference_frame[0] =
	    picture.backward_reference_frame[0];
	picture.forward_reference_frame[1] =
	    picture.backward_reference_frame[1];
	picture.forward_reference_frame[2] =
	    picture.backward_reference_frame[2];

	picture.backward_reference_frame[0] = picture.current_frame[0];
	picture.backward_reference_frame[1] = picture.current_frame[1];
	picture.backward_reference_frame[2] = picture.current_frame[2];

    } else {

	picture.current_frame[0] = picture.throwaway_frame[0];
	picture.current_frame[1] = picture.throwaway_frame[1];
	picture.current_frame[2] = picture.throwaway_frame[2];

    }
}


static int parse_chunk (INITTYPE * output, int code, uint8_t * buffer)
{
    int is_frame_done;

    if (is_sequence_needed && (code != 0xb3))	/* b3 = sequence_header_code */
	return 0;

    stats_header (code, buffer);

    is_frame_done = in_slice && ((!code) || (code >= 0xb0));

    if (is_frame_done) {
	in_slice = 0;

	if (!drop_frame) {
	    if (((HACK_MODE == 2) || (picture.mpeg1)) &&
		((picture.picture_structure == FRAME_PICTURE) ||
		 (picture.second_field))) {
		if (picture.picture_coding_type == B_TYPE)
		    output->draw_frame(picture.throwaway_frame);
		else
		    output->draw_frame(picture.forward_reference_frame);
	    }
	    output->flip_page ();
#ifdef ARCH_X86
	    if (config.flags & MM_ACCEL_X86_MMX)
		emms ();
#endif
	}
    }

    switch (code) {
    case 0x00:	/* picture_start_code */
	if (header_process_picture_header (&picture, buffer)) {
	    printf ("bad picture header\n");
	    exit (1);
	}
	drop_frame = drop_flag && (picture.picture_coding_type == B_TYPE);
	break;

    case 0xb3:	/* sequence_header_code */
	if (header_process_sequence_header (&picture, buffer)) {
	    printf ("bad sequence header\n");
	    exit (1);
	}
	is_sequence_needed = 0;
	break;

    case 0xb5:	/* extension_start_code */
	if (header_process_extension (&picture, buffer)) {
	    printf ("bad extension\n");
	    exit (1);
	}
	break;

    default:
	if (code >= 0xb9)
	    printf ("stream not demultiplexed ?\n");

	if (code >= 0xb0)
	    break;

	if (!(in_slice)) {
	    in_slice = 1;

	    if (!is_display_initialized) {
		ATTR_TYPE attr;

		attr.width = picture.coded_picture_width;
		attr.height = picture.coded_picture_height;
		attr.fullscreen = 0;
		attr.title = NULL;
#ifndef __OMS__
		attr.format = 0x32315659;
#endif

		if (output->setup (&attr USERDATA)) {
		    printf ("display init failed\n");
		    exit (1);
		}

		decode_allocate_image_buffers (output, &picture);
		decode_reorder_frames ();
		is_display_initialized = 1;
	    } else if (!picture.second_field)
		decode_reorder_frames ();
	}

	drop_frame |= drop_flag && (picture.picture_coding_type == B_TYPE);
	
	if (!drop_frame) {
	    slice_process (&picture, code, buffer);

	    if ((HACK_MODE < 2) && (!picture.mpeg1)) {
		uint8_t * foo[3];
		uint8_t ** bar;
		int offset;

		if (picture.picture_coding_type == B_TYPE)
		    bar = picture.throwaway_frame;
		else
		    bar = picture.forward_reference_frame;

		offset = (code-1) * 4 * picture.coded_picture_width;
		if ((! HACK_MODE) && (picture.picture_coding_type == B_TYPE))
		    offset = 0;

		foo[0] = bar[0] + 4 * offset;
		foo[1] = bar[1] + offset;
		foo[2] = bar[2] + offset;

		output->draw_slice (foo, code-1);
	    }

#ifdef ARCH_X86
	    if (config.flags & MM_ACCEL_X86_MMX)
		emms ();
#endif
	}
    }

    return is_frame_done;
}

int mpeg2_decode_data (INITTYPE *output, uint8_t *current, uint8_t *end)
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

	ret += parse_chunk (output, code, chunk_buffer);

	/* done with header or slice, prepare for next one */

	code = byte;
	chunk_ptr = chunk_buffer;
	shift = 0xffffff00;
    }

    return ret;
}

void mpeg2_close (INITTYPE * output)
{
    static uint8_t finalizer[] = {0,0,1,0};

    mpeg2_decode_data (output, finalizer, finalizer+4);

    if (is_display_initialized)
	output->draw_frame (picture.backward_reference_frame);

#ifdef __OMS__
    output->free_image_buffer (picture.backward_reference_frame);
    output->free_image_buffer (picture.forward_reference_frame);
    output->free_image_buffer (picture.throwaway_frame);
#endif
}

void mpeg2_drop (int flag)
{
    drop_flag = flag;
}

void mpeg2_output_init (int flag)
{
    is_display_initialized = flag;
}
