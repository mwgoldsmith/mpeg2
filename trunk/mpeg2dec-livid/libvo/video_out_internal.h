/*
  definitions for video library
  Copyright (C) 2000  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file COPYRIGHT in this package

 */


int libvo_common_alloc_frames (vo_instance_t * this, int width, int height,
			       void (* draw) (vo_frame_t * frame));
void libvo_common_free_frames (vo_instance_t * this);
vo_frame_t * libvo_common_get_frame (vo_instance_t * this, int prediction);
