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

#include "video_out.h"
#include "mpeg2.h"
#include "mpeg2_internal.h"
#include "attributes.h"

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

uint8_t mpeg2_scan_norm[64] ATTR_ALIGN(16) =
{
    /* Zig-Zag scan pattern */
     0, 1, 8,16, 9, 2, 3,10,
    17,24,32,25,18,11, 4, 5,
    12,19,26,33,40,48,41,34,
    27,20,13, 6, 7,14,21,28,
    35,42,49,56,57,50,43,36,
    29,22,15,23,30,37,44,51,
    58,59,52,45,38,31,39,46,
    53,60,61,54,47,55,62,63
};

uint8_t mpeg2_scan_alt[64] ATTR_ALIGN(16) =
{
    /* Alternate scan pattern */
    0,8,16,24,1,9,2,10,17,25,32,40,48,56,57,49,
    41,33,26,18,3,11,4,12,19,27,34,42,50,58,35,43,
    51,59,20,28,5,13,6,14,21,29,36,44,52,60,37,45,
    53,61,22,30,7,15,23,31,38,46,54,62,39,47,55,63
};

void mpeg2_header_state_init (decoder_t * decoder)
{
    decoder->scan = mpeg2_scan_norm;
}

int mpeg2_header_sequence (uint8_t * buffer, decoder_t * decoder,
			   sequence_t * sequence)
{
    int width, height;
    int i;

    if ((buffer[6] & 0x20) != 0x20)
	return 1;	/* missing marker_bit */

    height = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
    width = height >> 12;
    height &= 0xfff;
    sequence->display_width = width;
    sequence->display_height = height;

    width = (width + 15) & ~15;
    height = (height + 15) & ~15;

    if ((width > 1920) || (height > 1152))
	return 1;	/* size restrictions for MP@HL */

    decoder->coded_picture_width = sequence->width = width;
    decoder->coded_picture_height = sequence->height = height;

    /* this is not used by the decoder */
    sequence->aspect_ratio = buffer[3] >> 4;
    sequence->frame_rate_code = buffer[3] & 15;
    sequence->bitrate = (buffer[4]<<10)|(buffer[5]<<2)|(buffer[6]>>6);

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

    /* MPEG1 - for testing only */
    decoder->mpeg1 = 1;
    decoder->intra_dc_precision = 0;
    decoder->frame_pred_frame_dct = 1;
    decoder->q_scale_type = 0;
    decoder->concealment_motion_vectors = 0;
    /* decoder->alternate_scan = 0; */
    decoder->picture_structure = FRAME_PICTURE;
    /* decoder->second_field = 0; */

    return 0;
}

static int sequence_extension (uint8_t * buffer, decoder_t * decoder,
			       sequence_t * sequence)
{
    /* check chroma format, size extensions, marker bit */
    if (((buffer[1] & 0x07) != 0x02) || (buffer[2] & 0xe0) ||
	((buffer[3] & 0x01) != 0x01))
	return 1;

    /* this is not used by the decoder */
    sequence->progressive_sequence = (buffer[1] >> 3) & 1;

    if (!(sequence->progressive_sequence))
	decoder->coded_picture_height =
	    (decoder->coded_picture_height + 31) & ~31;

    /* MPEG1 - for testing only */
    decoder->mpeg1 = 0;

    return 0;
}

static int quant_matrix_extension (uint8_t * buffer, decoder_t * decoder)
{
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

static int picture_coding_extension (uint8_t * buffer, decoder_t * decoder,
				     picture_t * picture)
{
    /* pre subtract 1 for use later in compute_motion_vector */
    decoder->f_motion.f_code[0] = (buffer[0] & 15) - 1;
    decoder->f_motion.f_code[1] = (buffer[1] >> 4) - 1;
    decoder->b_motion.f_code[0] = (buffer[1] & 15) - 1;
    decoder->b_motion.f_code[1] = (buffer[2] >> 4) - 1;

    decoder->intra_dc_precision = (buffer[2] >> 2) & 3;
    decoder->picture_structure = buffer[2] & 3;
    decoder->frame_pred_frame_dct = (buffer[3] >> 6) & 1;
    decoder->concealment_motion_vectors = (buffer[3] >> 5) & 1;
    decoder->q_scale_type = (buffer[3] >> 4) & 1;
    decoder->intra_vlc_format = (buffer[3] >> 3) & 1;

    if (buffer[3] & 4)	/* alternate_scan */
	decoder->scan = mpeg2_scan_alt;
    else
	decoder->scan = mpeg2_scan_norm;

    /* these are not used by the decoder */
    decoder->top_field_first = picture->top_field_first = buffer[3] >> 7;
    picture->repeat_first_field = (buffer[3] >> 1) & 1;
    picture->progressive_frame = buffer[4] >> 7;

    return 0;
}

int mpeg2_header_extension (uint8_t * buffer, decoder_t * decoder,
			    mpeg2_info_t * info)
{
    switch (buffer[0] & 0xf0) {
    case 0x10:	/* sequence extension */
	return sequence_extension (buffer, decoder, &(info->sequence));

    case 0x30:	/* quant matrix extension */
	return quant_matrix_extension (buffer, decoder);

    case 0x80:	/* picture coding extension */
	return picture_coding_extension (buffer, decoder, &(info->picture));
    }

    return 0;
}

int mpeg2_header_picture (uint8_t * buffer, decoder_t * decoder,
			  picture_t * picture)
{
    decoder->picture_coding_type = (buffer [1] >> 3) & 7;

    /* forward_f_code and backward_f_code - used in mpeg1 only */
    decoder->f_motion.f_code[1] = (buffer[3] >> 2) & 1;
    decoder->f_motion.f_code[0] =
	(((buffer[3] << 1) | (buffer[4] >> 7)) & 7) - 1;
    decoder->b_motion.f_code[1] = (buffer[4] >> 6) & 1;
    decoder->b_motion.f_code[0] = ((buffer[4] >> 3) & 7) - 1;

    /* move in header_process_picture_header */
        decoder->second_field =
            (decoder->picture_structure != FRAME_PICTURE) &&
            !(decoder->second_field);

    return 0;
}
