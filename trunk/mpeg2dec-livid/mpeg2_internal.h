/*
 *  mpeg2_internal.h
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

//header start codes
#define PICTURE_START_CODE      0x100
#define SLICE_START_CODE_MIN    0x101
#define SLICE_START_CODE_MAX    0x1AF
#define USER_DATA_START_CODE    0x1B2
#define SEQUENCE_HEADER_CODE    0x1B3
#define SEQUENCE_ERROR_CODE     0x1B4
#define EXTENSION_START_CODE    0x1B5
#define SEQUENCE_END_CODE       0x1B7
#define GROUP_START_CODE        0x1B8
#define SYSTEM_START_CODE_MIN   0x1B9
#define SYSTEM_START_CODE_MAX   0x1FF

#define ISO_END_CODE            0x1B9
#define PACK_START_CODE         0x1BA
#define SYSTEM_START_CODE       0x1BB

#define VIDEO_ELEMENTARY_STREAM 0x1e0

//extension start code ids
#define SEQUENCE_EXTENSION_ID                    1
#define SEQUENCE_DISPLAY_EXTENSION_ID            2
#define QUANT_MATRIX_EXTENSION_ID                3
#define COPYRIGHT_EXTENSION_ID                   4
#define SEQUENCE_SCALABLE_EXTENSION_ID           5
#define PICTURE_DISPLAY_EXTENSION_ID             7
#define PICTURE_CODING_EXTENSION_ID              8
#define PICTURE_SPATIAL_SCALABLE_EXTENSION_ID    9
#define PICTURE_TEMPORAL_SCALABLE_EXTENSION_ID  10


// macroblock type 
#define MACROBLOCK_INTRA                        1
#define MACROBLOCK_PATTERN                      2
#define MACROBLOCK_MOTION_BACKWARD              4
#define MACROBLOCK_MOTION_FORWARD               8
#define MACROBLOCK_QUANT                        16
#define SPATIAL_TEMPORAL_WEIGHT_CODE_FLAG       32
#define PERMITTED_SPATIAL_TEMPORAL_WEIGHT_CLASS 64

//picture structure 
#define TOP_FIELD     1
#define BOTTOM_FIELD  2
#define FRAME_PICTURE 3

//picture coding type 
#define I_TYPE 1
#define P_TYPE 2
#define B_TYPE 3
#define D_TYPE 4

// motion_type 
#define MC_FIELD 1
#define MC_FRAME 2
#define MC_16X8  2
#define MC_DMV   3

// mv_format 
#define MV_FIELD 0
#define MV_FRAME 1

// chroma_format 
#define CHROMA_420 1
#define CHROMA_422 2
#define CHROMA_444 3

//The picture struct contains all of the top level state
//information (ie everything except slice and macroblock
//state)
typedef struct picture_s
{
	//-- sequence header stuff --
	uint_32 horizontal_size;
	uint_32 vertical_size;
	uint_32 aspect_ratio_information;
	uint_32 frame_rate_code;
	uint_32 bit_rate_value;
	uint_32 vbv_buffer_size;
	uint_32 constrained_parameters_flag;
	uint_8 *intra_quantizer_matrix;
	uint_8 *non_intra_quantizer_matrix;

	uint_32 use_custom_intra_quantizer_matrix;
	uint_32 use_custom_non_intra_quantizer_matrix;
	uint_8 custom_intra_quantization_matrix[64];
	uint_8 custom_non_intra_quantization_matrix[64];

	//The width and height of the picture snapped to macroblock units
	uint_32 coded_picture_width;
	uint_32 coded_picture_height;

	//--sequence extension stuff--
	//a lot of the stuff in the sequence extension stuff we dont' care
	//about, so it's not in here
	
	//color format of the sequence (4:2:0, 4:2:2, or 4:4:4)
	uint_16 chroma_format;
	//bool to indicate that only progressive frames are present in the
	//bitstream
	uint_16 progressive_sequence;

	//-- sequence display extension stuff --
	uint_16 video_format;
	uint_16 display_horizontal_size;
	uint_16 display_vertical_size;
  uint_16 color_description;
  uint_16 color_primaries;
  uint_16 transfer_characteristics;
  uint_16 matrix_coefficients;
	
	//-- group of pictures header stuff --
	
	//these are all part of the time code for this gop
	uint_32 drop_flag;
	uint_32 hour;
	uint_32 minute;
	uint_32 sec;
	uint_32 frame;

	//set if the B frames in this gop don't reference outside of the gop
	uint_32 closed_gop;
	//set if the previous I frame is missing (ie don't decode)
	uint_32 broken_link;

	//-- picture header stuff --
	
	//what type of picture this is (I,P,or B) D from MPEG-1 isn't supported
	uint_32 picture_coding_type;	
	uint_32 temporal_reference;	
	uint_32 vbv_delay;	


	//MPEG-1 stuff
	uint_8 full_pel_forward_vector;
	uint_8 forward_f_code;
	uint_8 full_pel_backward_vector;
	uint_8 backward_f_code;
	
	//-- picture coding extension stuff --
	
	//quantization factor for motion vectors
	uint_8 f_code[2][2];
	//quantization factor for intra dc coefficients
	uint_16 intra_dc_precision;
	//what type of picture is this (field or frame)
	uint_16 picture_structure;
	//bool to indicate the top field is first
	uint_16 top_field_first;
	//bool to indicate all predictions are frame based
	uint_16 frame_pred_frame_dct;
	//bool to indicate whether intra blocks have motion vectors 
	//(for concealment)
	uint_16 concealment_motion_vectors;
	//bit to indicate which quantization table to use
	uint_16 q_scale_type;
	//bool to use different vlc tables
	uint_16 intra_vlc_format;
	//bool to use different zig-zag pattern	
	uint_16 alternate_scan;
	//wacky field stuff
	uint_16 repeat_first_field;
	//wacky field stuff
	uint_16 progressive_frame;
	//wacky analog stuff (not used)
	uint_16 composite_display_flag;

	

	//stuff derived from bitstream
	
	//pointer to the zigzag scan we're supposed to be using
	uint_8 *scan;

	//These are pointers to the planar frame buffers (Y,Cr,CB)
	uint_8 *current_frame[3];
	uint_8 *forward_reference_frame[3];
	uint_8 *backward_reference_frame[3];
} picture_t;

typedef struct slice_s
{
  uint_32 slice_vertical_position_extension;
  uint_32 quantizer_scale_code;
  uint_32 slice_picture_id_enable;
  uint_32 slice_picture_id;
  uint_32 extra_information_slice;

	//Motion vectors
	sint_32 pmv[2][2][2];
	sint_32 dc_dct_pred[3];
	uint_32 quantizer_scale;
} slice_t;

typedef struct macroblock_s
{
	sint_16 *y_blocks;
	sint_16 *cr_blocks;
	sint_16 *cb_blocks;

	uint_16 mba;
	uint_16 macroblock_type;
	uint_16 motion_type;
	uint_16 motion_vector_count;
	sint_16 motion_vertical_field_select[2][2];
	sint_16 dmvector[2];
	uint_16 mv_format;
	uint_16 mvscale;
	uint_16 dmv;
	uint_16 dct_type;
	uint_16 coded_block_pattern; 
	uint_16 quantizer_scale_code;

	uint_16 skipped;
} macroblock_t;

//The only global variable,
//the config struct
extern mpeg2_config_t config;
