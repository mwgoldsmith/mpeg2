/*
 *  slice.c
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

#include "config.h"
#include "mpeg2.h"
#include "mpeg2_internal.h"

#include "bitstream.h"
#include "stats.h"

static const uint_8 default_intra_quantization_matrix[64] ALIGN_16_BYTE = 
{
   8, 16, 19, 22, 26, 27, 29, 34,
  16, 16, 22, 24, 27, 29, 34, 37,
  19, 22, 26, 27, 29, 34, 34, 38,
  22, 22, 26, 27, 29, 34, 37, 40,
  22, 26, 27, 29, 32, 35, 40, 48,
  26, 27, 29, 32, 35, 40, 48, 58,
  26, 27, 29, 34, 38, 46, 56, 69,
  27, 29, 35, 38, 46, 56, 69, 83
};

static const uint_8 default_non_intra_quantization_matrix[64] ALIGN_16_BYTE = 
{
	16, 16, 16, 16, 16, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 16, 16, 
	16, 16, 16, 16, 16, 16, 16, 16 
};

#ifdef __i386__
static const uint_8 scan_norm_mmx[64] ALIGN_16_BYTE  =
{ 
	// MMX Zig-Zag scan pattern (transposed)  
	0,  8, 1,  2, 9,16,24,17,
	10, 3, 4, 11,18,25,32,40,
	33,26,19,12, 5, 6,13,20,
	27,34,41,48,56,49,42,35,
	28,21,14, 7,15,22,29,36,
	43,50,57,58,51,44,37,30,
	23,31,38,45,52,59,60,53,
	46,39,47,54,61,62,55,63
};

static const uint_8 scan_alt_mmx[64] ALIGN_16_BYTE = 
{ 
	// Alternate scan pattern (transposed)
	0, 1, 2, 3, 8, 9,16,17,
	10,11, 4, 5, 6, 7,15,14,
	13,12,19,18,24,25,32,33,
	26,27,20,21,22,23,28,29,
	30,31,34,35,40,41,48,49,
	42,43,36,37,38,39,44,45,
	46,47,50,51,56,57,58,59,
	52,53,54,55,60,61,62,63,
};
#endif

static const uint_8 scan_norm[64] ALIGN_16_BYTE =
{ 
	// Zig-Zag scan pattern
	 0, 1, 8,16, 9, 2, 3,10,
	17,24,32,25,18,11, 4, 5,
	12,19,26,33,40,48,41,34,
	27,20,13, 6, 7,14,21,28,
	35,42,49,56,57,50,43,36,
	29,22,15,23,30,37,44,51,
	58,59,52,45,38,31,39,46,
	53,60,61,54,47,55,62,63
};

static const uint_8 scan_alt[64] ALIGN_16_BYTE =
{ 
	// Alternate scan pattern 
	0,8,16,24,1,9,2,10,17,25,32,40,48,56,57,49,
	41,33,26,18,3,11,4,12,19,27,34,42,50,58,35,43,
	51,59,20,28,5,13,6,14,21,29,36,44,52,60,37,45,
	53,61,22,30,7,15,23,31,38,46,54,62,39,47,55,63
};

static void header_process_sequence_display_extension(picture_t *picture);
static void header_process_sequence_extension(picture_t *picture);
static void header_process_picture_coding_extension(picture_t *picture);

void
header_state_init(picture_t *picture)
{
	picture->intra_quantizer_matrix = (uint_8*) default_intra_quantization_matrix;
	picture->non_intra_quantizer_matrix = (uint_8*) default_non_intra_quantization_matrix;
	//FIXME we should set pointers to the real scan matrices
	//here (mmx vs normal) instead of the ifdefs in header_process_picture_coding_extension

#ifdef __i386__
	if(config.flags & MPEG2_MMX_ENABLE)
		picture->scan = scan_norm_mmx;
	else
#endif
		picture->scan = scan_norm;

}

//FIXME remove once we get everything working
void header_get_marker_bit(char *string)
{
  int marker;

  marker = bitstream_get(1);

  if(!marker)
    fprintf(stderr,"(header) %s marker_bit set to 0\n",string);
}


void 
header_process_sequence_header(picture_t *picture)
{
	uint_32 i;

  picture->horizontal_size             = bitstream_get(12);
  picture->vertical_size               = bitstream_get(12);

	//XXX this needs field fixups
	picture->coded_picture_height = ((picture->vertical_size + 15)/16) * 16;
	picture->coded_picture_width  = ((picture->horizontal_size   + 15)/16) * 16;
	picture->last_mba = ((picture->coded_picture_height * picture->coded_picture_width) >> 8) - 1;

  picture->aspect_ratio_information    = bitstream_get(4);
  picture->frame_rate_code             = bitstream_get(4);
  picture->bit_rate_value              = bitstream_get(18);
  header_get_marker_bit("sequence_header");
  picture->vbv_buffer_size             = bitstream_get(10);
  picture->constrained_parameters_flag = bitstream_get(1);

  if((picture->use_custom_intra_quantizer_matrix = bitstream_get(1)))
  {
		picture->intra_quantizer_matrix = picture->custom_intra_quantization_matrix;
    for (i=0; i < 64; i++)
      picture->custom_intra_quantization_matrix[scan_norm[i]] = bitstream_get(8);
  }
	else
		picture->intra_quantizer_matrix = (uint_8*) default_intra_quantization_matrix;

  if((picture->use_custom_non_intra_quantizer_matrix = bitstream_get(1)))
  {
		picture->non_intra_quantizer_matrix = picture->custom_non_intra_quantization_matrix;
    for (i=0; i < 64; i++)
      picture->custom_non_intra_quantization_matrix[scan_norm[i]] = bitstream_get(8);
  }
	else
		picture->non_intra_quantizer_matrix = (uint_8*) default_non_intra_quantization_matrix;

	stats_sequence_header(picture);

}

void
header_process_extension(picture_t *picture)
{
	uint_32 code;

	code = bitstream_get(4);
		
	switch(code)
	{
		case PICTURE_CODING_EXTENSION_ID:
			header_process_picture_coding_extension(picture);
			break;

		case SEQUENCE_EXTENSION_ID:
			header_process_sequence_extension(picture);
			break;

		case SEQUENCE_DISPLAY_EXTENSION_ID:
			header_process_sequence_display_extension(picture);
			break;

		default:
			fprintf (stderr,"(header) unsupported extension %x\n",code);
			fprintf (stderr, "EXIT\n");
	}
}

void
header_process_user_data(void)
{
  while (bitstream_show(24)!=0x01L)
    bitstream_flush(8);
}

static void
header_process_sequence_extension(picture_t *picture)
{

  /*picture->profile_and_level_indication = */ bitstream_get(8);
  picture->progressive_sequence           =    bitstream_get(1);
  picture->chroma_format                  =    bitstream_get(2);
  /*picture->horizontal_size_extension    = */ bitstream_get(2);
  /*picture->vertical_size_extension      = */ bitstream_get(2);
  /*picture->bit_rate_extension           = */ bitstream_get(12);
  header_get_marker_bit("sequence_extension");
  /*picture->vbv_buffer_size_extension    = */ bitstream_get(8);
  /*picture->low_delay                    = */ bitstream_get(1);
  /*picture->frame_rate_extension_n       = */ bitstream_get(2);
  /*picture->frame_rate_extension_d       = */ bitstream_get(5);

	stats_sequence_ext(picture);
	//
	//XXX since we don't support anything but 4:2:0, die gracefully
	if(picture->chroma_format != CHROMA_420)
	{
		fprintf(stderr,"(parse) sorry, mpeg2dec doesn't support color formats other than 4:2:0\n");
		fprintf (stderr, "EXIT\n");
	}
}


