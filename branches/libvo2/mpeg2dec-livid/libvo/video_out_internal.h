/*
  definitions for video library
  Copyright (C) 2000  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file COPYRIGHT in this package

 */

#define VIDEO_BUFFER_SIZE 5

frame_t * libvo_common_alloc (int width, int height);
void libvo_common_free (frame_t * frame);

frame_t * request_frame (void);
void release_frame(frame_t *frame);

// static? isn't this evil?
static frame_t *video_buffer[VIDEO_BUFFER_SIZE];

#define LIBVO_EXTERN(x,id) \
vo_output_video_t video_out_##x = {\
	name: id,\
        close:                  x ## _close,\
	setup:                  x ## _setup,\
	draw_frame:             x ## _draw_frame,\
	draw_slice:             x ## _draw_slice,\
	flip_page:              x ## _flip_page,\
	allocate_image_buffer:  x ## _allocate_image_buffer,\
	free_image_buffer:      x ## _free_image_buffer,\
	request_frame:		request_frame,\
	release_frame:          release_frame\
};
