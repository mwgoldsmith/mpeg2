/*
  definitions for video library
  Copyright (C) 2000  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file COPYRIGHT in this package

 */


int libvo_common_alloc_frames (vo_instance_t * instance, int width, int height,
			       int frame_size,
			       void (* copy) (vo_frame_t *, uint8_t **),
			       void (* field) (vo_frame_t *, int),
			       void (* draw) (vo_frame_t *));
void libvo_common_free_frames (vo_instance_t * instance);
vo_frame_t * libvo_common_get_frame (vo_instance_t * instance, int prediction);
