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
	picture.current_frame[0] = malloc(frame_size);
	picture.current_frame[1] = malloc(frame_size / 4);
	picture.current_frame[2] = malloc(frame_size / 4);

	//FIXME setup config properly
	config.flags = MPEG2_MMX_ENABLE;

	//intialize the decoder state (ie the parser knows best)
	parse_state_init(&picture);
	idct_init();
	motion_comp_init();
	mb = mb_buffer_init(CHROMA_420);

}

uint_32 frame_counter = 0;
void
mpeg2_decode(void) 
{
	uint_32 mba;      //macroblock address
	uint_32 last_mba; //last macroblock in frame
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

	//XXX We only do I-frames now
	if( picture.picture_coding_type != I_TYPE) 
		return;

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
		//printf("starting mba %d of %d  mbwidth=%d\n",mba,last_mba,mb_width);
		
		parse_slice_header(&picture,&slice);
		do
		{
			mba_inc = Get_macroblock_address_increment();

			if(mba_inc > 1)
				for(i=0; i< mba_inc - 1; i++)
				{
					mb->skipped = 1;
					mb->mba = ++mba;
					mb = mb_buffer_increment();
				}
			
			mb->skipped = 0;
			mb->mba = ++mba; 

			parse_macroblock(&picture,&slice,mb);
			mb = mb_buffer_increment();

			if(!mb)
				decode_flush_buffer();
		}
		while(bitstream_show(23));
	}
	while(mba < last_mba);
	
	decode_flush_buffer();
	display_frame(picture.current_frame);

	if(bitstream_show(32) == SEQUENCE_END_CODE)
		is_sequence_needed = 1;

	printf("frame_counter = %d\n",frame_counter++);
}

uint_32 buf[2048/4];
FILE *in_file;
 
void fill_buffer(uint_32 **start,uint_32 **end)
{
	uint_32 bytes_read;

	bytes_read = fread(buf,1,2048,in_file);

	*start = buf;
	*end   = buf + bytes_read/4;

	if(bytes_read != 2048)
		exit(1);
}

int main(int argc,char *argv[])
{

	if(argc < 2)
	{
		fprintf(stderr,"usage: %s video_stream\n",argv[0]);
		exit(1);
	}

	printf(PACKAGE"-"VERSION" (C) 1999 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>\n");

	if(argv[1][0] != '-')
	{
		in_file = fopen(argv[1],"r");	

		if(!in_file)
		{
			fprintf(stderr,"%s - Couldn't open file ",argv[1]);
			perror(0);
			exit(1);
		}
	}
	else
		in_file = stdin;

	bitstream_init(fill_buffer);
	//FIXME this doesn't go here later
	mpeg2_init();

	while(1)
		mpeg2_decode();

  return 0;
}

