/*
 *  decode.c
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
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "config.h"
#include "old_crap.h"
#include "mpeg2.h"
#include "mpeg2_internal.h"

#include "mb_buffer.h"
#include "motion_comp.h"
#include "bitstream.h"
#include "idct.h"
#include "parse.h"
#include "display.h"
#include "inv_quantize.h"

//this is where we keep the state of the decoder
static picture_t picture;
static slice_t slice;
static macroblock_t *mb;

//global config struct
mpeg2_config_t config;
//frame structure to pass back to caller
mpeg2_frame_t mpeg2_frame;

static uint_32 is_display_initialized = 0;
static uint_32 is_sequence_needed = 1;

static void find_next_start_code()
{
	uint_32 code;

	bitstream_byte_align();
  while ((code = bitstream_show(24))!=0x01L)
	{
		//fprintf(stderr,"tried %08x\n",code);
    bitstream_flush(8);
	}
}

//find a picture header and parse anything else we happen to find 
//on the way
//FIXME this is crap
uint_32
decode_find_header(uint_32 type,picture_t *picture)
{
	uint_32 code;

	while(1)
	{

		//printf("looking for %08x\n",type);
		// look for next_start_code 
		find_next_start_code();
		code = bitstream_get(32);
	
		//we found the header we're looking for
		if(code == type)
			return code;

		//we're looking for a slice header 
		if(code <= SLICE_START_CODE_MAX && code >= SLICE_START_CODE_MIN &&
				type == SLICE_START_CODE_MIN)
			return code;
		
		//deal with the header otherwise
		switch (code)
		{
			case GROUP_START_CODE:
				parse_gop_header(picture);
				break;
			case USER_DATA_START_CODE:
				parse_user_data();
				break;
			case EXTENSION_START_CODE:
				parse_extension(picture);
				break;

			//FIXME add in the other strange extension headers
			default:
			//	fprintf(stderr,"Unsupported header 0x%08x\n",code);
		}
	}
}

static void decode_flush_buffer(void)
{
	mb_buffer_t mb_buffer;

	mb_buffer_flush(&mb_buffer);

	idct(&mb_buffer);
	motion_comp(&picture,&mb_buffer);

	//reset mb pointer for next slice
	mb = mb_buffer.macroblocks;
}

void
mpeg2_init(void)
{
	uint_32 frame_size;

	//FIXME this should go somewhere after we discover how big
	//the frame is, or size it so that it will be big enough for
	//all cases
	frame_size = 720 * 576;
	picture.throwaway_frame[0] = malloc(frame_size);
	picture.throwaway_frame[1] = malloc(frame_size / 4);
	picture.throwaway_frame[2] = malloc(frame_size / 4);
	picture.backward_reference_frame[0] = malloc(frame_size);
	picture.backward_reference_frame[1] = malloc(frame_size / 4);
	picture.backward_reference_frame[2] = malloc(frame_size / 4);
	picture.forward_reference_frame[0] = malloc(frame_size);
	picture.forward_reference_frame[1] = malloc(frame_size / 4);
	picture.forward_reference_frame[2] = malloc(frame_size / 4);

	//FIXME setup config properly
	config.flags = MPEG2_MMX_ENABLE;

	//intialize the decoder state (ie the parser knows best)
	parse_state_init(&picture);
	idct_init();
	motion_comp_init();
	mb = mb_buffer_init(CHROMA_420);

}


void
decode_reorder_frames(void)
{
	uint_8 *tmp[3];

	if(picture.picture_coding_type != B_TYPE)
	{
		//reuse the soon to be outdated forward reference frame
		picture.current_frame[0] = picture.forward_reference_frame[0];
		picture.current_frame[1] = picture.forward_reference_frame[1];
		picture.current_frame[2] = picture.forward_reference_frame[2];

		//make the backward reference frame the new forward reference frame
		tmp[0] = picture.forward_reference_frame[0];
		tmp[1] = picture.forward_reference_frame[1];
		tmp[2] = picture.forward_reference_frame[2];
		picture.forward_reference_frame[0] = picture.backward_reference_frame[0];
		picture.forward_reference_frame[1] = picture.backward_reference_frame[1];
		picture.forward_reference_frame[2] = picture.backward_reference_frame[2];
		picture.backward_reference_frame[0] = tmp[0];
		picture.backward_reference_frame[1] = tmp[1];
		picture.backward_reference_frame[2] = tmp[2];
	}
	else
	{
		picture.current_frame[0] = picture.throwaway_frame[0];
		picture.current_frame[1] = picture.throwaway_frame[1];
		picture.current_frame[2] = picture.throwaway_frame[2];
	}
}



mpeg2_frame_t*
mpeg2_decode_frame(void) 
{
	uint_32 mba;      //macroblock address
	uint_32 last_mba; //last macroblock in frame
	uint_32 prev_macroblock_type = 0; 
	uint_32 mba_inc;
	uint_32 mb_width;
	uint_32 code;
	uint_32 i;

	//
	//decode one frame
	//
	//
	if(is_sequence_needed)
	{
		decode_find_header(SEQUENCE_HEADER_CODE,&picture);
		parse_sequence_header(&picture);
		is_sequence_needed = 0;
	}

	
	decode_find_header(PICTURE_START_CODE,&picture);
	parse_picture_header(&picture);

	//XXX We dont' handle B-frames yet
	if( picture.picture_coding_type == B_TYPE) 
		return NULL;

	decode_reorder_frames();

	last_mba = ((picture.coded_picture_height * picture.coded_picture_width) >> 8) - 1;
	mb_width = picture.coded_picture_width >> 4;

	//we can't initialize the display until we know how big the picture is
	if(!is_display_initialized)
	{
		display_init(picture.coded_picture_width,picture.coded_picture_height);
		is_display_initialized = 1;
	}

	do
	{
		//find a slice start
		code = decode_find_header(SLICE_START_CODE_MIN,&picture);

		mba = ((code &0xff) - 1) * mb_width - 1;
		
		parse_reset_pmv(&slice);
		parse_slice_header(&picture,&slice);
		do
		{
			mba_inc = Get_macroblock_address_increment();

			if(mba_inc > 1)
			{
				//handling of skipped mb's differs between P_TYPE and B_TYPE
				//pictures
				if(picture.picture_coding_type == P_TYPE)
				{
					parse_reset_pmv(&slice);

					for(i=0; i< mba_inc - 1; i++)
					{
						memset(mb->f_motion_vectors,0,8);
						//FIXME i don't know why this doesn't work
						//mb->macroblock_type = MACROBLOCK_MOTION_FORWARD;
						mb->skipped = 1;
						mb->mba = ++mba;
						mb = mb_buffer_increment();
					}
				}
				else
				{
					for(i=0; i< mba_inc - 1; i++)
					{
						memcpy(mb->f_motion_vectors[0],slice.f_pmv,8);
						memcpy(mb->f_motion_vectors[1],slice.f_pmv,8);
						mb->macroblock_type = prev_macroblock_type;
						mb->skipped = 1;
						mb->mba = ++mba;
						mb = mb_buffer_increment();
					}
				}
			}
			
			mb->skipped = 0;
			mb->mba = ++mba; 

			parse_macroblock(&picture,&slice,mb);
			//we store the last macroblock mv flags, as skipped b-frame blocks
			//inherit them
			prev_macroblock_type = mb->macroblock_type & (MACROBLOCK_MOTION_FORWARD | MACROBLOCK_MOTION_BACKWARD);
			mb = mb_buffer_increment();

			if(!mb)
				decode_flush_buffer();
		}
		while(bitstream_show(23));
	}
	while(mba < last_mba);
	
	decode_flush_buffer();

	//decide which frame to send to the display
	if(picture.picture_coding_type == B_TYPE)
	{
		mpeg2_frame.frame[0] = picture.throwaway_frame[0];
		mpeg2_frame.frame[1] = picture.throwaway_frame[1];
		mpeg2_frame.frame[2] = picture.throwaway_frame[2];
	}
	else
	{
		mpeg2_frame.frame[0] = picture.forward_reference_frame[0];
		mpeg2_frame.frame[1] = picture.forward_reference_frame[1];
		mpeg2_frame.frame[2] = picture.forward_reference_frame[2];
	}

	if(bitstream_show(32) == SEQUENCE_END_CODE)
		is_sequence_needed = 1;

	return &mpeg2_frame;
}

