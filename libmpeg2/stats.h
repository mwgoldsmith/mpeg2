/*
 *  stats.h
 *
 *  Copyright (C) Aaron Holtzman <aholtzma@ess.engr.uvic.ca> - Nov 1999
 *
 *  Decodes an MPEG-2 video stream.
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
 
void stats_sequence_header(picture_t *picture);
void stats_gop_header(picture_t *picture);
void stats_picture_header(picture_t* picture);
void stats_slice_header(slice_t* slice);
void stats_macroblock(macroblock_t *mb);
void stats_picture_coding_ext_header(picture_t *picture);
void stats_sequence_ext(picture_t *picture);
void stats_sequence_display_ext(picture_t *picture);

//FIXME These still need fixins
void stats_quant_matrix_ext_header(picture_t *picture);
