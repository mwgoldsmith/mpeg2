#ifndef __VIDEO_OUT_H
#define __VIDEO_OUT_H
/*
  definitions for video library
  Copyright (C) 2000  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file COPYRIGHT in this package

 */


typedef struct overlay_buf_struct {
        unsigned char *data;    // 7-4: mixer key, 3-0: color index
        unsigned int x;         // x start of subpicture area
        unsigned int y;         // y start of subpicture area
        int width;              // width of subpicture area
        int height;             // height of subpicture area

        unsigned char clut[4];  // color lookup table
        unsigned char trans[4]; // mixer key table

        unsigned int time_execute;     // time in ms
} overlay_buf_t;

typedef struct frame_s {
	uint8_t *base[3];	// pointer to 3 planes
	void *private;
} frame_t;

typedef struct vo_output_video_attr_s {
	int width;
	int height;
	uint32_t fullscreen;
	char *title;
} vo_output_video_attr_t;

#include "output_video.h"

typedef struct vo_output_video_s {
    PLUGIN_GENERIC;
    int (*setup)                (vo_output_video_attr_t *attr); 
    int (*overlay)              (overlay_buf_t *overlay_buf, int id); 
    int (*draw_frame)   (frame_t *frame); 
    int (*draw_slice)   (uint8_t *src[], int slice_num); 
    void        (*flip_page)    (void); 
    frame_t *(*allocate_image_buffer)(int width, int height, uint32_t format); 
    void    (*free_image_buffer)        (frame_t* image); 
} vo_output_video_t;

// NULL terminated array of all drivers 
extern vo_output_video_t* video_out_drivers[];
#endif
