/*
  definitions for video library
  Copyright (C) 2000  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file COPYRIGHT in this package

 */


void libvo_common_alloc_frames (int width, int height);
void libvo_common_free_frames (void);
frame_t * libvo_common_get_frame (int prediction);
