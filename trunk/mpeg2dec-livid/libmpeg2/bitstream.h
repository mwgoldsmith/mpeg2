/* 
 *  bitstream.h
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

extern uint_32 bitstream_buffer;
extern uint_32 bitstream_avail_bits;
extern uint_8 * bitstream_ptr;

void bitstream_init(uint_8 *start);

static inline uint_32 getbyte (void)
{
	return *(bitstream_ptr++);
}

static inline void needbits (uint_32 num_bits)
{
	while (bitstream_avail_bits < num_bits)
	{
		bitstream_buffer |= getbyte() << (24 - bitstream_avail_bits);
		bitstream_avail_bits += 8;
	}
}

static inline void dumpbits (uint_32 num_bits)
{
	bitstream_buffer <<= num_bits;
	bitstream_avail_bits -= num_bits;
}

static inline uint_32 
bitstream_show(uint_32 num_bits)
{
	needbits (num_bits);
	return bitstream_buffer >> (32 - num_bits);
}

static inline uint_32 
bitstream_get(uint_32 num_bits)
{
	uint_32 result;

	needbits (num_bits);
	result = bitstream_buffer >> (32 - num_bits);
	dumpbits (num_bits);

	return result;
}

static inline void 
bitstream_flush(uint_32 num_bits)
{
	needbits (num_bits);
	dumpbits (num_bits);
}
