/*
 * decode.c
 * Copyright (C) 2000-2002 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
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

#ifdef HAVE_MEMALIGN
/* some systems have memalign() but no declaration for it */
void * memalign (size_t align, size_t size);
#else
/* assume malloc alignment is sufficient */
#define memalign(align,size) malloc (size)
#endif

#define BUFFER_SIZE (1194 * 1024)

#define STATE_INVALID 0
#define STATE_SEQUENCE 1
#define STATE_GOP 2
#define STATE_PICTURE 3
#define STATE_SLICE 4

void mpeg2_init (mpeg2dec_t * mpeg2dec, uint32_t mm_accel,
		 vo_instance_t * output)
{
    static int do_init = 1;

    if (do_init) {
	do_init = 0;
	mpeg2_cpu_state_init (mm_accel);
	mpeg2_idct_init (mm_accel);
	mpeg2_mc_init (mm_accel);
    }

    mpeg2dec->chunk_buffer = memalign (16, BUFFER_SIZE + 4);
    mpeg2dec->decoder = memalign (16, sizeof (decoder_t));

    mpeg2dec->shift = 0xffffff00;
    mpeg2dec->is_sequence_needed = 1;
    mpeg2dec->state = STATE_INVALID;
    mpeg2dec->output = output;
    mpeg2dec->chunk_ptr = mpeg2dec->chunk_buffer;
    mpeg2dec->code = 0xb4;

    memset (mpeg2dec->decoder, 0, sizeof (decoder_t));

    /* initialize substructures */
    mpeg2_header_state_init (mpeg2dec->decoder);
}

static inline int crap1 (mpeg2dec_t * mpeg2dec)
{
    decoder_t * decoder = mpeg2dec->decoder;
    int is_frame_done = 0;

    if ((decoder->picture_structure == FRAME_PICTURE) ||
	(decoder->second_field)) {
	is_frame_done = 1;
	vo_draw ((decoder->coding_type == B_TYPE) ?
		 decoder->current_frame :
		 decoder->forward_reference_frame);
    }
    return is_frame_done;
}

static inline void crap2 (mpeg2dec_t * mpeg2dec)
{
    decoder_t * decoder = mpeg2dec->decoder;

    if (mpeg2dec->is_sequence_needed) {
	mpeg2dec->is_sequence_needed = 0;
	if (vo_setup (mpeg2dec->output, decoder->width, decoder->height)) {
	    fprintf (stderr, "display setup failed\n");
	    exit (1);
	}
	decoder->forward_reference_frame =
	    vo_get_frame (mpeg2dec->output,
			  VO_PREDICTION_FLAG | VO_BOTH_FIELDS);
	decoder->backward_reference_frame =
	    vo_get_frame (mpeg2dec->output,
			  VO_PREDICTION_FLAG | VO_BOTH_FIELDS);
    }
}

static inline void crap3 (mpeg2dec_t * mpeg2dec)
{
    decoder_t * decoder = mpeg2dec->decoder;

    if (decoder->second_field)
	vo_field (decoder->current_frame, decoder->picture_structure);
    else {
	if (decoder->coding_type == B_TYPE)
	    decoder->current_frame =
		vo_get_frame (mpeg2dec->output,
			      decoder->picture_structure);
	else {
	    decoder->current_frame =
		vo_get_frame (mpeg2dec->output,
			      (VO_PREDICTION_FLAG |
			       decoder->picture_structure));
	    decoder->forward_reference_frame =
		decoder->backward_reference_frame;
	    decoder->backward_reference_frame = decoder->current_frame;
	}
    }
}

static inline uint8_t * copy_chunk (mpeg2dec_t * mpeg2dec,
				    uint8_t * current, uint8_t * end)
{
    uint32_t shift;
    uint8_t * chunk_ptr;
    uint8_t * limit;
    uint8_t byte;

    shift = mpeg2dec->shift;
    chunk_ptr = mpeg2dec->chunk_ptr;
    limit = current + (mpeg2dec->chunk_buffer + BUFFER_SIZE - chunk_ptr);
    if (limit > end)
	limit = end;

    do {
	byte = *current++;
	if (shift == 0x00000100)
	    goto startcode;
	shift = (shift | byte) << 8;
	*chunk_ptr++ = byte;
    } while (current < limit);

    mpeg2dec->bytes_since_pts += chunk_ptr - mpeg2dec->chunk_ptr;
    mpeg2dec->shift = shift;
    if (current == end) {
	mpeg2dec->chunk_ptr = chunk_ptr;
	return NULL;
    } else {
	/* we filled the chunk buffer without finding a start code */
	mpeg2dec->chunk_ptr = mpeg2dec->chunk_buffer;
	mpeg2dec->code = 0xb4;	/* sequence_error_code */
	return current;
    }

startcode:
    mpeg2dec->bytes_since_pts += chunk_ptr + 1 - mpeg2dec->chunk_ptr;
    mpeg2dec->chunk_ptr = mpeg2dec->chunk_buffer;
    mpeg2dec->shift = 0xffffff00;
    mpeg2dec->code = byte;
    if (!byte) {
	if (!mpeg2dec->num_pts)
	    mpeg2dec->pts = 0;	/* none */
	else if (mpeg2dec->bytes_since_pts >= 4) {
	    mpeg2dec->num_pts = 0;
	    mpeg2dec->pts = mpeg2dec->pts_current;
	} else if (mpeg2dec->num_pts > 1) {
	    mpeg2dec->num_pts = 1;
	    mpeg2dec->pts = mpeg2dec->pts_previous;
	} else
	    mpeg2dec->pts = 0;	/* none */
    }
    return current;
}

