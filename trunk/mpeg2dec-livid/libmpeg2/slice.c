/*
 * slice.c
 *
 * Copyright (C) Aaron Holtzman <aholtzma@ess.engr.uvic.ca> - Nov 1999
 *
 * Decodes an MPEG-2 video stream.
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
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "mpeg2.h"
#include "mpeg2_internal.h"

#include "motion_comp.h"
#include "bitstream.h"
#include "idct.h"

extern mc_functions_t mc_functions;
extern void (*idct_block_copy) (int16_t * block, uint8_t * dest, int stride);
extern void (*idct_block_add) (int16_t * block, uint8_t * dest, int stride);

//XXX put these on the stack in slice_process?
static slice_t slice;
int16_t DCTblock[64] ALIGN_16_BYTE;

typedef struct {
    char run, level, len;
} DCTtab;

//extern DCTtab DCTtabfirst[],DCTtabnext[],DCTtab0[],DCTtab1[];
//extern DCTtab DCTtab2[],DCTtab3[],DCTtab4[],DCTtab5[],DCTtab6[];
//extern DCTtab DCTtab0a[],DCTtab1a[];

DCTtab DCT_16 [] = {
    { 1,18,16}, { 1,17,16}, { 1,16,16}, { 1,15,16},
    { 6, 3,16}, {16, 2,16}, {15, 2,16}, {14, 2,16},
    {13, 2,16}, {12, 2,16}, {11, 2,16}, {31, 1,16},
    {30, 1,16}, {29, 1,16}, {28, 1,16}, {27, 1,16}
};

DCTtab DCT_15 [] = {
    { 0,40,15}, { 0,39,15}, { 0,38,15}, { 0,37,15},
    { 0,36,15}, { 0,35,15}, { 0,34,15}, { 0,33,15},
    { 0,32,15}, { 1,14,15}, { 1,13,15}, { 1,12,15},
    { 1,11,15}, { 1,10,15}, { 1, 9,15}, { 1, 8,15},
    { 0,31,14}, { 0,31,14}, { 0,30,14}, { 0,30,14},
    { 0,29,14}, { 0,29,14}, { 0,28,14}, { 0,28,14},
    { 0,27,14}, { 0,27,14}, { 0,26,14}, { 0,26,14},
    { 0,25,14}, { 0,25,14}, { 0,24,14}, { 0,24,14},
    { 0,23,14}, { 0,23,14}, { 0,22,14}, { 0,22,14},
    { 0,21,14}, { 0,21,14}, { 0,20,14}, { 0,20,14},
    { 0,19,14}, { 0,19,14}, { 0,18,14}, { 0,18,14},
    { 0,17,14}, { 0,17,14}, { 0,16,14}, { 0,16,14}
};

DCTtab DCT_13 [] = {
    {10, 2,13}, { 9, 2,13}, { 5, 3,13}, { 3, 4,13},
    { 2, 5,13}, { 1, 7,13}, { 1, 6,13}, { 0,15,13},
    { 0,14,13}, { 0,13,13}, { 0,12,13}, {26, 1,13},
    {25, 1,13}, {24, 1,13}, {23, 1,13}, {22, 1,13},
    { 0,11,12}, { 0,11,12}, { 8, 2,12}, { 8, 2,12},
    { 4, 3,12}, { 4, 3,12}, { 0,10,12}, { 0,10,12},
    { 2, 4,12}, { 2, 4,12}, { 7, 2,12}, { 7, 2,12},
    {21, 1,12}, {21, 1,12}, {20, 1,12}, {20, 1,12},
    { 0, 9,12}, { 0, 9,12}, {19, 1,12}, {19, 1,12},
    {18, 1,12}, {18, 1,12}, { 1, 5,12}, { 1, 5,12},
    { 3, 3,12}, { 3, 3,12}, { 0, 8,12}, { 0, 8,12},
    { 6, 2,12}, { 6, 2,12}, {17, 1,12}, {17, 1,12}
};

DCTtab DCT_B14_10 [] = {
    {16, 1,10}, { 5, 2,10}, { 0, 7,10}, { 2, 3,10},
    { 1, 4,10}, {15, 1,10}, {14, 1,10}, { 4, 2,10}
};

DCTtab DCT_B14_8 [] = {
    {65, 0, 6}, {65, 0, 6}, {65, 0, 6}, {65, 0, 6},
    { 2, 2, 7}, { 2, 2, 7}, { 9, 1, 7}, { 9, 1, 7},
    { 0, 4, 7}, { 0, 4, 7}, { 8, 1, 7}, { 8, 1, 7},
    { 7, 1, 6}, { 7, 1, 6}, { 7, 1, 6}, { 7, 1, 6},
    { 6, 1, 6}, { 6, 1, 6}, { 6, 1, 6}, { 6, 1, 6},
    { 1, 2, 6}, { 1, 2, 6}, { 1, 2, 6}, { 1, 2, 6},
    { 5, 1, 6}, { 5, 1, 6}, { 5, 1, 6}, { 5, 1, 6},
    {13, 1, 8}, { 0, 6, 8}, {12, 1, 8}, {11, 1, 8},
    { 3, 2, 8}, { 1, 3, 8}, { 0, 5, 8}, {10, 1, 8}
};

DCTtab DCT_B14AC_5 [] = {
		{ 0, 3, 5}, { 4, 1, 5}, { 3, 1, 5},
    { 0, 2, 4}, { 0, 2, 4}, { 2, 1, 4}, { 2, 1, 4},
    { 1, 1, 3}, { 1, 1, 3}, { 1, 1, 3}, { 1, 1, 3},
    {64, 0, 2}, {64, 0, 2}, {64, 0, 2}, {64, 0, 2},
    {64, 0, 2}, {64, 0, 2}, {64, 0, 2}, {64, 0, 2},
    { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2},
    { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2}
};

DCTtab DCT_B14DC_5 [] = {
		{ 0, 3, 5}, { 4, 1, 5}, { 3, 1, 5},
    { 0, 2, 4}, { 0, 2, 4}, { 2, 1, 4}, { 2, 1, 4},
    { 1, 1, 3}, { 1, 1, 3}, { 1, 1, 3}, { 1, 1, 3},
    { 0, 1, 1}, { 0, 1, 1}, { 0, 1, 1}, { 0, 1, 1},
    { 0, 1, 1}, { 0, 1, 1}, { 0, 1, 1}, { 0, 1, 1},
    { 0, 1, 1}, { 0, 1, 1}, { 0, 1, 1}, { 0, 1, 1},
    { 0, 1, 1}, { 0, 1, 1}, { 0, 1, 1}, { 0, 1, 1}
};

DCTtab DCT_B15_10 [] = {
    { 5, 2, 9}, { 5, 2, 9}, {14, 1, 9}, {14, 1, 9},
    { 2, 4,10}, {16, 1,10}, {15, 1, 9}, {15, 1, 9}
};

DCTtab DCT_B15_8 [] = {
    {65, 0, 6}, {65, 0, 6}, {65, 0, 6}, {65, 0, 6},
    { 7, 1, 7}, { 7, 1, 7}, { 8, 1, 7}, { 8, 1, 7},
    { 6, 1, 7}, { 6, 1, 7}, { 2, 2, 7}, { 2, 2, 7},
    { 0, 7, 6}, { 0, 7, 6}, { 0, 7, 6}, { 0, 7, 6},
    { 0, 6, 6}, { 0, 6, 6}, { 0, 6, 6}, { 0, 6, 6},
    { 4, 1, 6}, { 4, 1, 6}, { 4, 1, 6}, { 4, 1, 6},
    { 5, 1, 6}, { 5, 1, 6}, { 5, 1, 6}, { 5, 1, 6},
    { 1, 5, 8}, {11, 1, 8}, { 0,11, 8}, { 0,10, 8},
    {13, 1, 8}, {12, 1, 8}, { 3, 2, 8}, { 1, 4, 8},
    { 2, 1, 5}, { 2, 1, 5}, { 2, 1, 5}, { 2, 1, 5},
    { 2, 1, 5}, { 2, 1, 5}, { 2, 1, 5}, { 2, 1, 5},
    { 1, 2, 5}, { 1, 2, 5}, { 1, 2, 5}, { 1, 2, 5},
    { 1, 2, 5}, { 1, 2, 5}, { 1, 2, 5}, { 1, 2, 5},
    { 3, 1, 5}, { 3, 1, 5}, { 3, 1, 5}, { 3, 1, 5},
    { 3, 1, 5}, { 3, 1, 5}, { 3, 1, 5}, { 3, 1, 5},
    { 1, 1, 3}, { 1, 1, 3}, { 1, 1, 3}, { 1, 1, 3},
    { 1, 1, 3}, { 1, 1, 3}, { 1, 1, 3}, { 1, 1, 3},
    { 1, 1, 3}, { 1, 1, 3}, { 1, 1, 3}, { 1, 1, 3},
    { 1, 1, 3}, { 1, 1, 3}, { 1, 1, 3}, { 1, 1, 3},
    { 1, 1, 3}, { 1, 1, 3}, { 1, 1, 3}, { 1, 1, 3},
    { 1, 1, 3}, { 1, 1, 3}, { 1, 1, 3}, { 1, 1, 3},
    { 1, 1, 3}, { 1, 1, 3}, { 1, 1, 3}, { 1, 1, 3},
    { 1, 1, 3}, { 1, 1, 3}, { 1, 1, 3}, { 1, 1, 3},
    {64, 0, 4}, {64, 0, 4}, {64, 0, 4}, {64, 0, 4},
    {64, 0, 4}, {64, 0, 4}, {64, 0, 4}, {64, 0, 4},
    {64, 0, 4}, {64, 0, 4}, {64, 0, 4}, {64, 0, 4},
    {64, 0, 4}, {64, 0, 4}, {64, 0, 4}, {64, 0, 4},
    { 0, 3, 4}, { 0, 3, 4}, { 0, 3, 4}, { 0, 3, 4},
    { 0, 3, 4}, { 0, 3, 4}, { 0, 3, 4}, { 0, 3, 4},
    { 0, 3, 4}, { 0, 3, 4}, { 0, 3, 4}, { 0, 3, 4},
    { 0, 3, 4}, { 0, 3, 4}, { 0, 3, 4}, { 0, 3, 4},
    { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2},
    { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2},
    { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2},
    { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2},
    { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2},
    { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2},
    { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2},
    { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2},
    { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2},
    { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2},
    { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2},
    { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2},
    { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2},
    { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2},
    { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2},
    { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2}, { 0, 1, 2},
    { 0, 2, 3}, { 0, 2, 3}, { 0, 2, 3}, { 0, 2, 3},
    { 0, 2, 3}, { 0, 2, 3}, { 0, 2, 3}, { 0, 2, 3},
    { 0, 2, 3}, { 0, 2, 3}, { 0, 2, 3}, { 0, 2, 3},
    { 0, 2, 3}, { 0, 2, 3}, { 0, 2, 3}, { 0, 2, 3},
    { 0, 2, 3}, { 0, 2, 3}, { 0, 2, 3}, { 0, 2, 3},
    { 0, 2, 3}, { 0, 2, 3}, { 0, 2, 3}, { 0, 2, 3},
    { 0, 2, 3}, { 0, 2, 3}, { 0, 2, 3}, { 0, 2, 3},
    { 0, 2, 3}, { 0, 2, 3}, { 0, 2, 3}, { 0, 2, 3},
    { 0, 4, 5}, { 0, 4, 5}, { 0, 4, 5}, { 0, 4, 5},
    { 0, 4, 5}, { 0, 4, 5}, { 0, 4, 5}, { 0, 4, 5},
    { 0, 5, 5}, { 0, 5, 5}, { 0, 5, 5}, { 0, 5, 5},
    { 0, 5, 5}, { 0, 5, 5}, { 0, 5, 5}, { 0, 5, 5},
    { 9, 1, 7}, { 9, 1, 7}, { 1, 3, 7}, { 1, 3, 7},
    {10, 1, 7}, {10, 1, 7}, { 0, 8, 7}, { 0, 8, 7},
    { 0, 9, 7}, { 0, 9, 7}, { 0,12, 8}, { 0,13, 8},
    { 2, 3, 8}, { 4, 2, 8}, { 0,14, 8}, { 0,15, 8}
};

static int non_linear_quantizer_scale[32] =
{
     0, 1, 2, 3, 4, 5, 6, 7,
     8,10,12,14,16,18,20,22,
    24,28,32,36,40,44,48,52,
    56,64,72,80,88,96,104,112
};

static inline int get_quantizer_scale (int q_scale_type)
{
    int quantizer_scale_code;

    quantizer_scale_code = bitstream_get (5);

    if (q_scale_type)
	return non_linear_quantizer_scale[quantizer_scale_code];
    else
	return quantizer_scale_code << 1;
}

static void slice_get_intra_block (const picture_t * picture,
				   slice_t * slice,
				   int16_t * dest, int cc)
{
    int i = 1;
    int j;
    int run;
    int val;
    const uint8_t * scan = picture->scan;
    uint8_t * quant_matrix = picture->intra_quantizer_matrix;
    int quantizer_scale = slice->quantizer_scale;
    int mismatch;
    uint32_t code;
    DCTtab * tab;

    needbits ();
    //Get the intra DC coefficient and inverse quantize it
    if (cc == 0)
	dest[0] = (slice->dc_dct_pred[0] += Get_Luma_DC_dct_diff ()) <<
	    (3 - picture->intra_dc_precision);
    else
	dest[0] = (slice->dc_dct_pred[cc]+= Get_Chroma_DC_dct_diff ()) <<
	    (3 - picture->intra_dc_precision);

    i = 1;
    mismatch = ~dest[0];

    while (1) {
	needbits ();
	code = bitstream_show (16);

	if (picture->intra_vlc_format) {
	    if (code >= 0x0400)
		tab = DCT_B15_8 - 4 + (code >> 8);
	    else if (code >= 0x0200)
		tab = DCT_B15_10 - 8 + (code >> 6);
	    else if (code >= 0x0080)
		tab = DCT_13 - 16 + (code >> 3);
	    else if (code >= 0x0020)
		tab = DCT_15 - 16 + (code >> 1);
	    else if (code >= 0x0010)
		tab = DCT_16 - 16 + code;
	    else {
		fprintf (stderr,
			 "(vlc) invalid huffman code 0x%x in vlc_get_block_coeff ()\n",
			 code);
		exit (1);
		break;
	    }
	} else {
	    if (code >= 0x2800)
		tab = DCT_B14AC_5 - 5 + (code >> 11);
	    else if (code >= 0x0400)
		tab = DCT_B14_8 - 4 + (code >> 8);
	    else if (code >= 0x0200)
		tab = DCT_B14_10 - 8 + (code >> 6);
	    else if (code >= 0x0080)
		tab = DCT_13 - 16 + (code >> 3);
	    else if (code >= 0x0020)
		tab = DCT_15 - 16 + (code >> 1);
	    else if (code >= 0x0010)
		tab = DCT_16 - 16 + code;
	    else {
		fprintf (stderr,
			 "(vlc) invalid huffman code 0x%x in vlc_get_block_coeff ()\n",
			 code);
		exit (1);
		break;
	    }
	}

	bitstream_flush (tab->len);

	if (tab->run==64) // end_of_block 
	    break;

	if (tab->run==65) /* escape */ {
	    run = bitstream_get (6);

	    needbits ();
	    val = bitstream_get (12);
	    if (val >= 2048)
		val = val - 4096;
	} else {
	    run = tab->run;

	    val = tab->level;
	    needbits ();	// FIXME get rid of this one
	    if (bitstream_get (1)) //sign bit
		val = -val;
	}

	i += run;
	j = scan[i++];
	mismatch ^= dest[j] = (val * quantizer_scale * quant_matrix[j]) / 16;
    }
    dest[63] ^= mismatch & 1;
}

