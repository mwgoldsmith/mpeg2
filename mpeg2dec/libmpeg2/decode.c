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
#include <string.h>	/* memcmp/memset, try to remove */
#include <stdlib.h>
#include <inttypes.h>

#include "mpeg2.h"
#include "mpeg2_internal.h"
#include "convert.h"

#define BUFFER_SIZE (1194 * 1024)

mpeg2dec_t * mpeg2_init (uint32_t mm_accel)
{
    static int do_init = 1;
    mpeg2dec_t * mpeg2dec;

    mpeg2dec = mpeg2_malloc (sizeof (mpeg2dec_t), ALLOC_MPEG2DEC);
    if (mpeg2dec == NULL)
	return NULL;

    if (do_init) {
	do_init = 0;
	mpeg2_cpu_state_init (mm_accel);
	mpeg2_idct_init (mm_accel);
	mpeg2_mc_init (mm_accel);
    }

    memset (mpeg2dec, 0, sizeof (mpeg2dec_t));

    mpeg2dec->chunk_buffer = mpeg2_malloc (BUFFER_SIZE + 4, ALLOC_CHUNK);

    mpeg2dec->shift = 0xffffff00;
    mpeg2dec->last_sequence.width = (unsigned int) -1;
    mpeg2dec->state = STATE_INVALID;
    mpeg2dec->chunk_ptr = mpeg2dec->chunk_buffer;
    mpeg2dec->code = 0xb4;

    /* initialize substructures */
    mpeg2_header_state_init (mpeg2dec);

    return mpeg2dec;
}

const mpeg2_info_t * mpeg2_info (mpeg2dec_t * mpeg2dec)
{
    return &(mpeg2dec->info);
}

