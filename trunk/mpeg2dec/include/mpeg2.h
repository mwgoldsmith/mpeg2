/*
 * mpeg2.h
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

#ifndef MPEG2_H
#define MPEG2_H

typedef struct {
    int width;
    int height;

    int display_width;
    int display_height;

    int aspect_ratio;
    int frame_rate_code;
    int bitrate;
    int progressive_sequence;
} sequence_t;

typedef struct {
    int progressive_frame;
    int top_field_first;
    int repeat_first_field;
    int picture_coding_type;
} picture_t;

typedef struct {
    sequence_t sequence;
    picture_t picture;
} mpeg2_info_t;

typedef struct {
    vo_instance_t * output;

    /* this is where we keep the state of the decoder */
    struct decoder_s * decoder;

    uint32_t shift;
    int is_display_initialized;
    int is_sequence_needed;
    int drop_flag;
    int drop_frame;
    int in_slice;

    /* the maximum chunk size is determined by vbv_buffer_size */
    /* which is 224K for MP@ML streams. */
    /* (we make no pretenses of decoding anything more than that) */
    /* allocated in init - gcc has problems allocating such big structures */
    uint8_t * chunk_buffer;
    /* pointer to current position in chunk_buffer */
    uint8_t * chunk_ptr;
    /* last start code ? */
    uint8_t code;

    /* PTS */
    uint32_t pts, pts_current, pts_previous;
    int num_pts;
    int bytes_since_pts;

    mpeg2_info_t info;
} mpeg2dec_t ;



typedef struct decoder_s decoder_t;

void mpeg2_header_state_init (decoder_t * decoder);

int mpeg2_header_picture (uint8_t * buffer, decoder_t * decoder,
                          picture_t * picture);

int mpeg2_header_sequence (uint8_t * buffer, decoder_t * decoder,
                           sequence_t * sequence);

int mpeg2_header_extension (uint8_t * buffer, decoder_t * decoder,
                            mpeg2_info_t * info);

void mpeg2_slice (decoder_t * decoder, int code, uint8_t * buffer);




void mpeg2_init (mpeg2dec_t * mpeg2dec, uint32_t mm_accel,
		 vo_instance_t * output);

void mpeg2_close (mpeg2dec_t * mpeg2dec);

int mpeg2_decode_data (mpeg2dec_t * mpeg2dec,
		       uint8_t * data_start, uint8_t * data_end);

void mpeg2_pts (mpeg2dec_t * mpeg2dec, uint32_t pts);

void mpeg2_drop (mpeg2dec_t * mpeg2dec, int flag);

#endif /* MPEG2_H */