int mpeg2_decode_data (mpeg2dec_t * mpeg2dec, uint8_t * current, uint8_t * end)
{
    int ret;
    uint8_t code;
    decoder_t * decoder;

    ret = 0;

    while (current != end) {
	code = mpeg2dec->code;
	current = copy_chunk (mpeg2dec, current, end);
	if (current == NULL)
	    return ret;

	/* wait for sequence_header_code */
	if (mpeg2dec->state == STATE_INVALID && code != 0xb3)
	    continue;

	mpeg2_stats (code, mpeg2dec->chunk_buffer);

	decoder = mpeg2dec->decoder;

	switch (code) {
	case 0x00:	/* picture_start_code */
	    mpeg2_header_picture (mpeg2dec->chunk_buffer,
				  &(mpeg2dec->info.picture), decoder);
	    break;
	case 0xb3:	/* sequence_header_code */
	    mpeg2_header_sequence (mpeg2dec->chunk_buffer,
				   &(mpeg2dec->info.sequence), decoder);
	    mpeg2dec->state = STATE_SEQUENCE;
	    break;
	case 0xb5:	/* extension_start_code */
	    mpeg2_header_extension (mpeg2dec->chunk_buffer,
				    &(mpeg2dec->info), decoder);
	    break;
	default:
	    if (code < 0xb0)
		mpeg2_slice (decoder, code, mpeg2dec->chunk_buffer);
	}

#define RECEIVED(code,state) (((state) << 8) + (code))

	switch (RECEIVED (mpeg2dec->code, mpeg2dec->state)) {

	/* legal state transitions */
	case RECEIVED (0x00, STATE_SEQUENCE):
	case RECEIVED (0x00, STATE_GOP):
	case RECEIVED (0x00, STATE_SLICE):
	case RECEIVED (0xb3, STATE_SLICE):
	case RECEIVED (0xb7, STATE_SLICE):
	case RECEIVED (0xb8, STATE_SEQUENCE):
	case RECEIVED (0xb8, STATE_SLICE):
	    switch (mpeg2dec->state) {
	    case STATE_SEQUENCE:
		crap2 (mpeg2dec);		/* alloc picture buffers */
		break;
	    case STATE_SLICE:
		ret += crap1 (mpeg2dec);	/* display picture, swap buf */
		break;
	    }
	    switch (mpeg2dec->code) {
	    case 0x00:	/* picture_start_code */
		mpeg2dec->state = STATE_PICTURE;	break;
	    case 0xb3:	/* sequence_header_code */
		mpeg2dec->state = STATE_SEQUENCE;	break;
	    case 0xb7:	/* sequence_end_code */
		mpeg2dec->state = STATE_INVALID;	break;
	    case 0xb8:	/* group_start_code */
		mpeg2dec->state = STATE_GOP;		break;
	    }

	/* legal headers within a given state */
	case RECEIVED (0xb2, STATE_SEQUENCE):
	case RECEIVED (0xb2, STATE_GOP):
	case RECEIVED (0xb2, STATE_PICTURE):
	case RECEIVED (0xb5, STATE_SEQUENCE):
	case RECEIVED (0xb5, STATE_PICTURE):
	    break;

	default:
	    if (mpeg2dec->code >= 0xb0) {
	case RECEIVED (0x00, STATE_PICTURE):
	illegal:
		/* illegal codes (0x00 - 0xb8) or system codes (0xb9 - 0xff) */
		break;
	    } else if (mpeg2dec->state == STATE_SLICE)
		break;		/* second slice and later */
	    else if (mpeg2dec->state != STATE_PICTURE)
		goto illegal;	/* slice at unexpected place */
	    mpeg2dec->state = STATE_SLICE;
	    crap3 (mpeg2dec);	/* done decoding picture */
	}
    }
    return ret;
}

void mpeg2_pts (mpeg2dec_t * mpeg2dec, uint32_t pts)
{
    mpeg2dec->pts_previous = mpeg2dec->pts_current;
    mpeg2dec->pts_current = pts;
    mpeg2dec->num_pts++;
    mpeg2dec->bytes_since_pts = 0;
}

void mpeg2_close (mpeg2dec_t * mpeg2dec)
{
    static uint8_t finalizer[] = {0,0,1,0xb4};

    mpeg2_decode_data (mpeg2dec, finalizer, finalizer+4);

    if (! (mpeg2dec->is_sequence_needed))
	vo_draw (mpeg2dec->decoder->backward_reference_frame);

    free (mpeg2dec->chunk_buffer);
    free (mpeg2dec->decoder);
}