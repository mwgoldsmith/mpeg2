/*
 * header.c
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

#include <inttypes.h>
#include <stdlib.h>	/* defines NULL */

#include "video_out.h"
#include "mpeg2.h"
#include "mpeg2_internal.h"
#include "attributes.h"

#define SEQ_EXT 2
#define SEQ_DISPLAY_EXT 4
#define QUANT_MATRIX_EXT 8
#define COPYRIGHT_EXT 0x10
#define PIC_DISPLAY_EXT 0x80
#define PIC_CODING_EXT 0x100
#define USER_DATA 0x10000

/* default intra quant matrix, in zig-zag order */
static uint8_t default_intra_quantizer_matrix[64] ATTR_ALIGN(16) = {
    8,
    16, 16,
    19, 16, 19,
    22, 22, 22, 22,
    22, 22, 26, 24, 26,
    27, 27, 27, 26, 26, 26,
    26, 27, 27, 27, 29, 29, 29,
    34, 34, 34, 29, 29, 29, 27, 27,
    29, 29, 32, 32, 34, 34, 37,
    38, 37, 35, 35, 34, 35,
    38, 38, 40, 40, 40,
    48, 48, 46, 46,
    56, 56, 58,
    69, 69,
    83
};

uint8_t mpeg2_scan_norm[64] ATTR_ALIGN(16) = {
    /* Zig-Zag scan pattern */
     0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};

uint8_t mpeg2_scan_alt[64] ATTR_ALIGN(16) = {
    /* Alternate scan pattern */
     0, 8,  16, 24,  1,  9,  2, 10, 17, 25, 32, 40, 48, 56, 57, 49,
    41, 33, 26, 18,  3, 11,  4, 12, 19, 27, 34, 42, 50, 58, 35, 43,
    51, 59, 20, 28,  5, 13,  6, 14, 21, 29, 36, 44, 52, 60, 37, 45,
    53, 61, 22, 30,  7, 15, 23, 31, 38, 46, 54, 62, 39, 47, 55, 63
};

void mpeg2_header_state_init (mpeg2dec_t * mpeg2dec)
{
    mpeg2dec->decoder.scan = mpeg2_scan_norm;
    mpeg2dec->picture = mpeg2dec->pictures;
    mpeg2dec->first = 1;
}

static void simplify (unsigned int * num, unsigned int * denum) 
{
    int a, b, tmp;

    a = *num;
    b = *denum;
    while (a) {
	tmp = a;
	a = b % tmp;
	b = tmp;
    }
    *num /= b;
    *denum /= b;
}

static void reset_info (mpeg2_info_t * info)
{
    info->current_picture = info->current_picture_2nd = NULL;
    info->display_picture = info->display_picture_2nd = NULL;
    info->current_fbuf = info->display_fbuf = info->discard_fbuf = NULL;
}

