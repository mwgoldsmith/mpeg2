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
#include "mpeg2.h"
#include "mpeg2_internal.h"

#include "bitstream.h"

uint_32 bits_left;
uint_32 current_word;
uint_32 next_word;
uint_32 *buffer_start;
uint_32 *buffer_end;


void (*bitstream_fill_buffer)(uint_8**,uint_8**);

//FIXME this is a hack to make non word aligned and non
//multiple of four chunks work.
uint_8 *chunk_start = 0, *chunk_end = 0;
uint_8 big_buffer[65536];

void
bitstream_fill_big_buffer(void)
{
	uint_32 bytes_read;
	uint_32 num_bytes;

	bytes_read = 0;

	do
	{
		if(chunk_end == chunk_start)
			bitstream_fill_buffer(&chunk_start,&chunk_end);

		num_bytes = chunk_end - chunk_start;

		if(bytes_read + num_bytes > 65536)
			num_bytes = 65536 - bytes_read;

		memcpy(&big_buffer[bytes_read], chunk_start, num_bytes);

		bytes_read += num_bytes;
		chunk_start += num_bytes;
	} 
	while (bytes_read != 65536);

	buffer_start = (uint_32*)  big_buffer;
	buffer_end   = (uint_32*) (big_buffer + 65536);
}
//end hack

static inline void
bitstream_fill_next()
{
	//fprintf(stderr,"(fill) buffer_start %p, buffer_end %p, current_word 0x%08x, bits_left %d\n",buffer_start,buffer_end,current_word,bits_left);

	next_word = *buffer_start++;
	next_word = swab32(next_word);

	if(buffer_start == buffer_end)
		bitstream_fill_big_buffer();
}

//
// The fast paths for _show, _flush, and _get are in the
// bitstream.h header file so they can be inlined.
//
// The "bottom half" of these routines are suffixed _bh
//
// -ah
//

uint_32 
bitstream_show_bh(uint_32 num_bits)
{
	uint_32 result;

	result = (current_word << (32 - bits_left)) >> (32 - bits_left);
	num_bits -= bits_left;
	result = (result << num_bits) | (next_word >> (32 - num_bits));

	return result;
}

uint_32
bitstream_get_bh(uint_32 num_bits)
{
	uint_32 result;

	num_bits -= bits_left;
	result = (current_word << (32 - bits_left)) >> (32 - bits_left);

	if(num_bits != 0)
		result = (result << num_bits) | (next_word >> (32 - num_bits));
	
	current_word = next_word;
	bits_left = 32 - num_bits;
	bitstream_fill_next();

	return result;
}

void 
bitstream_flush_bh(uint_32 num_bits)
{
	//fprintf(stderr,"(flush) current_word 0x%08x, next_word 0x%08x, bits_left %d, num_bits %d\n",current_word,next_word,bits_left,num_bits);
	
	current_word = next_word;
	bits_left = (32 + bits_left) - num_bits; 
	bitstream_fill_next();
}

void 
bitstream_byte_align(void)
{
	//byte align the bitstream
	bits_left = bits_left & ~7;
	if(!bits_left)
	{
		current_word = next_word;
		bits_left = 32;
		bitstream_fill_next();
	}
}

void
bitstream_init(void(*fill_function)(uint_8**,uint_8**))
{
	//fprintf(stderr,"(fill_buffer) buffer buffer_start %p, buffer_end %p, current_word 0x%08x, bits_left %d\n",buffer_start,buffer_end,current_word,bits_left);
	// Setup the buffer fill callback 
	bitstream_fill_buffer = fill_function;

	bitstream_fill_big_buffer();

	bits_left = 32;
	current_word = *buffer_start++;
	current_word = swab32(current_word);
	next_word    = *buffer_start++;
	next_word    = swab32(next_word);
}

uint_32 bitstream_done()
{
	//FIXME
	return 0;
}

