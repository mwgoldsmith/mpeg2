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
