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
    uint8_t run;
    uint8_t level;
    uint8_t len;
} DCTtab;

DCTtab DCT_16 [] = {
    {129, 0, 0}, {129, 0, 0}, {129, 0, 0}, {129, 0, 0},
    {129, 0, 0}, {129, 0, 0}, {129, 0, 0}, {129, 0, 0},
    {129, 0, 0}, {129, 0, 0}, {129, 0, 0}, {129, 0, 0},
    {129, 0, 0}, {129, 0, 0}, {129, 0, 0}, {129, 0, 0},
    {  2,18, 0}, {  2,17, 0}, {  2,16, 0}, {  2,15, 0},
    {  7, 3, 0}, { 17, 2, 0}, { 16, 2, 0}, { 15, 2, 0},
    { 14, 2, 0}, { 13, 2, 0}, { 12, 2, 0}, { 32, 1, 0},
    { 31, 1, 0}, { 30, 1, 0}, { 29, 1, 0}, { 28, 1, 0}
};

DCTtab DCT_15 [] = {
    {  1,40,15}, {  1,39,15}, {  1,38,15}, {  1,37,15},
    {  1,36,15}, {  1,35,15}, {  1,34,15}, {  1,33,15},
    {  1,32,15}, {  2,14,15}, {  2,13,15}, {  2,12,15},
    {  2,11,15}, {  2,10,15}, {  2, 9,15}, {  2, 8,15},
    {  1,31,14}, {  1,31,14}, {  1,30,14}, {  1,30,14},
    {  1,29,14}, {  1,29,14}, {  1,28,14}, {  1,28,14},
    {  1,27,14}, {  1,27,14}, {  1,26,14}, {  1,26,14},
    {  1,25,14}, {  1,25,14}, {  1,24,14}, {  1,24,14},
    {  1,23,14}, {  1,23,14}, {  1,22,14}, {  1,22,14},
    {  1,21,14}, {  1,21,14}, {  1,20,14}, {  1,20,14},
    {  1,19,14}, {  1,19,14}, {  1,18,14}, {  1,18,14},
    {  1,17,14}, {  1,17,14}, {  1,16,14}, {  1,16,14}
};

DCTtab DCT_13 [] = {
    { 11, 2,13}, { 10, 2,13}, {  6, 3,13}, {  4, 4,13},
    {  3, 5,13}, {  2, 7,13}, {  2, 6,13}, {  1,15,13},
    {  1,14,13}, {  1,13,13}, {  1,12,13}, { 27, 1,13},
    { 26, 1,13}, { 25, 1,13}, { 24, 1,13}, { 23, 1,13},
    {  1,11,12}, {  1,11,12}, {  9, 2,12}, {  9, 2,12},
    {  5, 3,12}, {  5, 3,12}, {  1,10,12}, {  1,10,12},
    {  3, 4,12}, {  3, 4,12}, {  8, 2,12}, {  8, 2,12},
    { 22, 1,12}, { 22, 1,12}, { 21, 1,12}, { 21, 1,12},
    {  1, 9,12}, {  1, 9,12}, { 20, 1,12}, { 20, 1,12},
    { 19, 1,12}, { 19, 1,12}, {  2, 5,12}, {  2, 5,12},
    {  4, 3,12}, {  4, 3,12}, {  1, 8,12}, {  1, 8,12},
    {  7, 2,12}, {  7, 2,12}, { 18, 1,12}, { 18, 1,12}
};

DCTtab DCT_B14_10 [] = {
    { 17, 1,10}, {  6, 2,10}, {  1, 7,10}, {  3, 3,10},
    {  2, 4,10}, { 16, 1,10}, { 15, 1,10}, {  5, 2,10}
};

DCTtab DCT_B14_8 [] = {
    { 65, 0, 6}, { 65, 0, 6}, { 65, 0, 6}, { 65, 0, 6},
    {  3, 2, 7}, {  3, 2, 7}, { 10, 1, 7}, { 10, 1, 7},
    {  1, 4, 7}, {  1, 4, 7}, {  9, 1, 7}, {  9, 1, 7},
    {  8, 1, 6}, {  8, 1, 6}, {  8, 1, 6}, {  8, 1, 6},
    {  7, 1, 6}, {  7, 1, 6}, {  7, 1, 6}, {  7, 1, 6},
    {  2, 2, 6}, {  2, 2, 6}, {  2, 2, 6}, {  2, 2, 6},
    {  6, 1, 6}, {  6, 1, 6}, {  6, 1, 6}, {  6, 1, 6},
    { 14, 1, 8}, {  1, 6, 8}, { 13, 1, 8}, { 12, 1, 8},
    {  4, 2, 8}, {  2, 3, 8}, {  1, 5, 8}, { 11, 1, 8}
};