static void slice_get_non_intra_block (const picture_t * picture,
				       slice_t * slice, int16_t * dest)
{
    int i;
    int j;
    int run;
    int val;
    const uint8_t * scan = picture->scan;
    uint8_t * quant_matrix = picture->non_intra_quantizer_matrix;
    int quantizer_scale = slice->quantizer_scale;
    int k;
    int mismatch;
    uint32_t code;
    DCTtab * tab;

    i = 0;
    mismatch = 1;

    needbits ();
    code = bitstream_show (16);

    if (code >= 0x2800)
	tab = DCT_B14DC_5 - 5 + (code >> 11);
    else if (code >= 0x0400)
	tab = DCT_B14_8 - 4 + (code >> 8);
    else if (code >= 0x0200)
	tab = DCT_B14_10 - 8 + (code >> 6);
    else if (code >= 0x0080)
	tab = DCT_13 - 16 + (code >> 3);
    else if (code >= 0x0020)
	tab = DCT_15 - 16 + (code >> 1);
    else if (code >= 0x0010)
	tab = DCT_16 - 16 + code;
    else {
	fprintf (stderr,
		 "(vlc) invalid huffman code 0x%x in vlc_get_block_coeff ()\n",
		 code);
	exit (1);
	return;
    }

    goto gotit;
 
    while (1) {
	needbits ();
	code = bitstream_show (16);

	if (code >= 0x2800)
	    tab = DCT_B14AC_5 - 5 + (code >> 11);
	else if (code >= 0x0400)
	    tab = DCT_B14_8 - 4 + (code >> 8);
	else if (code >= 0x0200)
	    tab = DCT_B14_10 - 8 + (code >> 6);
	else if (code >= 0x0080)
	    tab = DCT_13 - 16 + (code >> 3);
	else if (code >= 0x0020)
	    tab = DCT_15 - 16 + (code >> 1);
	else if (code >= 0x0010)
	    tab = DCT_16 - 16 + code;
	else {
	    fprintf (stderr,
		     "(vlc) invalid huffman code 0x%x in vlc_get_block_coeff ()\n",
		     code);
	    exit (1);
	    break;
	}

    gotit:

	bitstream_flush (tab->len);

	if (tab->run==64) // end_of_block 
	    break;

	if (tab->run==65) /* escape */ {
	    run = bitstream_get (6);

	    needbits ();
	    val = bitstream_get (12);
	    if (val >= 2048)
		val = val - 4096;
	} else {
	    run = tab->run;

	    val = tab->level;
	    needbits ();	// FIXME get rid of this one
	    if (bitstream_get (1)) //sign bit
		val = -val;
	}

	i += run;
	j = scan[i++];
	k = (val > 0) ? 1 : ((val < 0) ? -1 : 0);
	mismatch ^= dest[j] =
	    ((2 * val + k) * quantizer_scale * quant_matrix[j]) / 32;
    }
    dest[63] ^= mismatch & 1;
}

