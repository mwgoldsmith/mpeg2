/*
 *  mpeg2dec.c
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

#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#include "mpeg2.h"

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
	uint_32 frame_counter = 0;
	mpeg2_frame_t *my_frame;

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

	//FIXME this doesn't go here later
	bitstream_init(fill_buffer);

	mpeg2_init();

	while(1)
	{
		my_frame = mpeg2_decode_frame();
		display_frame(my_frame->frame);
		printf("frame_counter = %d\n",frame_counter++);
	}

  return 0;
}

