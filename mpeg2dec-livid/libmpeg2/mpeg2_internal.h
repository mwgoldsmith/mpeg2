/*
 * mpeg2_internal.h
 *
 * Copyright (C) Aaron Holtzman <aholtzma@ess.engr.uvic.ca> - Nov 1999
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 *	
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Make; see the file COPYING. If not, write to
 * the Free Software Foundation, 
 *
 */

// macroblock modes
#define MACROBLOCK_INTRA 1
#define MACROBLOCK_PATTERN 2
#define MACROBLOCK_MOTION_BACKWARD 4
#define MACROBLOCK_MOTION_FORWARD 8
#define MACROBLOCK_QUANT 16
#define DCT_TYPE_INTERLACED 32
// motion_type
#define MOTION_TYPE_MASK (3*64)
#define MOTION_TYPE_BASE 64
#define MC_FIELD (1*64)
#define MC_FRAME (2*64)
#define MC_16X8 (2*64)
#define MC_DMV (3*64)

//picture structure
#define TOP_FIELD 1
#define BOTTOM_FIELD 2
#define FRAME_PICTURE 3

//picture coding type
#define I_TYPE 1
#define P_TYPE 2
#define B_TYPE 3
#define D_TYPE 4

//use gcc attribs to align critical data structures
#define ALIGN_16_BYTE __attribute__ ((aligned (16)))

//The picture struct contains all of the top level state
//information (ie everything except slice and macroblock
//state)
typedef struct picture_s {
    //-- sequence header stuff --
    uint8_t *intra_quantizer_matrix;
    uint8_t *non_intra_quantizer_matrix;

    uint8_t custom_intra_quantization_matrix[64];
    uint8_t custom_non_intra_quantization_matrix[64];

    //The width and height of the picture snapped to macroblock units
    uint32_t coded_picture_width;
    uint32_t coded_picture_height;

    //-- picture header stuff --

    //what type of picture this is (I,P,or B) D from MPEG-1 isn't supported
    uint32_t picture_coding_type;
	
    //-- picture coding extension stuff --
	
    //quantization factor for motion vectors
    uint8_t f_code[2][2];
    //quantization factor for intra dc coefficients
    uint16_t intra_dc_precision;
    //bool to indicate all predictions are frame based
    uint16_t frame_pred_frame_dct;
    //bool to indicate whether intra blocks have motion vectors 
    // (for concealment)
    uint16_t concealment_motion_vectors;
    //bit to indicate which quantization table to use
    uint16_t q_scale_type;
    //bool to use different vlc tables
    uint16_t intra_vlc_format;

    //last macroblock in the picture
    uint32_t last_mba;
    //width of picture in macroblocks
    uint32_t mb_width;

    //stuff derived from bitstream

    //pointer to the zigzag scan we're supposed to be using
    const uint8_t *scan;

    //Pointer to the current planar frame buffer (Y,Cr,CB)
    uint8_t *current_frame[3];
    //storage for reference frames plus a b-frame
    uint8_t *forward_reference_frame[3];
    uint8_t *backward_reference_frame[3];
    uint8_t *throwaway_frame[3];

    //these things are not needed by the decoder
    //NOTICE : this is a temporary interface, we will build a better one later.
    uint16_t aspect_ratio_information;
    uint16_t frame_rate_code;
    uint16_t progressive_sequence;
    uint16_t top_field_first;
    uint16_t repeat_first_field;
    uint16_t progressive_frame;
} picture_t;

typedef struct motion_s {
    uint8_t * ref_frame[3];
    int16_t pmv[2][2];
    uint8_t f_code[2];
} motion_t;

// state that is carried from one macroblock to the next inside of a same slice
typedef struct slice_s {
    //Motion vectors
    //The f_ and b_ correspond to the forward and backward motion
    //predictors
    motion_t b_motion;
    motion_t f_motion;

    // predictor for DC coefficients in intra blocks
    int16_t dc_dct_pred[3];

    uint16_t quantizer_scale;	// remove
} slice_t;

//The only global variable,
//the config struct
extern mpeg2_config_t config;




//FIXME remove
int Get_Luma_DC_dct_diff (void);
int Get_Chroma_DC_dct_diff (void);
int Get_macroblock_type (int picture_coding_type);
int Get_motion_code (void);
int Get_coded_block_pattern (void);
int Get_macroblock_address_increment (void);
