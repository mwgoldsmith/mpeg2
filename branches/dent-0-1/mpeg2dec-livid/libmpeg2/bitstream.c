/* 
 *  bitstream.c
 *
 *	Copyright (C) Aaron Holtzman - Dec 1999
 *
 *  This file is part of mpeg2dec, a free MPEG-2 video decoder
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
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include <bswap.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "mpeg2.h"
#include "mpeg2_internal.h"

#include "bitstream.h"

uint32_t bits_left;
uint64_t current_word;
uint64_t *buffer_start;
uint64_t *buffer_end;



/**
 *
 **/

static inline void bitstream_fill_current()
{
	current_word = bswap_64 (*buffer_start++);
}


//
// The fast paths for _show, _flush, and _get are in the
// bitstream.h header file so they can be inlined.
//
// The "bottom half" of these routines are suffixed _bh
//
// -ah
//


/**
 *
 **/

uint32_t bitstream_show_bh (uint32_t num_bits)
{
	uint32_t result;
	uint64_t next_word = bswap_64 (*buffer_start);

	result = (current_word << (64 - bits_left)) >> (64 - bits_left);
	num_bits -= bits_left;
	result = (result << num_bits) | (next_word >> (64 - num_bits));

	return result;
}


/**
 *
 **/

uint32_t bitstream_get_bh (uint32_t num_bits)
{
	uint32_t result;

	num_bits -= bits_left;
	result = (current_word << (64 - bits_left)) >> (64 - bits_left);

	bitstream_fill_current ();

	if(num_bits != 0)
		result = (result << num_bits) | (current_word >> (64 - num_bits));
	
	bits_left = 64 - num_bits;

	return result;
}


/**
 *
 **/

void bitstream_flush_bh (uint32_t num_bits)
{
	bits_left = (64 + bits_left) - num_bits; 
	bitstream_fill_current ();
}


/**
 *
 **/

void bitstream_byte_align (void)
{
	//byte align the bitstream
	bits_left &= ~7;

	if (!bits_left) {
		bits_left = 64;
		bitstream_fill_current ();
	}
}


/**
 *
 **/

void bitstream_init (uint8_t *start)
{
	buffer_start = (uint64_t*) start;
	buffer_end = buffer_start + 65536; //XXX hack

	bits_left = 64;
	current_word = bswap_64 (*buffer_start++);
}