DCTtab DCT_B14AC_5 [] = {
		 {  1, 3, 5}, {  5, 1, 5}, {  4, 1, 5},
    {  1, 2, 4}, {  1, 2, 4}, {  3, 1, 4}, {  3, 1, 4},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {129, 0, 2}, {129, 0, 2}, {129, 0, 2}, {129, 0, 2},
    {129, 0, 2}, {129, 0, 2}, {129, 0, 2}, {129, 0, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}
};

DCTtab DCT_B14DC_5 [] = {
		 {  1, 3, 5}, {  5, 1, 5}, {  4, 1, 5},
    {  1, 2, 4}, {  1, 2, 4}, {  3, 1, 4}, {  3, 1, 4},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1},
    {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1},
    {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1},
    {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1}
};

DCTtab DCT_B15_10 [] = {
    {  6, 2, 9}, {  6, 2, 9}, { 15, 1, 9}, { 15, 1, 9},
    {  3, 4,10}, { 17, 1,10}, { 16, 1, 9}, { 16, 1, 9}
};

DCTtab DCT_B15_8 [] = {
    { 65, 0, 6}, { 65, 0, 6}, { 65, 0, 6}, { 65, 0, 6},
    {  8, 1, 7}, {  8, 1, 7}, {  9, 1, 7}, {  9, 1, 7},
    {  7, 1, 7}, {  7, 1, 7}, {  3, 2, 7}, {  3, 2, 7},
    {  1, 7, 6}, {  1, 7, 6}, {  1, 7, 6}, {  1, 7, 6},
    {  1, 6, 6}, {  1, 6, 6}, {  1, 6, 6}, {  1, 6, 6},
    {  5, 1, 6}, {  5, 1, 6}, {  5, 1, 6}, {  5, 1, 6},
    {  6, 1, 6}, {  6, 1, 6}, {  6, 1, 6}, {  6, 1, 6},
    {  2, 5, 8}, { 12, 1, 8}, {  1,11, 8}, {  1,10, 8},
    { 14, 1, 8}, { 13, 1, 8}, {  4, 2, 8}, {  2, 4, 8},
    {  3, 1, 5}, {  3, 1, 5}, {  3, 1, 5}, {  3, 1, 5},
    {  3, 1, 5}, {  3, 1, 5}, {  3, 1, 5}, {  3, 1, 5},
    {  2, 2, 5}, {  2, 2, 5}, {  2, 2, 5}, {  2, 2, 5},
    {  2, 2, 5}, {  2, 2, 5}, {  2, 2, 5}, {  2, 2, 5},
    {  4, 1, 5}, {  4, 1, 5}, {  4, 1, 5}, {  4, 1, 5},
    {  4, 1, 5}, {  4, 1, 5}, {  4, 1, 5}, {  4, 1, 5},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
    {129, 0, 4}, {129, 0, 4}, {129, 0, 4}, {129, 0, 4},
    {129, 0, 4}, {129, 0, 4}, {129, 0, 4}, {129, 0, 4},
    {129, 0, 4}, {129, 0, 4}, {129, 0, 4}, {129, 0, 4},
    {129, 0, 4}, {129, 0, 4}, {129, 0, 4}, {129, 0, 4},
    {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4},
    {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4},
    {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4},
    {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
    {  1, 4, 5}, {  1, 4, 5}, {  1, 4, 5}, {  1, 4, 5},
    {  1, 4, 5}, {  1, 4, 5}, {  1, 4, 5}, {  1, 4, 5},
    {  1, 5, 5}, {  1, 5, 5}, {  1, 5, 5}, {  1, 5, 5},
    {  1, 5, 5}, {  1, 5, 5}, {  1, 5, 5}, {  1, 5, 5},
    { 10, 1, 7}, { 10, 1, 7}, {  2, 3, 7}, {  2, 3, 7},
    { 11, 1, 7}, { 11, 1, 7}, {  1, 8, 7}, {  1, 8, 7},
    {  1, 9, 7}, {  1, 9, 7}, {  1,12, 8}, {  1,13, 8},
    {  3, 3, 8}, {  5, 2, 8}, {  1,14, 8}, {  1,15, 8}
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

static void slice_get_intra_block_B14 (const picture_t * picture,
				       slice_t * slice, int16_t * dest)
{
    int i;
    int j;
    int val;
    const uint8_t * scan = picture->scan;
    uint8_t * quant_matrix = picture->non_intra_quantizer_matrix;
    int quantizer_scale = slice->quantizer_scale;
    int mismatch;
    DCTtab * tab;
    uint32_t bit_buf;
    int bits;

    i = 0;
    mismatch = ~dest[0];

    bit_buf = bitstream_buffer;
    bits = bitstream_avail_bits;

    NEEDBITS (bit_buf, bits);

    while (1) {
	if (bit_buf >= 0x28000000) {

	    tab = DCT_B14AC_5 - 5 + UBITS (bit_buf, 5);

	    i += tab->run;
	    if (i >= 64)
		break;	// end of block

	normal_code:
	    j = scan[i];
	    bit_buf <<= tab->len;
	    bits += tab->len + 1;
	    val = (tab->level * quantizer_scale * quant_matrix[j]) >> 4;

	    // if (bitstream_get (1)) val = -val;
	    val = (val ^ SBITS (bit_buf, 1)) - SBITS (bit_buf, 1);

	    dest[j] = val;
	    mismatch ^= val;

	    bit_buf <<= 1;
	    NEEDBITS (bit_buf, bits);

	    continue;

	} else if (bit_buf >= 0x04000000) {

	    tab = DCT_B14_8 - 4 + UBITS (bit_buf, 8);

	    i += tab->run;
	    if (i < 64)
		goto normal_code;

	    // escape code

	    i += UBITS (bit_buf << 6, 6) - 64;
	    if (i >= 64)
		break;	// illegal, but check needed to avoid buffer overflow

	    j = scan[i];

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits);
	    val = (SBITS (bit_buf, 12) *
		   quantizer_scale * quant_matrix[j]) / 16;

	    dest[j] = val;
	    mismatch ^= val;

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits);

	    continue;

	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B14_10 - 8 + UBITS (bit_buf, 10);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 - 16 + UBITS (bit_buf, 13);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 - 16 + UBITS (bit_buf, 15);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    bit_buf |= getword () << (bits + 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	}
	break;	// illegal, but check needed to avoid buffer overflow
    }
    dest[63] ^= mismatch & 1;
    DUMPBITS (bit_buf, bits, 2);	// dump end of block code
    bitstream_buffer = bit_buf;
    bitstream_avail_bits = bits;
}

static void slice_get_intra_block_B15 (const picture_t * picture,
				       slice_t * slice, int16_t * dest)
{
    int i;
    int j;
    int val;
    const uint8_t * scan = picture->scan;
    uint8_t * quant_matrix = picture->non_intra_quantizer_matrix;
    int quantizer_scale = slice->quantizer_scale;
    int mismatch;
    DCTtab * tab;
    uint32_t bit_buf;
    int bits;

    i = 0;
    mismatch = ~dest[0];

    bit_buf = bitstream_buffer;
    bits = bitstream_avail_bits;

    NEEDBITS (bit_buf, bits);

    while (1) {
	if (bit_buf >= 0x04000000) {

	    tab = DCT_B15_8 - 4 + UBITS (bit_buf, 8);

	    i += tab->run;
	    if (i < 64) {

	    normal_code:
		j = scan[i];
		bit_buf <<= tab->len;
		bits += tab->len + 1;
		val = (tab->level * quantizer_scale * quant_matrix[j]) >> 4;

		// if (bitstream_get (1)) val = -val;
		val = (val ^ SBITS (bit_buf, 1)) - SBITS (bit_buf, 1);

		dest[j] = val;
		mismatch ^= val;

		bit_buf <<= 1;
		NEEDBITS (bit_buf, bits);

		continue;

	    } else {

		// end of block. I commented out this code because if we
		// dont exit here we will still exit at the later test :)

		//if (i >= 128) break;	// end of block

		// escape code

		i += UBITS (bit_buf << 6, 6) - 64;
		if (i >= 64)
		    break;	// illegal, but check against buffer overflow

		j = scan[i];

		DUMPBITS (bit_buf, bits, 12);
		NEEDBITS (bit_buf, bits);
		val = (SBITS (bit_buf, 12) *
		       quantizer_scale * quant_matrix[j]) / 16;

		dest[j] = val;
		mismatch ^= val;

		DUMPBITS (bit_buf, bits, 12);
		NEEDBITS (bit_buf, bits);

		continue;

	    }
	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B15_10 - 8 + UBITS (bit_buf, 10);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 - 16 + UBITS (bit_buf, 13);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 - 16 + UBITS (bit_buf, 15);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    bit_buf |= getword () << (bits + 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	}
	break;	// illegal, but check needed to avoid buffer overflow
    }
    dest[63] ^= mismatch & 1;
    DUMPBITS (bit_buf, bits, 4);	// dump end of block code
    bitstream_buffer = bit_buf;
    bitstream_avail_bits = bits;
}

static void slice_get_non_intra_block (const picture_t * picture,
				       slice_t * slice, int16_t * dest)
{
    int i;
    int j;
    int val;
    const uint8_t * scan = picture->scan;
    uint8_t * quant_matrix = picture->non_intra_quantizer_matrix;
    int quantizer_scale = slice->quantizer_scale;
    int mismatch;
    DCTtab * tab;
    uint32_t bit_buf;
    int bits;

    i = -1;
    mismatch = 1;

    bit_buf = bitstream_buffer;
    bits = bitstream_avail_bits;

    NEEDBITS (bit_buf, bits);
    if (bit_buf >= 0x28000000) {
	tab = DCT_B14DC_5 - 5 + UBITS (bit_buf, 5);
	goto entry_1;
    } else
	goto entry_2;

    while (1) {
	if (bit_buf >= 0x28000000) {

	    tab = DCT_B14AC_5 - 5 + UBITS (bit_buf, 5);

	entry_1:
	    i += tab->run;
	    if (i >= 64)
		break;	// end of block

	normal_code:
	    j = scan[i];
	    bit_buf <<= tab->len;
	    bits += tab->len + 1;
	    val = ((2*tab->level+1) * quantizer_scale * quant_matrix[j]) >> 5;

	    // if (bitstream_get (1)) val = -val;
	    val = (val ^ SBITS (bit_buf, 1)) - SBITS (bit_buf, 1);

	    dest[j] = val;
	    mismatch ^= val;

	    bit_buf <<= 1;
	    NEEDBITS (bit_buf, bits);

	    continue;

	}

    entry_2:
	if (bit_buf >= 0x04000000) {

	    tab = DCT_B14_8 - 4 + UBITS (bit_buf, 8);

	    i += tab->run;
	    if (i < 64)
		goto normal_code;

	    // escape code

	    i += UBITS (bit_buf << 6, 6) - 64;
	    if (i >= 64)
		break;	// illegal, but check needed to avoid buffer overflow

	    j = scan[i];

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits);
	    val = 2 * (SBITS (bit_buf, 12) + SBITS (bit_buf, 1)) + 1;
	    val = (val * quantizer_scale * quant_matrix[j]) / 32;

	    dest[j] = val;
	    mismatch ^= val;

	    DUMPBITS (bit_buf, bits, 12);
	    NEEDBITS (bit_buf, bits);

	    continue;

	} else if (bit_buf >= 0x02000000) {
	    tab = DCT_B14_10 - 8 + UBITS (bit_buf, 10);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00800000) {
	    tab = DCT_13 - 16 + UBITS (bit_buf, 13);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else if (bit_buf >= 0x00200000) {
	    tab = DCT_15 - 16 + UBITS (bit_buf, 15);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	} else {
	    tab = DCT_16 + UBITS (bit_buf, 16);
	    bit_buf <<= 16;
	    bit_buf |= getword () << (bits + 16);
	    i += tab->run;
	    if (i < 64)
		goto normal_code;
	}
	break;	// illegal, but check needed to avoid buffer overflow
    }
    dest[63] ^= mismatch & 1;
    DUMPBITS (bit_buf, bits, 2);	// dump end of block code
    bitstream_buffer = bit_buf;
    bitstream_avail_bits = bits;
}

static inline void slice_intra_DCT (picture_t * picture, slice_t * slice,
				    int cc, uint8_t * dest, int stride)
{
    needbits ();
    //Get the intra DC coefficient and inverse quantize it
    if (cc == 0)
	slice->dc_dct_pred[0] += Get_Luma_DC_dct_diff ();
    else
	slice->dc_dct_pred[cc] += Get_Chroma_DC_dct_diff ();
    DCTblock[0] = slice->dc_dct_pred[cc] << (3 - picture->intra_dc_precision);

    if (picture->intra_vlc_format)
	slice_get_intra_block_B15 (picture, slice, DCTblock);
    else
	slice_get_intra_block_B14 (picture, slice, DCTblock);
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
} while (0)

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
	if (! (bitstream_show () & 0xffe00000))
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
