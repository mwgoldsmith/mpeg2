/*
 *  video_out_sdl.c
 *
 *  Copyright (C) Dominik Schnitzer <aeneas@linuxvideo.org> - Janurary 13, 2001.
 *  Copyright (C) Ryan C. Gordon <icculus@lokigames.com> - April 22, 2000.
 *
 *  A mpeg2dec display driver that does output through the
 *  Simple DirectMedia Layer (SDL) library. This effectively gives us all
 *  sorts of output options: X11, SVGAlib, fbcon, AAlib, GGI. Win32, MacOS
 *  and BeOS support, too. Yay. SDL info, source, and binaries can be found
 *  at http://www.libsdl.org/
 *
 *  This file is part of oms, free DVD and Video player.
 *  It is derived from the mpeg2dec SDL video output plugin.
 *  Adopted for mpeg2dec from Dominik Schnitzer, Jan. 24. 2001
 *	
 *  oms is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  this oms output plugin is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 
 *
 *  Changes:
 *    Dominik Schnitzer <dominik@schnitzer.at> - November 08, 2000.
 *    - Added resizing support, fullscreen: chnaged the sdlmodes selection
 *       routine.
 *    - SDL bugfixes: removed the atexit(SLD_Quit), SDL_Quit now resides in
 *       the plugin_exit routine.
 *    - Commented the source :)
 *    - Shortcuts: for switching between Fullscreen/Windowed mode and for
 *       cycling between the different Fullscreen modes.
 *    - Small bugfixes: proper width/height of movie
 *    Dominik Schnitzer <dominik@schnitzer.at> - November 11, 2000.
 *    - Cleanup code, more comments
 *    - Better error handling
 *    Dominik Schnitzer <dominik@schnitzer.at> - December 17, 2000.
 *    - sdl_close() cleans up everything properly.
 *    - integrated Bruno Barreyra <barreyra@ufl.edu>'s fix which eliminates
 *       those memcpy()s for whole frames (sdl_draw_frame()) HACKMODE 2 patch
 *    - support for YV12 and IYUV format :>
 *    - minor fixups for future enhancements
 *    Dominik Schnitzer <dominik@schnitzer.at> - Janurary 13, 2001.
 *    - bugfix: Double buffered screenmodes made sdl output display nothing but
 *       a black screen.
 *    Dominik Schnitzer <dominik@schnitzer.at> - Janurary 20, 2001.
 *    - subpictures work! Properly! We even time the subpictures displayed they're
 *       still just blue, but it'll 100% work soon :)
 *    - switch between fullscreen and windowed mode now possible everytime, even
 *       when playback stopped
 *    - we keep the last frame displayed, so window content doesn't wash away
 *       when you i.e. resize the window when stopped. looks pretty cool.
 *    Dominik Schnitzer <dominik@schnitzer.at> - Janurary 21, 2001.
 *    - corrected some threading issues (now using mutexes)
 *    - chapter switching now works flawless on SDL side (!), _setup is checking
 *       if it was already called. no need to run _setup() twice.
 *    Dominik Schnitzer <dominik@schnitzer.at> - Janurary 24, 2001.
 *    - many changes to make sdl vo work with mpeg2dec
 *    Dominik Schnitzer <dominik@schnitzer.at> - Janurary 25, 2001.
 *    - sync with mpeg2dec vo, sudden changes in the vo interface
 *    - AAlib output added, Software only output added
 * 
 */

#include "config.h"

#ifdef LIBVO_SDL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <SDL/SDL.h>

#include "video_out.h"
#include "video_out_internal.h"

/** Private SDL Data structure **/

typedef struct sdl_frame_s {
    vo_frame_t vo;
    SDL_Overlay * overlay;
} sdl_frame_t;

typedef struct sdl_instance_s {
    vo_instance_t vo;
    int prediction_index;
    sdl_frame_t frame[3];

    /* SDL YUV surface & overlay */
    SDL_Surface *surface;
	
    /* surface attributes for windowed mode */
    Uint32 sdlflags;

    /* Bits per Pixel */
    Uint8 bpp;

} sdl_instance_t;