static inline int copy_chunk (mpeg2dec_t * mpeg2dec)
{
    uint8_t * current;
    uint32_t shift;
    uint8_t * chunk_ptr;
    uint8_t * limit;
    uint8_t byte;

    current = mpeg2dec->buf_start;
    if (current == mpeg2dec->buf_end)
	return 1;

    shift = mpeg2dec->shift;
    chunk_ptr = mpeg2dec->chunk_ptr;
    limit = current + (mpeg2dec->chunk_buffer + BUFFER_SIZE - chunk_ptr);
    if (limit > mpeg2dec->buf_end)
	limit = mpeg2dec->buf_end;

    do {
	byte = *current++;
	if (shift == 0x00000100)
	    goto startcode;
	shift = (shift | byte) << 8;
	*chunk_ptr++ = byte;
    } while (current < limit);

    mpeg2dec->bytes_since_pts += chunk_ptr - mpeg2dec->chunk_ptr;
    mpeg2dec->shift = shift;
    if (current == mpeg2dec->buf_end) {
	mpeg2dec->chunk_ptr = chunk_ptr;
	return 1;
    } else {
	/* we filled the chunk buffer without finding a start code */
	mpeg2dec->chunk_ptr = mpeg2dec->chunk_buffer;
	mpeg2dec->code = 0xb4;	/* sequence_error_code */
	mpeg2dec->buf_start = current;
	return 0;
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
    mpeg2dec->buf_start = current;
    return 0;
}

void mpeg2_buffer (mpeg2dec_t * mpeg2dec, uint8_t * start, uint8_t * end)
{
    mpeg2dec->buf_start = start;
    mpeg2dec->buf_end = end;
}

static inline int repeated_sequence (sequence_t * seq1, sequence_t * seq2)
{
    /*
     * according to 6.1.1.6, repeat sequence headers should be
     * identical to the original. However some DVDs dont respect that
     * and have different bitrates in the repeat sequence headers. So
     * we'll ignore that in the comparison and still consider these as
     * repeat sequence headers - the drawback of this, though, is that
     * the user will never get to see the new bitrate value.
     */

    return (seq1->width == seq2->width &&
	    seq1->height == seq2->height &&
	    seq1->chroma_width == seq2->chroma_width &&
	    seq1->chroma_height == seq2->chroma_height &&
	    seq1->vbv_buffer_size == seq2->vbv_buffer_size &&
	    seq1->flags && seq2->flags &&
	    seq1->picture_width == seq2->picture_width &&
	    seq1->picture_height == seq2->picture_height &&
	    seq1->display_width == seq2->display_width &&
	    seq1->display_height == seq2->display_height &&
	    seq1->pixel_width == seq2->pixel_width &&
	    seq1->pixel_height == seq2->pixel_height &&
	    seq1->frame_period == seq2->frame_period &&
	    seq1->profile_level_id == seq2->profile_level_id &&
	    seq1->colour_primaries == seq2->colour_primaries &&
	    seq1->transfer_characteristics == seq2->transfer_characteristics &&
	    seq1->matrix_coefficients == seq2->matrix_coefficients);
}

int mpeg2_parse (mpeg2dec_t * mpeg2dec)
{
    uint8_t code;

    if (mpeg2dec->state == STATE_END) {
	mpeg2_header_end (mpeg2dec);
	return STATE_END;
    }

    while (1) {
	code = mpeg2dec->code;
	if (copy_chunk (mpeg2dec))
	    return -1;

	/* wait for sequence_header_code */
	if (mpeg2dec->state == STATE_INVALID && code != 0xb3)
	    continue;

	mpeg2_stats (code, mpeg2dec->chunk_buffer);

	switch (code) {
	case 0x00:	/* picture_start_code */
	    mpeg2_header_picture (mpeg2dec);
	    break;
	case 0xb2:	/* user_data_start_code */
	    mpeg2_header_user_data (mpeg2dec);
	    break;
	case 0xb3:	/* sequence_header_code */
	    mpeg2_header_sequence (mpeg2dec);
	    break;
	case 0xb5:	/* extension_start_code */
	    mpeg2_header_extension (mpeg2dec);
	    break;
	case 0xb8:	/* group_start_code */
	    mpeg2_header_gop (mpeg2dec);
	    break;
	default:
	    if (code >= 0xb0)
		break;
	    if (mpeg2dec->state != STATE_SLICE &&
		mpeg2dec->state != STATE_SLICE_1ST)
		mpeg2_header_slice (mpeg2dec);
	    mpeg2_slice (&(mpeg2dec->decoder), code, mpeg2dec->chunk_buffer);
	}

#define RECEIVED(code,state) (((state) << 8) + (code))

	switch (RECEIVED (mpeg2dec->code, mpeg2dec->state)) {

	/* state transition after a sequence header */
	case RECEIVED (0x00, STATE_SEQUENCE):
	case RECEIVED (0xb8, STATE_SEQUENCE):
	    mpeg2_header_sequence_finalize (mpeg2dec);
	    if (!repeated_sequence(&(mpeg2dec->last_sequence),
				   &(mpeg2dec->sequence))) {
		mpeg2dec->last_sequence = mpeg2dec->sequence;
		return STATE_SEQUENCE;
	    }
	    break;

	/* end of sequence */
	case RECEIVED (0xb7, STATE_SLICE):
	    mpeg2dec->state = STATE_END;
	    mpeg2dec->last_sequence.width = (unsigned int) -1;
	    return STATE_SLICE;

	/* other legal state transitions */
	case RECEIVED (0x00, STATE_GOP):
	case RECEIVED (0x00, STATE_SLICE_1ST):
	case RECEIVED (0x00, STATE_SLICE):
	case RECEIVED (0xb3, STATE_SLICE):
	case RECEIVED (0xb8, STATE_SLICE):
	    return mpeg2dec->state;

	/* legal headers within a given state */
	case RECEIVED (0xb2, STATE_SEQUENCE):
	case RECEIVED (0xb2, STATE_GOP):
	case RECEIVED (0xb2, STATE_PICTURE):
	case RECEIVED (0xb2, STATE_PICTURE_2ND):
	case RECEIVED (0xb5, STATE_SEQUENCE):
	case RECEIVED (0xb5, STATE_PICTURE):
	case RECEIVED (0xb5, STATE_PICTURE_2ND):
	    break;

	default:
	    if (mpeg2dec->code >= 0xb0) {
	case RECEIVED (0x00, STATE_PICTURE):
	case RECEIVED (0x00, STATE_PICTURE_2ND):
	illegal:
		/* illegal codes (0x00 - 0xb8) or system codes (0xb9 - 0xff) */
		mpeg2dec->state = STATE_INVALID;
		break;
	    } else if (mpeg2dec->state == STATE_PICTURE ||
		       mpeg2dec->state == STATE_PICTURE_2ND)
		return mpeg2dec->state;
	    else if (mpeg2dec->state != STATE_SLICE &&
		     mpeg2dec->state != STATE_SLICE_1ST)
		goto illegal;	/* slice at unexpected position */
	}
    }
}

void mpeg2_convert (mpeg2dec_t * mpeg2dec,
		    void (* convert) (int, int, void *,
				      struct convert_init_s *), void * arg)
{
    convert_init_t convert_init;
    int size;

    convert_init.id = NULL;
    convert (mpeg2dec->decoder.width, mpeg2dec->decoder.height, arg,
	     &convert_init);
    if (convert_init.id_size) {
	convert_init.id = mpeg2dec->convert_id =
	    mpeg2_malloc (convert_init.id_size, ALLOC_CONVERT_ID);
	convert (mpeg2dec->decoder.width, mpeg2dec->decoder.height, arg,
		 &convert_init);
    }
    mpeg2dec->convert_size[0] = size = convert_init.buf_size[0];
    mpeg2dec->convert_size[1] = size += convert_init.buf_size[1];
    mpeg2dec->convert_size[2] = size += convert_init.buf_size[2];
    mpeg2dec->convert_start = convert_init.start;
    mpeg2dec->convert_copy = convert_init.copy;

    size = mpeg2dec->decoder.width * mpeg2dec->decoder.height >> 2;
    mpeg2dec->yuv_buf[0][0] = mpeg2_malloc (6 * size, ALLOC_YUV);
    mpeg2dec->yuv_buf[0][1] = mpeg2dec->yuv_buf[0][0] + 4 * size;
    mpeg2dec->yuv_buf[0][2] = mpeg2dec->yuv_buf[0][0] + 5 * size;
    mpeg2dec->yuv_buf[1][0] = mpeg2_malloc (6 * size, ALLOC_YUV);
    mpeg2dec->yuv_buf[1][1] = mpeg2dec->yuv_buf[1][0] + 4 * size;
    mpeg2dec->yuv_buf[1][2] = mpeg2dec->yuv_buf[1][0] + 5 * size;
    size = mpeg2dec->decoder.width * 8;
    mpeg2dec->yuv_buf[2][0] = mpeg2_malloc (6 * size, ALLOC_YUV);
    mpeg2dec->yuv_buf[2][1] = mpeg2dec->yuv_buf[2][0] + 4 * size;
    mpeg2dec->yuv_buf[2][2] = mpeg2dec->yuv_buf[2][0] + 5 * size;
}

void mpeg2_set_buf_alloc (mpeg2dec_t * mpeg2dec, uint8_t * buf[3], void * id)
{
    mpeg2dec->fbuf[2] = mpeg2dec->fbuf[1];
    mpeg2dec->fbuf[1] = mpeg2dec->fbuf[0];
    mpeg2dec->fbuf_alloc[2] = mpeg2dec->fbuf_alloc[1];
    mpeg2dec->fbuf_alloc[1] = mpeg2dec->fbuf_alloc[0];
    mpeg2dec->fbuf_alloc[2].free = 1;

    mpeg2dec->fbuf[0].buf[0] = buf[0];
    mpeg2dec->fbuf[0].buf[1] = buf[1];
    mpeg2dec->fbuf[0].buf[2] = buf[2];
    mpeg2dec->fbuf[0].id = id;
    mpeg2dec->fbuf_alloc[0].fbuf = mpeg2dec->fbuf[0];
    mpeg2dec->fbuf_alloc[0].free = 0;
}

void mpeg2_set_buf_alloc_XXX (mpeg2dec_t * mpeg2dec)
{
    uint8_t * buf[3];

    if (mpeg2dec->convert_start) {
	buf[0] = mpeg2_malloc (mpeg2dec->convert_size[0], ALLOC_CONVERTED);
	buf[1] = buf[0] + mpeg2dec->convert_size[1];
	buf[2] = buf[0] + mpeg2dec->convert_size[2];
    } else {
	int size;

	size = mpeg2dec->decoder.width * mpeg2dec->decoder.height >> 2;
	buf[0] = mpeg2_malloc (6 * size, ALLOC_YUV);
	buf[1] = buf[0] + 4 * size;
	buf[2] = buf[0] + 5 * size;
    }
    mpeg2_set_buf_alloc (mpeg2dec, buf, NULL);
}

void mpeg2_set_buf (mpeg2dec_t * mpeg2dec, uint8_t * buf[3], void * id)
{
    mpeg2dec->fbuf->buf[0] = buf[0];
    mpeg2dec->fbuf->buf[1] = buf[1];
    mpeg2dec->fbuf->buf[2] = buf[2];
    mpeg2dec->fbuf->id = id;
    switch (mpeg2dec->state) {
    case STATE_PICTURE:
	mpeg2dec->info.current_fbuf = mpeg2dec->fbuf;
	if ((mpeg2dec->decoder.coding_type == B_TYPE) ||
	    (mpeg2dec->sequence.flags & SEQ_FLAG_LOW_DELAY)) {
	    if ((mpeg2dec->decoder.coding_type == B_TYPE) ||
		(mpeg2dec->convert_start))
		mpeg2dec->info.discard_fbuf = mpeg2dec->fbuf;
	    mpeg2dec->info.display_fbuf = mpeg2dec->fbuf;
	}
	break;
    case STATE_SEQUENCE:
	mpeg2dec->fbuf[2] = mpeg2dec->fbuf[1];
	mpeg2dec->fbuf[1] = mpeg2dec->fbuf[0];
	break;
    }
}

void mpeg2_custom_fbuf (mpeg2dec_t * mpeg2dec, int custom_fbuf)
{
    mpeg2dec->custom_fbuf = custom_fbuf;
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
    /* static uint8_t finalizer[] = {0,0,1,0xb4}; */

    /* mpeg2_decode_data (mpeg2dec, finalizer, finalizer+4); */

    mpeg2_free (mpeg2dec->chunk_buffer);
}