int mpeg2_header_sequence (mpeg2dec_t * mpeg2dec)
{
    uint8_t * buffer = mpeg2dec->chunk_buffer;
    sequence_t * sequence = &(mpeg2dec->sequence);
    decoder_t * decoder = &(mpeg2dec->decoder);
    static unsigned int frame_period[9] = {
	0, 1126125, 1125000, 1080000, 900900, 900000, 540000, 450450, 450000
    };
    int width, height;
    int i;

    if ((buffer[6] & 0x20) != 0x20)
	return 1;	/* missing marker_bit */

    i = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
    sequence->display_width = sequence->picture_width = width = i >> 12;
    sequence->display_height = sequence->picture_height = height = i & 0xfff;
    decoder->width = sequence->width = width = (width + 15) & ~15;
    decoder->height = sequence->height = height = (height + 15) & ~15;
    sequence->chroma_width = width >> 1;
    sequence->chroma_height = height >> 1;

    sequence->flags = SEQ_FLAG_PROGRESSIVE_SEQUENCE;

    switch (buffer[3] >> 4) {
    case 0:	case 15:	/* illegal */
	sequence->pixel_width = sequence->pixel_height = 0;		break;
    case 1:	/* square pixels */
	sequence->flags |= SEQ_FLAG_SQUARE_PIXEL;
	sequence->pixel_width = sequence->pixel_height = 1;		break;
    case 3:	/* 720x576 16:9 */
	sequence->pixel_width = 64;	sequence->pixel_height = 45;	break;
    case 6:	/* 720x480 16:9 */
	sequence->pixel_width = 32;	sequence->pixel_height = 27;	break;
    case 12:	/* 720*480 4:3 */
	sequence->pixel_width = 8;	sequence->pixel_height = 9;	break;
    default:
	sequence->pixel_width = 2000;
	sequence->pixel_height = 88 * (buffer[3] >> 4) + 1171;
	simplify (&(sequence->pixel_width), &(sequence->pixel_height));
    }

    sequence->frame_period = 0;
    if ((buffer[3] & 15) < 9)
	sequence->frame_period = frame_period[buffer[3] & 15];

    sequence->byte_rate = 50 * ((buffer[4]<<10)|(buffer[5]<<2)|(buffer[6]>>6));
    if (sequence->byte_rate == 50 * 0x3ffff)
	sequence->byte_rate = 0;	/* mpeg-1 VBR */

    sequence->vbv_buffer_size = ((buffer[6]<<16)|(buffer[7]<<8))&0x1ff800;

    if (buffer[7] & 4)
	sequence->flags |= SEQ_FLAG_CONSTRAINED_PARAMETERS;

    if (buffer[7] & 2) {
	for (i = 0; i < 64; i++)
	    decoder->intra_quantizer_matrix[mpeg2_scan_norm[i]] =
		(buffer[i+7] << 7) | (buffer[i+8] >> 1);
	buffer += 64;
    } else
	for (i = 0; i < 64; i++)
	    decoder->intra_quantizer_matrix[mpeg2_scan_norm[i]] =
		default_intra_quantizer_matrix [i];

    if (buffer[7] & 1)
	for (i = 0; i < 64; i++)
	    decoder->non_intra_quantizer_matrix[mpeg2_scan_norm[i]] =
		buffer[i+8];
    else
	for (i = 0; i < 64; i++)
	    decoder->non_intra_quantizer_matrix[i] = 16;

    sequence->profile_level_id = 0x80;
    sequence->colour_primaries = 1;
    sequence->transfer_characteristics = 1;
    sequence->matrix_coefficients = 1;

    decoder->mpeg1 = 1;
    decoder->intra_dc_precision = 0;
    decoder->frame_pred_frame_dct = 1;
    decoder->q_scale_type = 0;
    decoder->concealment_motion_vectors = 0;
    decoder->scan = mpeg2_scan_norm;
    decoder->picture_structure = FRAME_PICTURE;
    decoder->second_field = 0;

    mpeg2dec->ext_state = SEQ_EXT | USER_DATA;
    mpeg2dec->state = STATE_SEQUENCE;

    mpeg2dec->info.sequence = sequence;
    reset_info (&(mpeg2dec->info));

    return 0;
}