static inline void slice_intra_DCT (picture_t * picture, slice_t * slice,
				    int cc, uint8_t * dest, int stride)
{
    slice_get_intra_block (picture, slice, DCTblock, cc);
    idct_block_copy (DCTblock, dest, stride);
    memset (DCTblock, 0, sizeof (int16_t) * 64);
}

static inline void slice_non_intra_DCT (picture_t * picture, slice_t * slice,
					uint8_t * dest, int stride)
{
    slice_get_non_intra_block (picture, slice, DCTblock);
    idct_block_add (DCTblock, dest, stride);
    memset (DCTblock, 0, sizeof (int16_t) * 64);
}

static int get_motion_delta (int f_code)
{
    int motion_code, motion_residual;

    motion_code = Get_motion_code ();
    if (motion_code == 0)
	return 0;

    motion_residual = 0;
    if (f_code != 0) {
	needbits ();
	motion_residual = bitstream_get (f_code);
    }

    if (motion_code > 0)
	return ((motion_code - 1) << f_code) + motion_residual + 1;
    else
	return ((motion_code + 1) << f_code) - motion_residual - 1;
}

static inline int bound_motion_vector (int vector, int f_code)
{
#if 1
    int limit;

    limit = 16 << f_code;

    if (vector >= limit)
	return vector - 2*limit;
    else if (vector < -limit)
	return vector + 2*limit;
    else return vector;
#else
    return (vector << (27 - f_code)) >> (27 - f_code);
#endif
}

