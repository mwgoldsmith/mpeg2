/*
  Copyright (C) 2000  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file COPYRIGHT in this package

 */

//
// Externally visible list of all vo drivers

#include "../video/output_video.h"

extern vo_output_video_t video_out_x11;
extern vo_output_video_t video_out_sdl;
extern vo_output_video_t video_out_3dfx;
extern vo_output_video_t video_out_null;
extern vo_output_video_t video_out_md5;
extern vo_output_video_t video_out_pgm;


vo_output_video_t* video_out_drivers[] = 
{
#ifdef HAVE_X11
        &video_out_x11,
#endif
#ifdef HAVE_SDL
        &video_out_sdl,
#endif
#ifdef HAVE_MGA
        &video_out_mga,
#endif
#ifdef HAVE_3DFX
        &video_out_3dfx,
#endif
#ifdef HAVE_SYNCFB
        &video_out_syncfb,
#endif
        &video_out_null,
        &video_out_pgm,
        &video_out_md5,
        NULL
};



