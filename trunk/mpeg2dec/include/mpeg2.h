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

#define SEQ_FLAG_MPEG2 1
#define SEQ_FLAG_CONSTRAINED_PARAMETERS 2
#define SEQ_FLAG_PROGRESSIVE_SEQUENCE 4
#define SEQ_FLAG_LOW_DELAY 8
#define SEQ_FLAG_COLOUR_DESCRIPTION 16

#define SEQ_MASK_VIDEO_FORMAT 0xe0
#define SEQ_VIDEO_FORMAT_COMPONENT 0
#define SEQ_VIDEO_FORMAT_PAL 0x20
#define SEQ_VIDEO_FORMAT_NTSC 0x40
#define SEQ_VIDEO_FORMAT_SECAM 0x60
#define SEQ_VIDEO_FORMAT_MAC 0x80
#define SEQ_VIDEO_FORMAT_UNSPECIFIED 0xa0

/* this flag is private, do not rely on it */
#define SEQ_FLAG_SQUARE_PIXEL 0x80000000

typedef struct {
    unsigned int width, height;
    unsigned int chroma_width, chroma_height;
    unsigned int byte_rate;
    unsigned int vbv_buffer_size;
    uint32_t flags;

    unsigned int picture_width, picture_height;
    unsigned int display_width, display_height;
    unsigned int pixel_width, pixel_height;
    unsigned int frame_period;

    uint8_t profile_level_id;
    uint8_t colour_primaries;
    uint8_t transfer_characteristics;
    uint8_t matrix_coefficients;
} sequence_t;

#define PIC_MASK_CODING_TYPE 7
#define PIC_FLAG_CODING_TYPE_I 1
#define PIC_FLAG_CODING_TYPE_P 2
#define PIC_FLAG_CODING_TYPE_B 3
#define PIC_FLAG_CODING_TYPE_D 4

#define PIC_FLAG_TOP_FIELD_FIRST 8
#define PIC_FLAG_PROGRESSIVE_FRAME 16
#define PIC_FLAG_COMPOSITE_DISPLAY 32
#define PIC_MASK_COMPOSITE_DISPLAY 0xfffff000

typedef struct {
    unsigned int temporal_reference;
    unsigned int nb_fields;
    uint32_t flags;
} picture_t;

typedef struct {
    /* this is where we keep the state of the decoder */
    struct decoder_s * decoder;

    uint32_t shift;
    int is_display_initialized;
    int state;
    uint32_t ext_state;

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

    vo_frame_t * current_frame;
    vo_frame_t * forward_reference_frame;
    vo_frame_t * backward_reference_frame;

    sequence_t last_sequence;
    sequence_t sequence;
    picture_t picture;
} mpeg2dec_t ;

typedef struct decoder_s decoder_t;

#define STATE_INVALID 0 
#define STATE_SEQUENCE 1 
#define STATE_GOP 2 
#define STATE_PICTURE 3 
#define STATE_SLICE_1ST 4 
#define STATE_PICTURE_2ND 5 
#define STATE_SLICE 6 
#define STATE_END 7

void mpeg2_header_state_init (decoder_t * decoder);
int mpeg2_header_sequence (mpeg2dec_t * mpeg2dec);
int mpeg2_header_gop (mpeg2dec_t * mpeg2dec);
int mpeg2_header_picture (mpeg2dec_t * mpeg2dec);
int mpeg2_header_extension (mpeg2dec_t * mpeg2dec);
int mpeg2_header_user_data (mpeg2dec_t * mpeg2dec);

int mpeg2_set_buf (mpeg2dec_t * mpeg2dec, vo_frame_t * buf);
void mpeg2_init_fbuf (decoder_t * decoder, uint8_t * current_fbuf[3],
		      uint8_t * forward_fbuf[3], uint8_t * backward_fbuf[3]);

void mpeg2_slice (decoder_t * decoder, int code, uint8_t * buffer);

void mpeg2_init (mpeg2dec_t * mpeg2dec, uint32_t mm_accel);

void mpeg2_close (mpeg2dec_t * mpeg2dec);

int mpeg2_buffer (mpeg2dec_t * mpeg2dec, uint8_t ** current, uint8_t * end);

void mpeg2_pts (mpeg2dec_t * mpeg2dec, uint32_t pts);

#endif /* MPEG2_H */
