/* 
 * bitstream.h
 *
 *	Copyright (C) Aaron Holtzman - Dec 1999
 *
 * This file is part of mpeg2dec, a free MPEG-2 video decoder
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
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

// common variables - they are shared between getvlc.c and slice.c
// they should be put elsewhere !
uint32_t bitstream_buffer;
int32_t bitstream_avail_bits;
uint8_t * bitstream_ptr;

static inline void bitstream_init (uint8_t *start)
{
	bitstream_ptr = start;
	bitstream_avail_bits = 16;
	bitstream_buffer = 0; 
}

static inline uint32_t getword (void)
{
	uint32_t value;

	value = (bitstream_ptr[0] << 8) | bitstream_ptr[1];
	bitstream_ptr += 2;
	return value;
}

static inline void needbits (uint32_t num_bits)
{
	if (bitstream_avail_bits > 0)
	{
		bitstream_buffer |= getword () << bitstream_avail_bits;
		bitstream_avail_bits -= 16;
	}
}

static inline void dumpbits (uint32_t num_bits)
{
	bitstream_buffer <<= num_bits;
	bitstream_avail_bits += num_bits;
}

static inline uint32_t 
bitstream_show (uint32_t num_bits)
{
	needbits (num_bits);
	return bitstream_buffer >> (32 - num_bits);
}

static inline uint32_t 
bitstream_get (uint32_t num_bits)
{
	uint32_t result;

	needbits (num_bits);
	result = bitstream_buffer >> (32 - num_bits);
	dumpbits (num_bits);

	return result;
}

static inline void 
bitstream_flush (uint32_t num_bits)
{
	// assume we only ever flush bits that we have already shown
	//needbits (num_bits);

	dumpbits (num_bits);
}