static int sequence_ext (mpeg2dec_t * mpeg2dec)
{
    uint8_t * buffer = mpeg2dec->chunk_buffer;
    sequence_t * sequence = &(mpeg2dec->sequence);
    decoder_t * decoder = &(mpeg2dec->decoder);
    int width, height;
    uint32_t flags;

    if (((buffer[0] & 0xf0) != 0x10) || !(buffer[3] & 1))
	return 1;

    sequence->profile_level_id = (buffer[0] << 4) | (buffer[1] >> 4);

    width = sequence->display_width = sequence->picture_width +=
	((buffer[1] << 13) | (buffer[2] << 5)) & 0x3000;
    height = sequence->display_height = sequence->picture_height +=
	(buffer[2] << 7) & 0x3000;
    do {
	switch (sequence->pixel_height) {
	case 1:		/* square pixels */
	    continue;
	case 1347:	/* 4:3 aspect ratio */
	    sequence->pixel_width = 4 * height;
	    sequence->pixel_height = 3 * width;
	    break;
	case 45:	/* 16:9 aspect ratio */
	    sequence->pixel_width = 16 * height;
	    sequence->pixel_height = 9 * width;
	    break;
	case 1523:	/* 2.21:1 aspect ratio */
	    sequence->pixel_width = 221 * height;
	    sequence->pixel_height = 100 * width;
	    break;
	default:
	    sequence->pixel_width = sequence->pixel_height = 0;
	    continue;
	}
	simplify (&(sequence->pixel_width), &(sequence->pixel_height));
    } while (0);
    flags = sequence->flags | SEQ_FLAG_MPEG2;
    decoder->progressive_sequence = buffer[1] >> 3;
    if (!(decoder->progressive_sequence)) {
	flags &= ~SEQ_FLAG_PROGRESSIVE_SEQUENCE;
	height = (height + 31) & ~31;
    }
    if (buffer[5] & 0x80)
	flags |= SEQ_FLAG_LOW_DELAY;
    sequence->flags = flags;
    decoder->width = sequence->width = width = (width + 15) & ~15;
    decoder->height = sequence->height = height = (height + 15) & ~15;
    switch (buffer[1] & 6) {
    case 0:	/* invalid */
	return 1;
    case 2:	/* 4:2:0 */
	height >>= 1;
    case 4:	/* 4:2:2 */
	width >>= 1;
    }
    sequence->chroma_width = width;
    sequence->chroma_height = height;


    if (!(sequence->byte_rate))
	sequence->byte_rate = 50 * 0x3ffff;
    sequence->byte_rate += 50 * (1<<17) * (((buffer[2]<<8)|buffer[3])&0x1ffe);

    sequence->vbv_buffer_size |= buffer[4] << 21;

    sequence->frame_period =
	sequence->frame_period * ((buffer[5]&31)+1) / (((buffer[5]>>2)&3)+1);

    decoder->mpeg1 = 0;

    mpeg2dec->ext_state = SEQ_DISPLAY_EXT | USER_DATA;

    return 0;
}

static int sequence_display_ext (mpeg2dec_t * mpeg2dec)
{
    uint8_t * buffer = mpeg2dec->chunk_buffer;
    sequence_t * sequence = &(mpeg2dec->sequence);
    uint32_t flags;

    if ((buffer[0] & 0xf0) != 0x20)
	return 1;

    flags = ((sequence->flags & ~SEQ_MASK_VIDEO_FORMAT) |
	     ((buffer[0]<<4) & SEQ_MASK_VIDEO_FORMAT));
    if (buffer[0] & 1) {
	flags |= SEQ_FLAG_COLOUR_DESCRIPTION;
	sequence->colour_primaries = buffer[1];
	sequence->transfer_characteristics = buffer[2];
	sequence->matrix_coefficients = buffer[3];
	buffer += 3;
    }

    if (!(buffer[2] & 2))
	return 1;	/* missing marker_bit */

    sequence->display_width = (buffer[1] << 6) | (buffer[2] >> 2);
    sequence->display_height =
	((buffer[2]& 1 ) << 13) | (buffer[3] << 5) | (buffer[4] >> 3);

    if (!(sequence->flags & SEQ_FLAG_SQUARE_PIXEL)) {
	sequence->pixel_width *= sequence->picture_width;
	sequence->pixel_height *= sequence->picture_height;
	simplify (&(sequence->pixel_width), &(sequence->pixel_height));
	sequence->pixel_width *= sequence->display_height;
	sequence->pixel_height *= sequence->display_width;
	simplify (&(sequence->pixel_width), &(sequence->pixel_height));
    }

    return 0;
}

int mpeg2_header_gop (mpeg2dec_t * mpeg2dec)
{
    mpeg2dec->state = STATE_GOP;
    mpeg2dec->ext_state = USER_DATA;
    reset_info (&(mpeg2dec->info));
    return 0;
}

