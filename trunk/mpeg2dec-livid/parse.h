/*
 *  parse.h
 *
 *  Copyright (C) Aaron Holtzman <aholtzma@ess.engr.uvic.ca> - Nov 1999
 *
 *  This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
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
 *  the Free Software Foundation, 
 *
 */
 
void parse_macroblock(const picture_t* picture,slice_t* slice, macroblock_t *mb);
void parse_slice_header(const picture_t *picture, slice_t *slice);
void parse_picture_header(picture_t *picture);
void parse_state_init(picture_t *picture);
void parse_sequence_header(picture_t *picture);
void parse_gop_header(picture_t *picture);
void parse_extension(picture_t *picture);
void parse_user_data(void);
