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
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <sys/errno.h>

#include "config.h"
#include "libmpeg2/mpeg2.h"
//#include "libvo/video_out.h"


#define BUFFER_SIZE 2048
static uint_8 buffer[BUFFER_SIZE];
static FILE *in_file;
static uint_32 frame_counter = 0;

static struct timeval tv_beg;
static struct timeval tv_end;
static float elapsed = 0;
static float total_elapsed = 0;
static uint_32 demux_dvd = 0;
static vo_functions_t *video_out;

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
 
static void 
fill_buffer(uint_32 num_bytes)
{
	uint_32 bytes_read;

	bytes_read = fread(buffer,1,num_bytes,in_file);

	if(bytes_read < num_bytes)
	{
		print_fps(1);
		exit(0);
	}
}

void
do_dvd_demux (uint_8 **buf_start, uint_8 **buf_end)
{
	uint_8 *start,*end;

	while(1)
	{
		//read in one sector
		fill_buffer(2048);

		// check pack start code
		if (buffer[0] || buffer[1] || (buffer[2] != 0x01) || (buffer[3] != 0xba))
		{
			printf ("bad pack start code - not a .vob file ?\n");
			exit (1);
		}

		// skip pack header
		start = buffer + 14 + (buffer[13] & 7);

		//there should be exactly one PES packet per sector in DVD stream
		end = start + (start[4] << 8) + start[5] + 6;

		if (end > buffer + 2048)
		{
			printf ("PES too big ?\n");
			exit (1);
		}

		if (start[3] == 0xe0)     // check for video ES bitstream
			break;
	}

	start += start[8] + 9;      // skip PES header

	*buf_start = start;
	*buf_end = end;
}


static void signal_handler(int sig)
{
	print_fps(1);
	signal(sig, SIG_DFL);
	raise(sig);
}

void
print_usage(char *argv[])
{
	uint_32 i = 0;

	fprintf(stderr,"usage: %s [-o mode] [-s] file\n"
	               "\t-s\tsystem stream (.vob file)\n"
	               "\t-o\tvideo_output mode\n",argv[0]);

	while (video_out_drivers[i] != NULL)
	{
		const vo_info_t *info;
		
		info = video_out_drivers[i++]->get_info();

		fprintf(stderr, "\t\t\t%s\t%s\n", info->short_name, info->name);
	}

	exit(1);
}

static void 
handle_args (int argc, char *argv[])
{
	char c;
	uint_32 i;

	while((c = getopt(argc,argv,"so:")) != EOF)
	{
		switch (c)
		{
			case 'o':
				for (i=0; video_out_drivers[i] != NULL; i++)
				{
					const vo_info_t *info = video_out_drivers[i]->get_info();

					if (strcmp(info->short_name,optarg) == 0)
						video_out = video_out_drivers[i];
				}
				if (video_out == NULL)
 				{
					fprintf(stderr,"Invalid video driver: %s\n", optarg);
					print_usage(argv);
				}
			break;

			case 's':
				demux_dvd = 1;
			break;

			default:
				print_usage(argv);
		}
	}

	// -o not specified, use a default driver 
	if (video_out == NULL)
	{
		video_out = video_out_drivers[0];
	}

	if (optind < argc)
	{
		in_file = fopen (argv[optind], "r");
		if (!in_file)
		{
			fprintf (stderr, "%s - couldnt open file %s\n", strerror (errno),
			argv[optind]);
			exit (1);
		}
	}
	else
		in_file = stdin;
}



int main(int argc,char *argv[])
{
	printf(PACKAGE"-"VERSION" (C) 2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>\n");

	handle_args(argc,argv);

	signal(SIGINT, signal_handler);

	mpeg2_init(video_out);

	gettimeofday(&tv_beg, NULL);

	while(1)
	{
		uint_32 num_frames;
		uint_8 *buf_start = buffer;
		uint_8 *buf_end = buffer + BUFFER_SIZE;
		
		if(demux_dvd)
			do_dvd_demux(&buf_start,&buf_end);
		else
			fill_buffer(BUFFER_SIZE);
		
		num_frames = mpeg2_decode_data(buf_start, buf_end);

		while(num_frames--)
			print_fps(0);

	}
  return 0;
}