int mpeg2_header_picture (mpeg2dec_t * mpeg2dec)
{
    uint8_t * buffer = mpeg2dec->chunk_buffer;
    picture_t * picture = mpeg2dec->picture;
    decoder_t * decoder = &(mpeg2dec->decoder);
    int type;
    int low_delay;

    type = (buffer [1] >> 3) & 7;
    low_delay = mpeg2dec->sequence.flags & SEQ_FLAG_LOW_DELAY;

    if (mpeg2dec->state != STATE_SLICE_1ST) {
	picture_t * other;

	mpeg2dec->state = STATE_PICTURE;
	decoder->second_field = 0;
	if (decoder->coding_type != PIC_FLAG_CODING_TYPE_B) {
	    mpeg2dec->fbuf[2] = mpeg2dec->fbuf[1];
	    mpeg2dec->fbuf[1] = mpeg2dec->fbuf[0];
	}
	if ((decoder->coding_type != PIC_FLAG_CODING_TYPE_B) ^
	    (picture < mpeg2dec->pictures + 2)) {
	    picture = mpeg2dec->pictures + 2;
	    other = mpeg2dec->pictures;
	} else {
	    picture = mpeg2dec->pictures;
	    other = mpeg2dec->pictures + 2;
	}
	reset_info (&(mpeg2dec->info));
	mpeg2dec->info.current_picture = picture;
	mpeg2dec->info.display_picture = picture;
	if (type != PIC_FLAG_CODING_TYPE_B) {
	    if (!low_delay) {
		if (mpeg2dec->first) {
		    mpeg2dec->info.display_picture = NULL;
		    mpeg2dec->first = 0;
		} else {
		    mpeg2dec->info.display_picture = other;
		    if (other->nb_fields == 1)
			mpeg2dec->info.display_picture_2nd = other + 1;
		    mpeg2dec->info.display_fbuf = mpeg2dec->fbuf + 1;
		}
	    }
	    if (!low_delay + !mpeg2dec->convert_start)
		mpeg2dec->info.discard_fbuf =
		    mpeg2dec->fbuf + !low_delay + !mpeg2dec->convert_start;
	}
    } else {
	mpeg2dec->state = STATE_PICTURE_2ND;
	decoder->second_field = 1;
	picture++;	/* second field picture */
	mpeg2dec->info.current_picture_2nd = picture;
	if (low_delay || type == PIC_FLAG_CODING_TYPE_B)
	    mpeg2dec->info.display_picture_2nd = picture;
    }
    mpeg2dec->ext_state = PIC_CODING_EXT | USER_DATA;
    mpeg2dec->picture = picture;

    picture->temporal_reference = (buffer[0] << 2) | (buffer[1] >> 6);

    decoder->coding_type = picture->flags = type;

    if (type == PIC_FLAG_CODING_TYPE_P || type == PIC_FLAG_CODING_TYPE_B) {
	/* forward_f_code and backward_f_code - used in mpeg1 only */
	decoder->f_motion.f_code[1] = (buffer[3] >> 2) & 1;
	decoder->f_motion.f_code[0] =
	    (((buffer[3] << 1) | (buffer[4] >> 7)) & 7) - 1;
	decoder->b_motion.f_code[1] = (buffer[4] >> 6) & 1;
	decoder->b_motion.f_code[0] = ((buffer[4] >> 3) & 7) - 1;
    }

    /* XXXXXX decode extra_information_picture as well */

    picture->nb_fields = 2;


    return 0;
}