static void motion_frame (motion_t * motion, uint8_t * dest[3],
			  int offset, int width,
			  void (** table) (uint8_t *, uint8_t *, int, int))
{
    int motion_x, motion_y;

    needbits ();
    motion_x = motion->pmv[0][0] + get_motion_delta (motion->f_code[0]);
    motion_x = bound_motion_vector (motion_x, motion->f_code[0]);
    motion->pmv[1][0] = motion->pmv[0][0] = motion_x;

    needbits ();
    motion_y = motion->pmv[0][1] + get_motion_delta (motion->f_code[1]);
    motion_y = bound_motion_vector (motion_y, motion->f_code[1]);
    motion->pmv[1][1] = motion->pmv[0][1] = motion_y;

    motion_block (table, motion_x, motion_y, dest, offset,
		  motion->ref_frame, offset, width, 16);
}

static void motion_field (motion_t * motion, uint8_t * dest[3],
			  int offset, int width,
			  void (** table) (uint8_t *, uint8_t *, int, int))
{
    int vertical_field_select;
    int motion_x, motion_y;

    needbits ();
    vertical_field_select = bitstream_get (1);

    motion_x = motion->pmv[0][0] + get_motion_delta (motion->f_code[0]);
    motion_x = bound_motion_vector (motion_x, motion->f_code[0]);
    motion->pmv[0][0] = motion_x;

    needbits ();
    motion_y = (motion->pmv[0][1] >> 1) + get_motion_delta (motion->f_code[1]);
    //motion_y = bound_motion_vector (motion_y, motion->f_code[1]);
    motion->pmv[0][1] = motion_y << 1;

    motion_block (table, motion_x, motion_y, dest, offset,
		  motion->ref_frame, offset + vertical_field_select * width,
		  width * 2, 8);

    needbits ();
    vertical_field_select = bitstream_get (1);

    motion_x = motion->pmv[1][0] + get_motion_delta (motion->f_code[0]);
    motion_x = bound_motion_vector (motion_x, motion->f_code[0]);
    motion->pmv[1][0] = motion_x;

    needbits ();
    motion_y = (motion->pmv[1][1] >> 1) + get_motion_delta (motion->f_code[1]);
    //motion_y = bound_motion_vector (motion_y, motion->f_code[1]);
    motion->pmv[1][1] = motion_y << 1;

    motion_block (table, motion_x, motion_y, dest, offset + width,
		  motion->ref_frame, offset + vertical_field_select * width,
		  width * 2, 8);
}

