/*
  definitions for video library
  Copyright (C) 2000  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file COPYRIGHT in this package

 */


frame_t * libvo_common_alloc (int width, int height);
void libvo_common_free (frame_t * frame);

#define __LIBVO_EXTERN(x,id) \
vo_output_video_t video_out_##x = {\
	name: id,\
        close:                  x ## _close,\
	setup:                  x ## _setup,\
	draw_frame:             x ## _draw_frame,\
	draw_slice:             x ## _draw_slice,\
	flip_page:              x ## _flip_page,\
	allocate_image_buffer:  x ## _allocate_image_buffer,\
	free_image_buffer:      x ## _free_image_buffer,\
        overlay:                x ## _overlay\
};

#ifdef __OMS__
#define LIBVO_EXTERN(x,id) \
	__LIBVO_EXTERN (x, id) \
	vo_output_video_t * video_out_oms_plugin = &video_out_##x;
#else
#define LIBVO_EXTERN(x,id) __LIBVO_EXTERN(x,id)
#endif
