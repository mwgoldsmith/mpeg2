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

void (*bitstream_fill_buffer)(uint_32**,uint_32**);

uint_32 mask[33] = 
{
	0x0,
	0x00000001, 0x00000003, 0x00000007, 0x0000000f, 
	0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff, 
	0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff, 
	0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff, 
	0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff, 
	0x00ffffff, 0x003fffff, 0x007fffff, 0x00ffffff, 
	0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff, 
	0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff, 
};

uint_32 fast_count=0;
uint_32 slow_count=0;

inline uint_32 
bitstream_show(uint_32 num_bits)
{
	uint_32 result;
	//fprintf(stderr,"(show) buffer_start %p, buffer_end %p, current_word 0x%08x, next_word 0x%08x, num_bits %d, bits_left %d\n",buffer_start,buffer_end,current_word,next_word,num_bits,bits_left);

	//fast path
	if(num_bits < bits_left)
	{
		//fast_count++;
		//printf("fast_count = %d slow_count = %d total = %d\n",fast_count,slow_count, fast_count + slow_count);
		return (current_word  >> (bits_left - num_bits)) & mask[num_bits];
	}

	//slow_count++;
	//printf("fast_count = %d slow_count = %d total = %d\n",fast_count,slow_count, fast_count + slow_count);
	//
	if(num_bits == bits_left)
		result = current_word & mask[num_bits];
	else
	{
		result = (current_word  << (num_bits - bits_left)) & mask[num_bits];
		result |= (next_word  >> (bits_left - num_bits)) & mask[num_bits];
	}
	return result;
}


static inline void
bitstream_fill_next()
{
	//fprintf(stderr,"(fill) buffer_start %p, buffer_end %p, current_word 0x%08x, bits_left %d\n",buffer_start,buffer_end,current_word,bits_left);

	next_word = *buffer_start++;
	next_word = swab32(next_word);

	if(buffer_start == buffer_end)
	{
		bitstream_fill_buffer(&buffer_start,&buffer_end);
	}
}

// Fetches 1-32 bits bitstream buffer
//
// Minimized the number of writes by using a bitmask. 
inline uint_32
bitstream_get(uint_32 num_bits)
{
	uint_32 result;

	//fprintf(stderr,"(get) buffer_start %p, buffer_end %p, current_word 0x%08x, next_word 0x%08x, num_bits %d,bits_left %d\n",buffer_start,buffer_end,current_word,next_word,num_bits,bits_left);
	//fast path
	if(num_bits < bits_left)
		return (current_word  >> (bits_left -= num_bits)) & mask[num_bits];

	if(num_bits == bits_left)
	{
		result = current_word & mask[num_bits];
		current_word = next_word;
		bits_left = 32;
		bitstream_fill_next();
	}
	else
	{
		result = (current_word  << (num_bits - bits_left)) & mask[num_bits];
		current_word = next_word;
		result |= (next_word  >> (32 - num_bits + bits_left)) & mask[num_bits];
		bits_left = 32 - num_bits + bits_left;
		bitstream_fill_next();
	}

	return result;
}

inline void 
bitstream_flush(uint_32 num_bits)
{
	//fprintf(stderr,"(flush) buffer_start %p, buffer_end %p, current_word 0x%08x, next_word 0x%08x, num_bits %d,bits_left %d\n",buffer_start,buffer_end,current_word,next_word,num_bits,bits_left);
	//fast path
	if(num_bits < bits_left)
	{
		bits_left -= num_bits;
		return;
	}

	if(num_bits == bits_left)
	{
		current_word = next_word;
		bits_left = 32;
		bitstream_fill_next();
	}
	else
	{
		current_word = next_word;
		bits_left = 32 - num_bits + bits_left;
		bitstream_fill_next();
	}
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
bitstream_init(void(*fill_function)(uint_32**,uint_32**))
{
	//fprintf(stderr,"(fill_buffer) buffer buffer_start %p, buffer_end %p, current_word 0x%08x, bits_left %d\n",buffer_start,buffer_end,current_word,bits_left);
	// Setup the buffer fill callback 
	bitstream_fill_buffer = fill_function;

	bitstream_fill_buffer(&buffer_start,&buffer_end);

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

