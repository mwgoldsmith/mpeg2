/*
 *  stats.c
 *
 *  Copyright (C) Aaron Holtzman <aholtzma@ess.engr.uvic.ca> - Nov 1999
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

#include <stdlib.h>
#include <stdio.h>
#include "mpeg2.h"
#include "mpeg2_internal.h"

#include "debug.h"
#include "stats.h"

char *chroma_format_str[4] =
{
	"Invalid Chroma Format",
	"4:2:0 Chroma",
	"4:2:2 Chroma",
	"4:4:4 Chroma"
};

char *picture_structure_str[4] =
{
	"Invalid Picture Structure",
	"Top field",
	"Bottom field",
	"Frame Picture"
};

char *picture_coding_type_str[8] = 
{
	"Invalid picture type",
	"I-type",
	"P-type",
	"B-type",
	"D (very bad)",
	"Invalid","Invalid","Invalid"
};

char *aspect_ratio_information_str[8] = 
{
	"Invalid Aspect Ratio",
	"1:1",
	"4:3",
	"16:9",
	"2.21:1",
	"Invalid Aspect Ratio",
	"Invalid Aspect Ratio",
	"Invalid Aspect Ratio"
};

char *frame_rate_str[16] = 
{
	"Invalid frame_rate_code",
	"23.976", "24", "25"   , "29.97",
	"30"    , "50", "59.94", "60"   ,
	"Invalid frame_rate_code", "Invalid frame_rate_code",
	"Invalid frame_rate_code", "Invalid frame_rate_code",
	"Invalid frame_rate_code", "Invalid frame_rate_code",
	"Invalid frame_rate_code"
};


void stats_sequence_header(picture_t *picture)
{
	dprintf("(seq) ");
	dprintf("%dx%d ",picture->horizontal_size,picture->vertical_size);
	dprintf("%s, ",aspect_ratio_information_str[picture->aspect_ratio_information]);
	dprintf("%s fps, ",frame_rate_str[picture->frame_rate_code]);
	dprintf("%5.0f kbps, ",picture->bit_rate_value * 400.0 / 1000.0);
	dprintf("VBV %d kB ",2 * picture->vbv_buffer_size);
	dprintf("%s ",picture->constrained_parameters_flag ? ", CP":"");
	dprintf("%s ",picture->use_custom_intra_quantizer_matrix ? ", Custom Intra Matrix":"");
	dprintf("%s ",picture->use_custom_non_intra_quantizer_matrix ? ", Custom Non-Intra Matrix":"");
	dprintf("\n");
}


void stats_gop_header(picture_t *picture)
{
	dprintf("(gop) ");
	dprintf("drop_flag %d, ", picture->drop_flag);
	dprintf("timecode %d:%02d:%02d:%02d, ", picture->hour, picture->minute, 
			picture->sec, picture->frame);
	dprintf("closed_gop %d, ",picture->closed_gop);
	dprintf("broken_link=%d\n",picture->broken_link);
}



void stats_picture_header(picture_t *picture)
{
	dprintf("(picture) %s ",picture_coding_type_str[picture->picture_coding_type]);
	dprintf("temporal_reference %d, ",picture->temporal_reference);
	dprintf("vbv_delay %d\n",picture->vbv_delay);
}



void stats_slice_header(slice_t *slice)
{
	dprintf("(slice) ");
	dprintf("quantizer_scale_code = %d, ",slice->quantizer_scale_code);
	dprintf("quantizer_scale= %d ",slice->quantizer_scale);
	dprintf("\n");
}


void stats_macroblock(macroblock_t *mb)
{
	dprintf("(macroblock) ");
	dprintf("cbp = %d ",mb->coded_block_pattern);
	dprintf("\n");
}


void 
stats_sequence_ext(picture_t *picture)
{
	dprintf("(seq_ext) ");
	dprintf("progressive_sequence %d, ",picture->progressive_sequence);
	dprintf("%s\n",chroma_format_str[picture->chroma_format]);
}


void stats_sequence_display_ext(picture_t *picture)
{
	dprintf("(seq_dsp_ext) ");
	dprintf("video_format %d, color_description %d",
			picture->video_format,picture->color_description);
	dprintf("\n");

	dprintf("(seq dsp ext) ");
	dprintf("display_horizontal_size %4d, display_vertical_size %4d",
			picture->display_horizontal_size,picture->display_vertical_size);
	dprintf("\n");

	if (picture->color_description)
	{
		dprintf("(seq dsp ext) ");
		dprintf("color_primaries %d, ",picture->color_primaries);
		dprintf("xfer_characteristics %d,",picture->transfer_characteristics);
		dprintf("matrix_coefficients %d",picture->matrix_coefficients);
		dprintf("\n");
	}
}



void stats_quant_matrix_ext_header(picture_t *picture)
{
#if 0
	if (Verbose_Flag>NO_LAYER)
	{
		dprintf("quant matrix extension (byte %d)\n",(pos>>3)-4);
		dprintf("  load_intra_quantizer_matrix=%d\n",
				ld->load_intra_quantizer_matrix);
		dprintf("  load_non_intra_quantizer_matrix=%d\n",
				ld->load_non_intra_quantizer_matrix);
		dprintf("  load_chroma_intra_quantizer_matrix=%d\n",
				ld->load_chroma_intra_quantizer_matrix);
		dprintf("  load_chroma_non_intra_quantizer_matrix=%d\n",
				ld->load_chroma_non_intra_quantizer_matrix);
	}
#endif
}

void 
stats_picture_coding_ext_header(picture_t *picture)
{
	dprintf("(pic_ext) ");
	dprintf("%s",picture_structure_str[picture->picture_structure]);
	dprintf("\n");

	dprintf("(pic_ext) ");
	dprintf("forward horizontal  f_code %d,  forward vertical  f_code   %d", 
			picture->f_code[0][0], picture->f_code[0][1]);
	dprintf("\n");

	dprintf("(pic_ext) ");
	dprintf("backward horizontal f_code %d,  backward_vertical f_code   %d", 
			picture->f_code[1][0], picture->f_code[1][1]);
	dprintf("\n");


	dprintf("(pic_ext) ");
	dprintf("frame_pred_frame_dct       %d,  concealment_motion_vectors %d",
			picture->frame_pred_frame_dct,picture->concealment_motion_vectors);
	dprintf("\n");

	dprintf("(pic_ext) ");
	dprintf("progressive_frame          %d,  composite_display_flag     %d",
			picture->progressive_frame, picture->composite_display_flag);
	dprintf("\n");

	dprintf("(pic_ext) ");
	dprintf("intra_dc_precision %d, q_scale_type  %d,    top_field_first %d",
			picture->intra_dc_precision,picture->q_scale_type,picture->top_field_first);
	dprintf("\n");

	dprintf("(pic_ext) ");
	dprintf("alternate_scan     %d, intra_vlc_fmt %d, repeat_first_field %d",
			picture->alternate_scan,picture->intra_vlc_format,picture->repeat_first_field);
	dprintf("\n");
}
