/*
  definitions for video library
  Copyright (C) 2000  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file COPYRIGHT in this package

 */

typedef struct frame_s {
	uint8_t *base[3];	// pointer to 3 planes

	uint8_t used:1;		// determine whether the frame is being used
	void *private;
} frame_t;

typedef struct vo_output_video_attr_s {
	int width;
	int height;
	uint32_t fullscreen;
	char *title;
} vo_output_video_attr_t;

typedef struct vo_output_video_s {
    char * name; 
 
    int (*setup)                (vo_output_video_attr_t *attr); 
    int (*close) (void * this); 
 
// <old> 
    int (*draw_frame)   (frame_t *frame); 
    int (*draw_slice)   (uint8_t *src[], int slice_num); 
    void        (*flip_page)    (void); 
    frame_t *(*allocate_image_buffer)(int width, int height, uint32_t format); 
    void    (*free_image_buffer)        (frame_t* image); 
// </old> 
} vo_output_video_t;

// NULL terminated array of all drivers 
extern vo_output_video_t* video_out_drivers[];
