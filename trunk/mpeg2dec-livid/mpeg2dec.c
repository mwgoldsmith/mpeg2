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


#define BUF_SIZE 2048
uint_8 buf[BUF_SIZE];
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

static mpeg2_display_t my_display = 
{
	display_init,
	display_frame,
	display_slice,
	display_flip_page,
	display_allocate_buffer
};

int main(int argc,char *argv[])
{
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

	signal(SIGINT, signal_handler);

	mpeg2_init(&my_display);

	gettimeofday(&tv_beg, NULL);

	while(1)
	{
		uint_32 bytes_read;
		uint_32 num_frames;
		
		bytes_read = fread(buf,1,BUF_SIZE,in_file);
		
		num_frames = mpeg2_decode_data(buf, buf + bytes_read);

		while(num_frames--)
			print_fps(0);

		if(bytes_read < BUF_SIZE)
		{
			print_fps(1);
			exit(0);
		}
	}
  return 0;
}