// like motion_frame, but reuse previous motion vectors
static void motion_reuse (motion_t * motion, uint8_t * dest[3],
			  int offset, int width,
			  void (** table) (uint8_t *, uint8_t *, int, int))
{
    motion_block (table, motion->pmv[0][0], motion->pmv[0][1], dest, offset,
		  motion->ref_frame, offset, width, 16);
}

// like motion_frame, but use null motion vectors
static void motion_zero (motion_t * motion, uint8_t * dest[3],
			 int offset, int width,
			 void (** table) (uint8_t *, uint8_t *, int, int))
{
    motion_block (table, 0, 0, dest, offset,
		  motion->ref_frame, offset, width, 16);
}

// like motion_frame, but no actual motion compensation
static void motion_conceal (motion_t * motion)
{
    int tmp;

    needbits ();
    tmp = motion->pmv[0][0] + get_motion_delta (motion->f_code[0]);
    tmp = bound_motion_vector (tmp, motion->f_code[0]);
    motion->pmv[1][0] = motion->pmv[0][0] = tmp;

    needbits ();
    tmp = motion->pmv[0][1] + get_motion_delta (motion->f_code[1]);
    tmp = bound_motion_vector (tmp, motion->f_code[1]);
    motion->pmv[1][1] = motion->pmv[0][1] = tmp;

    bitstream_flush (1); // remove marker_bit
}