static int picture_coding_ext (mpeg2dec_t * mpeg2dec)
{
    uint8_t * buffer = mpeg2dec->chunk_buffer;
    picture_t * picture = mpeg2dec->picture;
    decoder_t * decoder = &(mpeg2dec->decoder);
    uint32_t flags;

    if ((buffer[0] & 0xf0) != 0x80)
	return 1;

    /* pre subtract 1 for use later in compute_motion_vector */
    decoder->f_motion.f_code[0] = (buffer[0] & 15) - 1;
    decoder->f_motion.f_code[1] = (buffer[1] >> 4) - 1;
    decoder->b_motion.f_code[0] = (buffer[1] & 15) - 1;
    decoder->b_motion.f_code[1] = (buffer[2] >> 4) - 1;

    flags = picture->flags;
    decoder->intra_dc_precision = (buffer[2] >> 2) & 3;
    decoder->picture_structure = buffer[2] & 3;
    switch (decoder->picture_structure) {
    case TOP_FIELD:
	flags |= PIC_FLAG_TOP_FIELD_FIRST;
    case BOTTOM_FIELD:
	picture->nb_fields = 1;
	break;
    case FRAME_PICTURE:
	if (!(decoder->progressive_sequence)) {
	    picture->nb_fields = (buffer[3] & 2) ? 3 : 2;
	    flags |= (buffer[3] & 128) ? PIC_FLAG_TOP_FIELD_FIRST : 0;
	} else
	    picture->nb_fields = (buffer[3]&2) ? ((buffer[3]&128) ? 6 : 4) : 2;
	break;
    default:
	return 1;
    }
    decoder->top_field_first = buffer[3] >> 7;
    decoder->frame_pred_frame_dct = (buffer[3] >> 6) & 1;
    decoder->concealment_motion_vectors = (buffer[3] >> 5) & 1;
    decoder->q_scale_type = (buffer[3] >> 4) & 1;
    decoder->intra_vlc_format = (buffer[3] >> 3) & 1;
    decoder->scan = (buffer[3] & 4) ? mpeg2_scan_alt : mpeg2_scan_norm;
    flags |= (buffer[4] & 0x80) ? PIC_FLAG_PROGRESSIVE_FRAME : 0;
    if (buffer[4] & 0x40)
	flags |= (((buffer[4]<<26) | (buffer[5]<<18) | (buffer[6]<<10)) &
		  PIC_MASK_COMPOSITE_DISPLAY) | PIC_FLAG_COMPOSITE_DISPLAY;
    picture->flags = flags;

    mpeg2dec->ext_state =
	PIC_DISPLAY_EXT | COPYRIGHT_EXT | QUANT_MATRIX_EXT | USER_DATA;

    return 0;
}

static int picture_display_ext (mpeg2dec_t * mpeg2dec)
{
    return 0;
}

static int copyright_ext (mpeg2dec_t * mpeg2dec)
{
    return 0;
}

static int quant_matrix_ext (mpeg2dec_t * mpeg2dec)
{
    uint8_t * buffer = mpeg2dec->chunk_buffer;
    decoder_t * decoder = &(mpeg2dec->decoder);
    int i;

    if (buffer[0] & 8) {
	for (i = 0; i < 64; i++)
	    decoder->intra_quantizer_matrix[mpeg2_scan_norm[i]] =
		(buffer[i] << 5) | (buffer[i+1] >> 3);
	buffer += 64;
    }

    if (buffer[0] & 4)
	for (i = 0; i < 64; i++)
	    decoder->non_intra_quantizer_matrix[mpeg2_scan_norm[i]] =
		(buffer[i] << 6) | (buffer[i+1] >> 2);

    return 0;
}

int mpeg2_header_extension (mpeg2dec_t * mpeg2dec)
{
    static int (* parser[]) (mpeg2dec_t *) = {
	0, sequence_ext, sequence_display_ext, quant_matrix_ext,
	copyright_ext, 0, 0, picture_display_ext, picture_coding_ext
    };
    int ext, ext_bit;

    ext = mpeg2dec->chunk_buffer[0] >> 4;
    ext_bit = 1 << ext;

    if (!(mpeg2dec->ext_state & ext_bit))
	return 1;	/* illegal extension */
    mpeg2dec->ext_state &= ~ext_bit;
    return parser[ext] (mpeg2dec);
}

int mpeg2_header_user_data (mpeg2dec_t * mpeg2dec)
{
    if (!(mpeg2dec->ext_state & USER_DATA))
	return 1;
    mpeg2dec->ext_state &= ~USER_DATA;
    return 0;
}

