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


extern uint32_t bits_left;
extern uint64_t current_word;

void bitstream_init(uint8_t *start);
void bitstream_byte_align(void);
inline uint32_t bitstream_show_bh(uint32_t num_bits);
inline uint32_t bitstream_get_bh(uint32_t num_bits);
inline void bitstream_flush_bh(uint32_t num_bits);

static inline uint32_t bitstream_show(uint32_t num_bits)
{
	if(num_bits <= bits_left)
		return (current_word << (64 - bits_left)) >> (64 - num_bits);
	
	return bitstream_show_bh(num_bits);
}

static inline uint32_t bitstream_get(uint32_t num_bits)
{
	uint32_t result;

	if(num_bits < bits_left) {
		result = (current_word << (64 - bits_left)) >> (64 - num_bits);
		bits_left -= num_bits;
		return result;
	}

	return bitstream_get_bh(num_bits);
}

static inline void bitstream_flush(uint32_t num_bits)
{
	if(num_bits < bits_left)
		bits_left -= num_bits;
	else
		bitstream_flush_bh(num_bits);
}