#define MOTION(routine,direction,slice,dest,offset,stride)	\
do {								\
    if ((direction) & MACROBLOCK_MOTION_FORWARD)		\
	routine (& ((slice).f_motion), dest, offset, stride,	\
		 mc_functions.put);				\
    if ((direction) & MACROBLOCK_MOTION_BACKWARD)		\
	routine (& ((slice).b_motion), dest, offset, stride,	\
		 ((direction) & MACROBLOCK_MOTION_FORWARD ?	\
		  mc_functions.avg : mc_functions.put));	\
} while (0);

static int get_macroblock_modes (int picture_coding_type,
				 int frame_pred_frame_dct)
{
    int macroblock_modes;

    macroblock_modes = Get_macroblock_type (picture_coding_type);

    if (frame_pred_frame_dct) {
	if (macroblock_modes & (MACROBLOCK_MOTION_FORWARD |
				MACROBLOCK_MOTION_BACKWARD))
	    macroblock_modes |= MC_FRAME;
	return macroblock_modes;
    }

    // get frame/field motion type 
    if (macroblock_modes & (MACROBLOCK_MOTION_FORWARD |
			    MACROBLOCK_MOTION_BACKWARD))
	macroblock_modes |= bitstream_get (2) * MOTION_TYPE_BASE;

    if (macroblock_modes & (MACROBLOCK_INTRA | MACROBLOCK_PATTERN))
	macroblock_modes |= bitstream_get (1) * DCT_TYPE_INTERLACED;

    return macroblock_modes;
}