void mpeg2_header_slice (mpeg2dec_t * mpeg2dec)
{
    if (mpeg2dec->state == STATE_PICTURE) {
	mpeg2dec->info.current_fbuf = mpeg2dec->fbuf;
	if (mpeg2dec->decoder.coding_type == B_TYPE) {
	    mpeg2dec->info.display_fbuf = mpeg2dec->fbuf;
	    mpeg2dec->info.discard_fbuf = mpeg2dec->fbuf;
	} else if (mpeg2dec->sequence.flags & SEQ_FLAG_LOW_DELAY) {
	    mpeg2dec->info.display_fbuf = mpeg2dec->fbuf;
	    if (mpeg2dec->convert_start)
		mpeg2dec->info.discard_fbuf = mpeg2dec->fbuf;
	}
    }

    mpeg2dec->state = ((mpeg2dec->picture->nb_fields > 1 ||
			mpeg2dec->state == STATE_PICTURE_2ND) ?
		       STATE_SLICE : STATE_SLICE_1ST);

    if (mpeg2dec->convert_start) {
	mpeg2dec->decoder.convert = mpeg2dec->convert_copy;
	mpeg2dec->decoder.fbuf_id = mpeg2dec->convert_id;

	mpeg2dec->convert_start (mpeg2dec->convert_id, mpeg2dec->decoder.width,
				 mpeg2dec->fbuf->buf, 0);

	if (mpeg2dec->decoder.coding_type == B_TYPE)
	    mpeg2_init_fbuf (&(mpeg2dec->decoder), mpeg2dec->yuv_buf[2],
			     mpeg2dec->yuv_buf[mpeg2dec->yuv_index ^ 1],
			     mpeg2dec->yuv_buf[mpeg2dec->yuv_index]);
	else {
	    mpeg2_init_fbuf (&(mpeg2dec->decoder),
			     mpeg2dec->yuv_buf[mpeg2dec->yuv_index ^ 1],
			     mpeg2dec->yuv_buf[mpeg2dec->yuv_index],
			     mpeg2dec->yuv_buf[mpeg2dec->yuv_index]);
	    if (mpeg2dec->state == STATE_SLICE)
		mpeg2dec->yuv_index ^= 1;
	}
    } else {
	fbuf_t * backward_fbuf;
	fbuf_t * forward_fbuf;

	mpeg2dec->decoder.convert =
	    (void *)(((vo_frame_t *)(mpeg2dec->fbuf->id))->copy);
	mpeg2dec->decoder.fbuf_id = mpeg2dec->fbuf->id;

	backward_fbuf =
	    mpeg2dec->fbuf + (mpeg2dec->decoder.coding_type == B_TYPE);
	forward_fbuf = backward_fbuf + 1;
	mpeg2_init_fbuf (&(mpeg2dec->decoder), mpeg2dec->fbuf->buf,
			 forward_fbuf->buf, backward_fbuf->buf);
    }
}

void mpeg2_header_end (mpeg2dec_t * mpeg2dec)
{
    picture_t * picture;
    fbuf_t * fbuf;

    picture = mpeg2dec->pictures;
    if (mpeg2dec->picture < picture + 2)
	picture = mpeg2dec->pictures + 2;

    fbuf = mpeg2dec->fbuf + (mpeg2dec->decoder.coding_type ==
			     PIC_FLAG_CODING_TYPE_B);
    mpeg2dec->state = STATE_INVALID;
    reset_info (&(mpeg2dec->info));
    if (!(mpeg2dec->sequence.flags & SEQ_FLAG_LOW_DELAY)) {
	mpeg2dec->info.display_picture = picture;
	if (picture->nb_fields == 1)
	    mpeg2dec->info.display_picture_2nd = picture + 1;
	mpeg2dec->info.display_fbuf = fbuf;
	if (!mpeg2dec->convert_start)
	    mpeg2dec->info.discard_fbuf = fbuf + 1;
    } else if (!mpeg2dec->convert_start)
	mpeg2dec->info.discard_fbuf = fbuf;
}
