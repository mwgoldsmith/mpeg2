/*
  definitions for video library
  Copyright (C) 2000  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file COPYRIGHT in this package

 */


int libvo_common_alloc_frame (frame_t * frame, int width, int height);
int libvo_common_alloc_frames (int (* alloc) (frame_t *, int, int),
			       int width, int height);
void libvo_common_free_frame (frame_t * frame);
void libvo_common_free_frames (void (* free_frame) (frame_t *));
frame_t * libvo_common_get_frame (vo_instance_t * this, int prediction);