int slice_process (picture_t * picture, uint8_t code, uint8_t * buffer)
{
    int mba; 
    int mba_inc;
    int macroblock_modes;
    int width;
    uint8_t * dest[3];
    int offset;

    width = picture->coded_picture_width;
    mba = (code - 1) * (picture->coded_picture_width >> 4);
    offset = (code - 1) * width * 4;

    dest[0] = picture->current_frame[0] + offset * 4;
    dest[1] = picture->current_frame[1] + offset;
    dest[2] = picture->current_frame[2] + offset;
    slice.f_motion.ref_frame[0] =
	picture->forward_reference_frame[0] + offset * 4;
    slice.f_motion.ref_frame[1] = picture->forward_reference_frame[1] + offset;
    slice.f_motion.ref_frame[2] = picture->forward_reference_frame[2] + offset;
    slice.f_motion.f_code[0] = picture->f_code[0][0];
    slice.f_motion.f_code[1] = picture->f_code[0][1];
    slice.f_motion.pmv[0][0] = slice.f_motion.pmv[0][1] = 0;
    slice.f_motion.pmv[1][0] = slice.f_motion.pmv[1][1] = 0;
    slice.b_motion.ref_frame[0] =
	picture->backward_reference_frame[0] + offset * 4;
    slice.b_motion.ref_frame[1] =
	picture->backward_reference_frame[1] + offset;
    slice.b_motion.ref_frame[2] =
	picture->backward_reference_frame[2] + offset;
    slice.b_motion.f_code[0] = picture->f_code[1][0];
    slice.b_motion.f_code[1] = picture->f_code[1][1];
    slice.b_motion.pmv[0][0] = slice.b_motion.pmv[0][1] = 0;
    slice.b_motion.pmv[1][0] = slice.b_motion.pmv[1][1] = 0;

    //reset intra dc predictor
    slice.dc_dct_pred[0]=slice.dc_dct_pred[1]=slice.dc_dct_pred[2]= 
	1<< (picture->intra_dc_precision + 7) ;

    bitstream_init (buffer);

    needbits ();
    slice.quantizer_scale = get_quantizer_scale (picture->q_scale_type);

    //Ignore intra_slice and all the extra data
    while (bitstream_get (1)) {
	bitstream_flush (8);
	needbits ();
    }

    mba_inc = Get_macroblock_address_increment () - 1;
    mba += mba_inc;
    offset = 16 * mba_inc;

    while (1) {
	needbits ();

	macroblock_modes =
	    get_macroblock_modes (picture->picture_coding_type,
				  picture->frame_pred_frame_dct);

	if (macroblock_modes & MACROBLOCK_QUANT)
	    slice.quantizer_scale =
		get_quantizer_scale (picture->q_scale_type);

	if (macroblock_modes & MACROBLOCK_INTRA) {

	    int DCT_offset, DCT_stride;

	    if (picture->concealment_motion_vectors)
		motion_conceal (&slice.f_motion);
	    else {
		slice.f_motion.pmv[0][0] = slice.f_motion.pmv[0][1] = 0;
		slice.f_motion.pmv[1][0] = slice.f_motion.pmv[1][1] = 0;
		slice.b_motion.pmv[0][0] = slice.b_motion.pmv[0][1] = 0;
		slice.b_motion.pmv[1][0] = slice.b_motion.pmv[1][1] = 0;
	    }

	    if (macroblock_modes & DCT_TYPE_INTERLACED) {
		DCT_offset = width;
		DCT_stride = width * 2;
	    } else {
		DCT_offset = width * 8;
		DCT_stride = width;
	    }

	    // Decode lum blocks
	    slice_intra_DCT (picture, &slice, 0,
			     dest[0] + offset, DCT_stride);
	    slice_intra_DCT (picture, &slice, 0,
			     dest[0] + offset + 8, DCT_stride);
	    slice_intra_DCT (picture, &slice, 0,
			     dest[0] + offset + DCT_offset, DCT_stride);
	    slice_intra_DCT (picture, &slice, 0,
			     dest[0] + offset + DCT_offset + 8, DCT_stride);

	    // Decode chroma blocks
	    slice_intra_DCT (picture, &slice, 1,
			     dest[1] + (offset>>1), width>>1);
	    slice_intra_DCT (picture, &slice, 2,
			     dest[2] + (offset>>1), width>>1);

	} else {

	    switch (macroblock_modes & MOTION_TYPE_MASK) {
	    case MC_FRAME:
		MOTION (motion_frame, macroblock_modes, slice, dest,
			offset, width);
		break;

	    case MC_FIELD:
		MOTION (motion_field, macroblock_modes, slice, dest,
			offset, width);
		break;

	    case 0:
		// non-intra mb without forward mv in a P picture
		slice.f_motion.pmv[0][0] = slice.f_motion.pmv[0][1] = 0;
		slice.f_motion.pmv[1][0] = slice.f_motion.pmv[1][1] = 0;

		MOTION (motion_zero, MACROBLOCK_MOTION_FORWARD,
			slice, dest, offset, width);
		break;
	    }

	    //6.3.17.4 Coded block pattern
	    if (macroblock_modes & MACROBLOCK_PATTERN) {
		int coded_block_pattern;
		int DCT_offset, DCT_stride;

		if (macroblock_modes & DCT_TYPE_INTERLACED) {
		    DCT_offset = width;
		    DCT_stride = width * 2;
		} else {
		    DCT_offset = width * 8;
		    DCT_stride = width;
		}

		needbits ();
		coded_block_pattern = Get_coded_block_pattern ();

		// Decode lum blocks

		if (coded_block_pattern & 0x20)
		    slice_non_intra_DCT (picture, &slice,
					 dest[0] + offset, DCT_stride);
		if (coded_block_pattern & 0x10)
		    slice_non_intra_DCT (picture, &slice,
					 dest[0] + offset + 8, DCT_stride);
		if (coded_block_pattern & 0x08)
		    slice_non_intra_DCT (picture, &slice,
					 dest[0] + offset + DCT_offset,
					 DCT_stride);
		if (coded_block_pattern & 0x04)
		    slice_non_intra_DCT (picture, &slice,
					 dest[0] + offset + DCT_offset + 8,
					 DCT_stride);

		// Decode chroma blocks

		if (coded_block_pattern & 0x2)
		    slice_non_intra_DCT (picture, &slice,
					 dest[1] + (offset>>1), width >> 1);
		if (coded_block_pattern & 0x1)
		    slice_non_intra_DCT (picture, &slice,
					 dest[2] + (offset>>1), width >> 1);
	    }

	    slice.dc_dct_pred[0]=slice.dc_dct_pred[1]=slice.dc_dct_pred[2]=
		1 << (picture->intra_dc_precision + 7);
	}

	mba++;
	offset += 16;

	needbits ();
	if (! bitstream_show (11))
	    break;

	mba_inc = Get_macroblock_address_increment () - 1;

	if (mba_inc) {
	    //reset intra dc predictor on skipped block
	    slice.dc_dct_pred[0]=slice.dc_dct_pred[1]=slice.dc_dct_pred[2]=
		1<< (picture->intra_dc_precision + 7);

	    //handling of skipped mb's differs between P_TYPE and B_TYPE
	    //pictures
	    if (picture->picture_coding_type == P_TYPE) {
		slice.f_motion.pmv[0][0] = slice.f_motion.pmv[0][1] = 0;
		slice.f_motion.pmv[1][0] = slice.f_motion.pmv[1][1] = 0;

		do {
		    MOTION (motion_zero, MACROBLOCK_MOTION_FORWARD,
			    slice, dest, offset, width);

		    mba++;
		    offset += 16;
		} while (--mba_inc);
	    } else {
		do {
		    MOTION (motion_reuse, macroblock_modes,
			    slice, dest, offset, width);

		    mba++;
		    offset += 16;
		} while (--mba_inc);
	    }
	}
    }

    return (mba >= picture->last_mba);
}