static sdl_instance_t sdl_static_instance;	/* FIXME */

extern vo_instance_t sdl_vo_instance;

/** libvo Plugin functions **/


/**
 * Checks for SDL window resize events.
 *
 *   params : none
 *  returns : doesn't return
 **/

static void check_events (void)
{
    sdl_instance_t * this = &sdl_static_instance;
    SDL_Event event;
	
    /* capture window resize events */
    while (SDL_PollEvent (&event)) {
	if (event.type == SDL_VIDEORESIZE)
	    this->surface = SDL_SetVideoMode (event.resize.w, event.resize.h,
					      this->bpp, this->sdlflags);
	if (event.type == SDL_KEYDOWN)
	    SDL_Delay (4000);
    }
}


/**
 * Draw a frame to the SDL YUV overlay and checks for events.
 *
 *   params : frame == vo_frame_t to draw
 *  returns : doesn't return
 **/

static void sdl_draw_frame (vo_frame_t * _frame)
{
    sdl_frame_t * frame;
    sdl_instance_t * this;

    frame = (sdl_frame_t *)_frame;
    this = (sdl_instance_t *)frame->vo.this;
	
    /* blit to the YUV overlay */
    SDL_DisplayYUVOverlay (frame->overlay, &(this->surface->clip_rect));
	
    /* check for events */
    check_events ();
	
    /* Unlock the frame - the frame is 100% filled with data to display 
     * We Lock it again when the frame was displayed. */
    SDL_UnlockYUVOverlay (frame->overlay);
}

static int sdl_alloc_frames (sdl_instance_t * this, int width, int height)
{
    int i;

    for (i = 0; i < 3; i++) {

	/* This is a bug somewhere else height == width, and width == height */
	this->frame[i].overlay = SDL_CreateYUVOverlay (width, height,
						       SDL_YV12_OVERLAY,
						       this->surface);
	if (this->frame[i].overlay == NULL)
	    return 1;
	this->frame[i].vo.base[0] = (this->frame[i].overlay->pixels[0]);
	this->frame[i].vo.base[1] = (this->frame[i].overlay->pixels[2]);
	this->frame[i].vo.base[2] = (this->frame[i].overlay->pixels[1]);
	this->frame[i].vo.draw = sdl_draw_frame;
	this->frame[i].vo.this = (vo_instance_t *)this;

	/* Locks the allocated frame, to allow writing to it.
	 * sdl_flip Unlocks it. sdl_draw_frame Locks it again.*/
	SDL_LockYUVOverlay (this->frame[i].overlay);	
    }
	
    return 0;
}


static void sdl_free_frames (sdl_instance_t * this)
{
    int i;

    for (i = 0; i < 3; i++)
	SDL_FreeYUVOverlay (this->frame[i].overlay);
}

vo_frame_t * sdl_get_frame (vo_instance_t * _this, int prediction)
{
    sdl_instance_t * this;

    this = (sdl_instance_t *)_this;

    if (!prediction)
	return (vo_frame_t *)(this->frame + 2);
    else {
	this->prediction_index ^= 1;
	return (vo_frame_t *)(this->frame + this->prediction_index);
    }
}

/**
 * Open and prepare SDL output.
 *
 *    params : this == instance to initialize
 *             width, height == width and height of output window
 *   returns : NULL on failure, a valid vo_instance_t on success
 **/

