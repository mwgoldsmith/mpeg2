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
#include <signal.h>
#include <sys/time.h>
#include "config.h"
#include "libmpeg2/mpeg2.h"
#include "display.h"

uint_8 buf[2048];
FILE *in_file;
static uint_32 frame_counter = 0;

struct timeval tv_beg;
struct timeval tv_end;
float elapsed = 0;
float total_elapsed = 0;

static void print_fps(uint_32 final) 
{

	//XXX hackety hack hack...
#ifdef __i386__
		asm("emms"); 
#endif
	gettimeofday(&tv_end, NULL);
	
	elapsed += (tv_end.tv_sec - tv_beg.tv_sec) + 
			(tv_end.tv_usec - tv_beg.tv_usec) / 1000000.0;        
	
	tv_beg = tv_end;


	if(!((frame_counter)%200))
	{
		printf("|---------+---------+------------+-------+--------|\n");
		printf("|total t  | avg fps | frame #    | time  | fps    |\n");
		printf("|---------+---------+------------+-------+--------|\n");

	}

	if(!((++frame_counter)%10))
	{
		total_elapsed += elapsed;

		printf("| % 7.3g |  %3.03f | %4d - %4d| %3.03f | %3.03f |\n",
		total_elapsed, frame_counter / total_elapsed, 
		frame_counter-10,frame_counter,elapsed, 10 / elapsed);

		elapsed = 0;
	}
	if (final)
	{

		printf("|---------+---------+------------+-------+--------|\n");
		printf("| % 7.3g |  %3.03f |    0 - %4d| %3.03f | %3.03f |\n",
		total_elapsed, frame_counter / total_elapsed, 
		frame_counter,total_elapsed, frame_counter / total_elapsed);
		printf("|---------+---------+------------+-------+--------|\n");
	}
}
 
static void signal_handler(int sig)
{
	print_fps(1);
	signal(sig, SIG_DFL);
	raise(sig);
}
void fill_buffer(uint_8 **start,uint_8 **end)
{
	uint_32 bytes_read;

	bytes_read = fread(buf,1,2048,in_file);

	*start = buf;
	*end   = buf + bytes_read;

	if(bytes_read < 2048)
	{
		print_fps(1);
		exit(0);
	}
}

int main(int argc,char *argv[])
{
	mpeg2_frame_t *my_frame = NULL;

	if(argc < 2)
	{
		fprintf(stderr,"usage: %s video_stream\n",argv[0]);
		exit(1);
	}

	printf(PACKAGE"-"VERSION" (C) 2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>\n");

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


	//FIXME this should go in mpeg2_init
	bitstream_init(fill_buffer);

	mpeg2_init();

	signal(SIGINT, signal_handler);

	gettimeofday(&tv_beg, NULL);

	while (my_frame == NULL)    // better safe than sorry.
		my_frame = mpeg2_decode_frame();

	display_init(my_frame->width,my_frame->height);

	while(1)
	{
		if(my_frame != NULL) 
		{
			display_frame(my_frame->frame);
			print_fps(0);
		}
		my_frame = mpeg2_decode_frame();
	}
  return 0;
}

