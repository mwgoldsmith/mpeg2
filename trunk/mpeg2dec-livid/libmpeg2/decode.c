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
#include <string.h>	/* memcpy/memset, try to remove */
#include <stdlib.h>
#include <inttypes.h>

#include "video_out.h"
#include "mpeg2.h"
#include "mpeg2_internal.h"
#include "mm_accel.h"
#include "attributes.h"
#include "mmx.h"

mpeg2_config_t config;

void mpeg2_init (mpeg2dec_t * mpeg2dec, vo_output_video_t * output)
{
    static int do_init = 1;

    if (do_init) {
	do_init = 0;
	config.flags = mm_accel () | MM_ACCEL_MLIB;
	idct_init ();
	motion_comp_init ();
	mpeg2dec->chunk_buffer = (uint8_t *) malloc(224 * 1024 + 4);
	mpeg2dec->picture = (picture_t *) malloc (sizeof (picture_t));
    }

    mpeg2dec->shift = 0;
    mpeg2dec->is_display_initialized = 0;
    mpeg2dec->is_sequence_needed = 1;
    mpeg2dec->drop_flag = 0;
    mpeg2dec->drop_frame = 0;
    mpeg2dec->in_slice = 0;
    mpeg2dec->output = output;
    mpeg2dec->output_data = NULL;
    mpeg2dec->chunk_ptr = mpeg2dec->chunk_buffer;
    mpeg2dec->code = 0xff;

    memset (mpeg2dec->picture, 0, sizeof (picture_t));

    /* initialize supstructures */
    header_state_init (mpeg2dec->picture);
}

static int parse_chunk (mpeg2dec_t * mpeg2dec, int code, uint8_t * buffer)
{
    picture_t * picture;
    int is_frame_done;

    /* wait for sequence_header_code */
    if (mpeg2dec->is_sequence_needed && (code != 0xb3))
	return 0;

    stats_header (code, buffer);

    picture = mpeg2dec->picture;
    is_frame_done = mpeg2dec->in_slice && ((!code) || (code >= 0xb0));

    if (is_frame_done) {
	mpeg2dec->in_slice = 0;

	if (((picture->picture_structure == FRAME_PICTURE) ||
	     (picture->second_field)) &&
	    (!(mpeg2dec->drop_frame))) {
	    mpeg2dec->output->draw_frame
		((picture->picture_coding_type == B_TYPE) ?
		 picture->current_frame : picture->forward_reference_frame);
#ifdef ARCH_X86
	    if (config.flags & MM_ACCEL_X86_MMX)
		emms ();
#endif
	}
    }

    switch (code) {
    case 0x00:	/* picture_start_code */
	if (header_process_picture_header (picture, buffer)) {
	    fprintf (stderr, "bad picture header\n");
	    exit (1);
	}
	mpeg2dec->drop_frame =
	    mpeg2dec->drop_flag && (picture->picture_coding_type == B_TYPE);
	break;

    case 0xb3:	/* sequence_header_code */
	if (header_process_sequence_header (picture, buffer)) {
	    fprintf (stderr, "bad sequence header\n");
	    exit (1);
	}
	mpeg2dec->is_sequence_needed = 0;
	break;

    case 0xb5:	/* extension_start_code */
	if (header_process_extension (picture, buffer)) {
	    fprintf (stderr, "bad extension\n");
	    exit (1);
	}
	break;

    default:
	if (code >= 0xb9)
	    fprintf (stderr, "stream not demultiplexed ?\n");

	if (code >= 0xb0)
	    break;

	if (!(mpeg2dec->in_slice)) {
	    mpeg2dec->in_slice = 1;

	    if (!(mpeg2dec->is_display_initialized)) {
		mpeg2dec->output_data =
		    mpeg2dec->output->setup (mpeg2dec->output_data,
					     picture->coded_picture_width,
					     picture->coded_picture_height);
		if (mpeg2dec->output_data == NULL) {
		    fprintf (stderr, "display setup failed\n");
		    exit (1);
		}
		picture->forward_reference_frame =
		    mpeg2dec->output->get_frame (mpeg2dec->output_data, 1);
		picture->backward_reference_frame =
		    mpeg2dec->output->get_frame (mpeg2dec->output_data, 1);
		mpeg2dec->is_display_initialized = 1;
	    }
	    if (!(picture->second_field)) {
		if (picture->picture_coding_type == B_TYPE)
		    picture->current_frame =
			mpeg2dec->output->get_frame (mpeg2dec->output_data, 0);
		else {
		    picture->current_frame =
			mpeg2dec->output->get_frame (mpeg2dec->output_data, 1);
		    picture->forward_reference_frame =
			picture->backward_reference_frame;
		    picture->backward_reference_frame = picture->current_frame;
#ifdef ARCH_X86
		    if (config.flags & MM_ACCEL_X86_MMX)
			emms ();	/* FIXME not needed ? */
#endif
		}
	    }
	}

	if (!(mpeg2dec->drop_frame)) {
	    slice_process (picture, code, buffer);

#ifdef ARCH_X86
	    if (config.flags & MM_ACCEL_X86_MMX)
		emms ();
#endif
	}
    }

    return is_frame_done;
}

int mpeg2_decode_data (mpeg2dec_t * mpeg2dec, uint8_t * current, uint8_t * end)
{
    uint32_t shift;
    uint8_t * chunk_ptr;
    uint8_t byte;
    int ret = 0;

    shift = mpeg2dec->shift;
    chunk_ptr = mpeg2dec->chunk_ptr;

    while (current != end) {
	while (1) {
	    byte = *current++;
	    if (shift != 0x00000100) {
		*chunk_ptr++ = byte;
		shift = (shift | byte) << 8;
		if (current != end)
		    continue;
		mpeg2dec->chunk_ptr = chunk_ptr;
		mpeg2dec->shift = shift;
		return ret;
	    }
	    break;
	}

	/* found start_code following chunk */

	ret += parse_chunk (mpeg2dec, mpeg2dec->code, mpeg2dec->chunk_buffer);

	/* done with header or slice, prepare for next one */

	mpeg2dec->code = byte;
	chunk_ptr = mpeg2dec->chunk_buffer;
	shift = 0xffffff00;
    }
    mpeg2dec->chunk_ptr = chunk_ptr;
    mpeg2dec->shift = shift;
    return ret;
}

void mpeg2_close (mpeg2dec_t * mpeg2dec)
{
    static uint8_t finalizer[] = {0,0,1,0};

    mpeg2_decode_data (mpeg2dec, finalizer, finalizer+4);

    if (mpeg2dec->is_display_initialized)
	mpeg2dec->output->draw_frame
	    (mpeg2dec->picture->backward_reference_frame);
    mpeg2dec->output->close (mpeg2dec->output_data);
}

void mpeg2_drop (mpeg2dec_t * mpeg2dec, int flag)
{
    mpeg2dec->drop_flag = flag;
}