static void
header_process_sequence_display_extension(picture_t *picture)
{
  picture->video_format      = bitstream_get(3);
  picture->color_description = bitstream_get(1);

  if (picture->color_description)
  {
    picture->color_primaries          = bitstream_get(8);
    picture->transfer_characteristics = bitstream_get(8);
    picture->matrix_coefficients      = bitstream_get(8);
  }

  picture->display_horizontal_size = bitstream_get(14);
  header_get_marker_bit("sequence_display_extension");
  picture->display_vertical_size   = bitstream_get(14);

	stats_sequence_display_ext(picture);
}

void 
header_process_gop_header(picture_t *picture)
{
  picture->drop_flag   = bitstream_get(1);
  picture->hour        = bitstream_get(5);
  picture->minute      = bitstream_get(6);
  header_get_marker_bit("gop_header");
  picture->sec         = bitstream_get(6);
  picture->frame       = bitstream_get(6);
  picture->closed_gop  = bitstream_get(1);
  picture->broken_link = bitstream_get(1);

	stats_gop_header(picture);

}


static void 
header_process_picture_coding_extension(picture_t *picture)
{
	//pre subtract 1 for use later in compute_motion_vector
  picture->f_code[0][0] = bitstream_get(4) - 1;
  picture->f_code[0][1] = bitstream_get(4) - 1;
  picture->f_code[1][0] = bitstream_get(4) - 1;
  picture->f_code[1][1] = bitstream_get(4) - 1;

  picture->intra_dc_precision         = bitstream_get(2);
  picture->picture_structure          = bitstream_get(2);
  picture->top_field_first            = bitstream_get(1);
  picture->frame_pred_frame_dct       = bitstream_get(1);
  picture->concealment_motion_vectors = bitstream_get(1);
  picture->q_scale_type               = bitstream_get(1);
  picture->intra_vlc_format           = bitstream_get(1);
  picture->alternate_scan             = bitstream_get(1);

#ifdef __i386__
	if(config.flags & MPEG2_MMX_ENABLE)
	{
		if(picture->alternate_scan)
			picture->scan = scan_alt_mmx;
		else
			picture->scan = scan_norm_mmx;
	}
	else
#endif
	{
		if(picture->alternate_scan)
			picture->scan = scan_alt;
		else
			picture->scan = scan_norm;
	}

  picture->repeat_first_field         = bitstream_get(1);
	/*chroma_420_type isn't used */       bitstream_get(1);
  picture->progressive_frame          = bitstream_get(1);
  picture->composite_display_flag     = bitstream_get(1);

  if (picture->composite_display_flag)
  {
		//This info is not used in the decoding process
    /* picture->v_axis            = */ bitstream_get(1);
    /* picture->field_sequence    = */ bitstream_get(3);
    /* picture->sub_carrier       = */ bitstream_get(1);
    /* picture->burst_amplitude   = */ bitstream_get(7);
    /* picture->sub_carrier_phase = */ bitstream_get(8);
  }

	//XXX die gracefully if we encounter a field picture based stream
	if(picture->picture_structure != FRAME_PICTURE)
	{
		fprintf(stderr,"(parse) sorry, mpeg2dec doesn't support field based pictures yet\n");
		fprintf (stderr, "EXIT\n");
	}

	stats_picture_coding_ext_header(picture);
}

void 
header_process_picture_header(picture_t *picture)
{
  picture->temporal_reference  = bitstream_get(10);
  picture->picture_coding_type = bitstream_get(3);
  picture->vbv_delay           = bitstream_get(16);

  if (picture->picture_coding_type==P_TYPE || picture->picture_coding_type==B_TYPE)
  {
    picture->full_pel_forward_vector = bitstream_get(1);
    picture->forward_f_code = bitstream_get(3);
  }
  if (picture->picture_coding_type==B_TYPE)
  {
    picture->full_pel_backward_vector = bitstream_get(1);
    picture->backward_f_code = bitstream_get(3);
  }

	stats_picture_header(picture);
}



