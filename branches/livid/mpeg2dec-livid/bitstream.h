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

//My new and improved vego-matic endian swapping routine
//(stolen from the kernel)
#ifdef WORDS_BIGENDIAN

#	define swab32(x) (x)

#elsif defined (__i386__)

#	define swab32(x) __i386_swab32(x)
	static inline const __u32 __i386_swab32(__u32 x)
	{
		__asm__("bswap %0" : "=r" (x) : "0" (x));
		return x;
	}

#else

#	define swab32(x)\
((((uint_8*)&x)[0] << 24) | (((uint_8*)&x)[1] << 16) |  \
 (((uint_8*)&x)[2] << 8)  | (((uint_8*)&x)[3]))

#endif



inline uint_32 bitstream_show(uint_32 num_bits);
inline uint_32 bitstream_get(uint_32 num_bits);
inline void bitstream_flush(uint_32 num_bits);
void bitstream_init(void(*fill_function)(uint_32**,uint_32**));
void bitstream_byte_align(void);


#define Get_Bits(x) bitstream_get((x))
#define Get_Bits1() bitstream_get(1)
#define Get_Bits32() bitstream_get(32)
#define Show_Bits(x) bitstream_show((x))
#define Flush_Buffer(x) bitstream_flush((x))
#define Flush_Buffer32() bitstream_flush(32)