static vo_instance_t * sdl_setup (vo_instance_t * _this, int width, int height)
{
    sdl_instance_t * this;
    const SDL_VideoInfo * vidInfo = NULL;

    if (_this != NULL)
	return NULL;
	
    this = &sdl_static_instance;

    /* Cleanup YUV Overlay structures */
    this->surface = NULL;
	
    /* Reset the configuration vars to its default values */
    this->bpp = 0;
    this->sdlflags = SDL_HWSURFACE|SDL_RESIZABLE;

    /* initialize the SDL Video system */
    if (SDL_Init (SDL_INIT_VIDEO)) {
	fprintf (stderr, "sdl video initialization failed.\n");
	return NULL;
    }

    /* get information about the graphics adapter */
    vidInfo = SDL_GetVideoInfo ();
	
    /* test for normal resizeable & windowed hardware accellerated surfaces */
    if (!SDL_ListModes (vidInfo->vfmt, this->sdlflags)) {
		
	/*
	 * test for NON hardware accelerated resizeable surfaces - poor you. 
	 * That's all we have. If this fails there's nothing left.
	 * Theoretically there could be Fullscreenmodes left -
	 * we ignore this for now.
	 */
	this->sdlflags &= ~SDL_HWSURFACE;
	if (!SDL_ListModes (vidInfo->vfmt, this->sdlflags)) {
	    fprintf(stderr, "sdl couldn't get any acceptable sdl videomode for output.\n");
	    return NULL;
	}
    }

   /*
    * YUV overlays need at least 16-bit color depth, but the
    * display might less. The SDL AAlib target says it can only do
    * 8-bits, for example. So, if the display is less than 16-bits,
    * we'll force the BPP to 16, and pray that SDL can emulate for us.
    */
    this->bpp = vidInfo->vfmt->BitsPerPixel;
    if (this->bpp < 16) {
	fprintf(stderr, "sdl has to emulate a 16 bit surfaces, that will slow things down.\n");
	this->bpp = 16;  
    }
	
    /* We dont want those in our event queue */
    SDL_EventState(SDL_ACTIVEEVENT, SDL_IGNORE);
    SDL_EventState(SDL_KEYUP, SDL_IGNORE);
    SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
    SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_IGNORE);
    SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE); 
    SDL_EventState(SDL_QUIT, SDL_IGNORE);
    SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
	
    SDL_WM_SetCaption ("sdl video out", NULL);

    if (!(this->surface = SDL_SetVideoMode (width, height, this->bpp,
					    this->sdlflags))) {
	fprintf (stderr, "sdl could not set the desired video mode\n");
	return NULL;
    }

    if (sdl_alloc_frames (this, width, height)) {
	fprintf (stderr, "sdl could not allocate frame buffers\n");
	return NULL;
    }
	 
    this->vo = sdl_vo_instance;
    return (vo_instance_t *)this;
}


/**
 * Open and prepare SDL output for dedicated Software and aalib out
 *
 *    params : this == instance to initialize
 *             width, height == width and height of output window
 *   returns : NULL on failure, a valid vo_instance_t on success
 **/

vo_instance_t * vo_sdl_setup (vo_instance_t * _this, int width, int height)
{
    setenv("SDL_VIDEO_YUV_HWACCEL", "1", 1);
    setenv("SDL_VIDEO_X11_NODIRECTCOLOR", "1", 1);
    return sdl_setup (_this, width, height);
}

vo_instance_t * vo_sdlsw_setup (vo_instance_t * _this, int width, int height)
{
    setenv ("SDL_VIDEO_YUV_HWACCEL", "0", 1);
    return sdl_setup (_this, width, height);
}

vo_instance_t * vo_sdlaa_setup (vo_instance_t * _this, int width, int height)
{
    setenv ("SDL_VIDEODRIVER", "aalib", 1);
    return sdl_setup (_this, width, height);
}


/**
 * Close SDL, Cleanups, Free Memory.
 *
 *    params : this == sdl vo instance to close
 *   returns : non-zero on error, zero on success
 **/

static void sdl_close (vo_instance_t * _this)
{
    sdl_instance_t * this = (sdl_instance_t *)_this;

    sdl_free_frames (this);

    /* Free our blitting surface */
    if (this->surface)
	SDL_FreeSurface(this->surface);
	
    /* Cleanup SDL */
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}




vo_instance_t sdl_vo_instance = {
    vo_sdl_setup, sdl_close, sdl_get_frame
};

vo_instance_t sdlsw_vo_instance = {
    vo_sdlsw_setup, sdl_close, sdl_get_frame
};

vo_instance_t sdlaa_vo_instance = {
    vo_sdlaa_setup, sdl_close, sdl_get_frame
};

#endif
