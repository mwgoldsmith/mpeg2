/*
 *  video_out_sdl.c
 *
 *  Copyright (C) Ryan C. Gordon <icculus@lokigames.com> - April 22, 2000.
 *
 *  A mpeg2dec display driver that does output through the
 *  Simple DirectMedia Layer (SDL) library. This effectively gives us all
 *  sorts of output options: X11, SVGAlib, fbcon, AAlib, GGI. Win32, MacOS
 *  and BeOS support, too. Yay. SDL info, source, and binaries can be found
 *  at http://slouken.devolution.com/SDL/
 *
 *  This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 *	
 *  mpeg2dec is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  mpeg2dec is distributed in the hope that it will be useful,
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
 */

#include "config.h"

#ifdef LIBVO_SDL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "video_out.h"
#include "video_out_internal.h"
#include "log.h"

#include <SDL/SDL.h>

/** Private SDL Data structure **/

static struct sdl_priv_s {

	/* SDL YUV surface & overlay */
	SDL_Surface *surface;
	SDL_Overlay *overlay;

	/* available fullscreen modes */
	SDL_Rect **fullmodes;

	/* surface attributes for fullscreen and windowed mode */
	Uint32 sdlflags, sdlfullflags;

	/* save the windowed output extents */
	SDL_Rect windowsize;
	
	/* Bits per Pixel */
	Uint8 bpp;

	/* current fullscreen mode, 0 = highest available fullscreen mode */
	int fullmode;

	/* YUV ints */
	int framePlaneY, framePlaneUV;
	int slicePlaneY, slicePlaneUV;
} sdl_priv;


/** OMS Plugin functions **/


/**
 * Take a null-terminated array of pointers, and find the last element.
 *
 *    params : array == array of which we want to find the last element.
 *   returns : index of last NON-NULL element.
 **/

static inline int findArrayEnd (SDL_Rect **array)
{
	int i = 0;
	while ( array[i++] );	/* keep loopin' ... */
	
	/* return the index of the last array element */
	return i - 1;
}


/**
 * Open and prepare SDL output.
 *
 *    params : *plugin ==
 *             *name == 
 *   returns : 0 on success, -1 on failure
 **/

static int sdl_open (void *plugin, void *name)
{
	struct sdl_priv_s *priv = &sdl_priv;
	const SDL_VideoInfo *vidInfo = NULL;
	static int opened = 0;

	if (opened)
	    return 0;
	opened = 1;

	LOG (LOG_DEBUG, "SDL video out: Opened Plugin");
	
	/* default to no fullscreen mode, we'll set this as soon we have the avail. mdoes */
	priv->fullmode = -2;
	/* other default values */
	priv->sdlflags = SDL_HWSURFACE|SDL_RESIZABLE|SDL_ASYNCBLIT;
	priv->sdlfullflags = SDL_HWSURFACE|SDL_FULLSCREEN|SDL_DOUBLEBUF|SDL_ASYNCBLIT;
	priv->surface = NULL;
	priv->overlay = NULL;
	priv->fullmodes = NULL;

	/* initialize the SDL Video system */
	if (SDL_Init (SDL_INIT_VIDEO)) {
		LOG (LOG_ERROR, "SDL video out: Initializing of SDL failed (SDL_Init). Please use the latest version of SDL.");
		return -1;
	}
	
	/* No Keyrepeats! */
	SDL_EnableKeyRepeat(0,0);

	/* get information about the graphics adapter */
	vidInfo = SDL_GetVideoInfo ();
	
	/* collect all fullscreen & hardware modes available */
	if (!(priv->fullmodes = SDL_ListModes (vidInfo->vfmt, priv->sdlfullflags))) {

		/* non hardware accelerated fullscreen modes */
		priv->sdlfullflags &= ~SDL_HWSURFACE;
 		priv->fullmodes = SDL_ListModes (vidInfo->vfmt, priv->sdlfullflags);
	}
	
	/* test for normal resizeable & windowed hardware accellerated surfaces */
	if (!SDL_ListModes (vidInfo->vfmt, priv->sdlflags)) {
		
		/* test for NON hardware accelerated resizeable surfaces - poor you. 
		 * That's all we have. If this fails there's nothing left.
		 * Theoretically there could be Fullscreenmodes left - we ignore this for now.
		 */
		priv->sdlflags &= ~SDL_HWSURFACE;
		if ((!SDL_ListModes (vidInfo->vfmt, priv->sdlflags)) && (!priv->fullmodes)) {
			LOG (LOG_ERROR, "SDL video out: Couldn't get any acceptable SDL Mode for output. (SDL_ListModes failed)");
			return -1;
		}
	}
	
		
   /* YUV overlays need at least 16-bit color depth, but the
    * display might less. The SDL AAlib target says it can only do
    * 8-bits, for example. So, if the display is less than 16-bits,
    * we'll force the BPP to 16, and pray that SDL can emulate for us.
	 */
	priv->bpp = vidInfo->vfmt->BitsPerPixel;
	if (priv->bpp < 16) {
		LOG (LOG_WARNING, "SDL video out: Your SDL display target wants to be at a color depth of (%d), but we need it to be at\
least 16 bits, so we need to emulate 16-bit color. This is going to slow things down; you might want to\
increase your display's color depth, if possible", priv->bpp);
		priv->bpp = 16;  
	}
	
	/* We dont want those in out event queue */
	SDL_EventState(SDL_ACTIVEEVENT, SDL_IGNORE);
	SDL_EventState(SDL_KEYUP, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);
	SDL_EventState(SDL_QUIT, SDL_IGNORE);
	SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
	SDL_EventState(SDL_USEREVENT, SDL_IGNORE);
	
	/* Success! */
	return 0;
}


/**
 * Close SDL, Cleanups, Free Memory
 *
 *    params : *plugin
 *   returns : non-zero on success, zero on error.
 **/

static int sdl_close (void *plugin)
{
	struct sdl_priv_s *priv = &sdl_priv;
	
	LOG (LOG_DEBUG, "SDL video out: Closed Plugin");
	LOG (LOG_INFO, "SDL video out: Closed Plugin");

	/* Cleanup YUV Overlay structure */
	if (priv->overlay) 
		SDL_FreeYUVOverlay(priv->overlay);

	/* Free our blitting surface */
	if (priv->surface)
		SDL_FreeSurface(priv->surface);
	
	/* TODO: cleanup the full_modes array */
	
	/* Cleanup SDL */
	SDL_Quit();

	return 0;
}


/**
 * Initialize an SDL surface and an SDL YUV overlay.
 *
 *    params : width  == width of video we'll be displaying.
 *             height == height of video we'll be displaying.
 *             fullscreen == want to be fullscreen?
 *             title == Title for window titlebar.
 *   returns : non-zero on success, zero on error.
 **/

static int sdl_setup (vo_output_video_attr_t *attr)
{
	struct sdl_priv_s *priv = &sdl_priv;

	sdl_open (NULL, NULL);

	/* Set window title of our output window */
	if (attr->title)
		SDL_WM_SetCaption (attr->title, "oms");
	else
		SDL_WM_SetCaption ("SDL: RETURN/ESCAPE = Toggle Fullscreen/Window; PLUS = Cycle Fullscreen Resolutions", "oms");

	/* Set the video mode via SDL & check for Fullscreen first */
	if ((attr->fullscreen) && priv->fullmodes) {
		if (!(priv->surface = SDL_SetVideoMode (priv->fullmodes[0]->w, priv->fullmodes[0]->h, priv->bpp, priv->sdlfullflags))) {
			LOG (LOG_ERROR, "SDL video out: Could not set the desired fullscreen mode (SDL_SetVideoMode). Will try windowed mode next.");

			/* disable fullscreen mode and free memory */
			attr->fullscreen = 0;
		}
	}

	/* Initialize the windowed SDL Window per default */
	if (!attr->fullscreen)
		if (!(priv->surface = SDL_SetVideoMode (attr->width, attr->height, priv->bpp, priv->sdlflags))) {
			LOG (LOG_ERROR, "SDL video out: Could not set the desired video mode (SDL_SetVideoMode)");
			return -1;
		}

	/* Initialize and create the YUV Overlay used for video out */
	if (!(priv->overlay = SDL_CreateYUVOverlay (attr->width, attr->height, SDL_IYUV_OVERLAY, priv->surface))) {
		LOG (LOG_ERROR, "SDL video out: Couldn't create an SDL-based YUV overlay");
		return -1;
	}
	priv->framePlaneY = (attr->width * attr->height);
	priv->framePlaneUV = ((attr->width / 2) * (attr->height / 2));
	priv->slicePlaneY = ((attr->width) * 16);
	priv->slicePlaneUV = ((attr->width / 2) * (8));
	
	/* Save the original Image size */
	priv->windowsize.w = attr->width;
	priv->windowsize.h = attr->height;
	
	return 0;
}


/**
 * Draw a frame to the SDL YUV overlay.
 *
 *   params : *src[] == the Y, U, and V planes that make up the frame.
 *  returns : non-zero on success, zero on error.
 **/

static int sdl_draw_frame (frame_t *frame)
{
	struct sdl_priv_s *priv = &sdl_priv;
	uint8_t *dst;

	if (SDL_LockYUVOverlay (priv->overlay)) {
		LOG (LOG_ERROR, "SDL video out: Couldn't lock SDL-based YUV overlay");
		return -1;
	}

	dst = (uint8_t *) *(priv->overlay->pixels);
	memcpy (dst, frame->base[0], priv->framePlaneY);
	dst += priv->framePlaneY;
	memcpy (dst, frame->base[1], priv->framePlaneUV);
	dst += priv->framePlaneUV;
	memcpy (dst, frame->base[2], priv->framePlaneUV);

	SDL_UnlockYUVOverlay (priv->overlay);

	return 0;
}


/**
 * Draw a slice (16 rows of image) to the SDL YUV overlay.
 *
 *   params : *src[] == the Y, U, and V planes that make up the slice.
 *  returns : non-zero on error, zero on success.
 **/

static int sdl_draw_slice (uint8_t *src[], int slice_num)
{
	struct sdl_priv_s *priv = &sdl_priv;
	uint8_t *dst;

	if (SDL_LockYUVOverlay (priv->overlay)) {
		LOG (LOG_ERROR, "SDL video out: Couldn't lock SDL-based YUV overlay");
		return -1;
	}

	dst = (uint8_t *) *(priv->overlay->pixels) + (priv->slicePlaneY * slice_num);
	memcpy (dst, src[0], priv->slicePlaneY);
	dst = (uint8_t *) *(priv->overlay->pixels) + priv->framePlaneY + (priv->slicePlaneUV * slice_num);
	memcpy (dst, src[1], priv->slicePlaneUV);
	dst += priv->framePlaneUV;
	memcpy (dst, src[2], priv->slicePlaneUV);
	
	SDL_UnlockYUVOverlay (priv->overlay);

	return 0;
}


/**
 * Sets the specified fullscreen mode.
 *
 *   params : mode == index of the desired fullscreen mode
 *  returns : doesn't return
 **/
 
static void set_fullmode (int mode)
{
	struct sdl_priv_s *priv = &sdl_priv;
	SDL_Surface *newsurface = NULL;
	
	
	/* if we haven't set a fullmode yet, default to the lowest res fullmode first */
	if (mode < 0) 
		mode = priv->fullmode = findArrayEnd(priv->fullmodes) - 1;

	/* change to given fullscreen mode and hide the mouse cursor*/
	newsurface = SDL_SetVideoMode(priv->fullmodes[mode]->w, priv->fullmodes[mode]->h, priv->bpp, priv->sdlfullflags);
	
	/* if we were successfull hide the mouse cursor and save the mode */
	if (newsurface) {
		priv->surface = newsurface;
		SDL_ShowCursor(0);
	}
}


/**
 * Checks for SDL keypress and window resize events
 *
 *   params : none
 *  returns : doesn't return
 **/
 
static void check_events (void)
{
	struct sdl_priv_s *priv = &sdl_priv;
	SDL_Event event;
	SDLKey keypressed;
	
	/* Poll the waiting SDL Events */
	while ( SDL_PollEvent(&event) ) {
		switch (event.type) {

			/* capture window resize events */
			case SDL_VIDEORESIZE:
				priv->surface = SDL_SetVideoMode(event.resize.w, event.resize.h, priv->bpp, priv->sdlflags);

				/* save video extents, to restore them after going fullscreen */
				priv->windowsize.w = priv->surface->w;
				priv->windowsize.h = priv->surface->h;
				
				LOG (LOG_DEBUG, "SDL video out: Window resize");
			break;
			
			
			/* graphics more selection shortcuts */
			case SDL_KEYDOWN:
				keypressed = event.key.keysym.sym;
				
				/* plus key pressed. plus cycles through available fullscreenmodes, if we have some */
				if ( ((keypressed == SDLK_PLUS) || (keypressed == SDLK_KP_PLUS)) && (priv->fullmodes) ) {
					/* select next fullscreen mode */
					priv->fullmode++;
					if (priv->fullmode > (findArrayEnd(priv->fullmodes) - 1)) priv->fullmode = 0;
					set_fullmode(priv->fullmode);
	
					LOG (LOG_DEBUG, "SDL video out: Set next available fullscreen mode.");
				}

				/* return or escape key pressed toggles/exits fullscreenmode */
				else if ( (keypressed == SDLK_RETURN) || (keypressed == SDLK_ESCAPE) ) {
				 	if (priv->surface->flags & SDL_FULLSCREEN) {
						priv->surface = SDL_SetVideoMode(priv->windowsize.w, priv->windowsize.h, priv->bpp, priv->sdlflags);
						SDL_ShowCursor(1);
						
						LOG (LOG_DEBUG, "SDL video out: Windowed mode");
					} 
					else if (priv->fullmodes){
						set_fullmode(priv->fullmode);

						LOG (LOG_DEBUG, "SDL video out: Set fullscreen mode.");
					}
				}
				break;
		}
	}
}


/**
 * Display the surface we have written our data to and check for events.
 *
 *   params : mode == index of the desired fullscreen mode
 *  returns : doesn't return
 **/

static void sdl_flip_page (void)
{
	struct sdl_priv_s *priv = &sdl_priv;
	
	/* check and react on keypresses and window resizes */
	check_events();

	/* blit to the YUV overlay */
	SDL_DisplayYUVOverlay (priv->overlay, &priv->surface->clip_rect);

	/* check if we have a double buffered surface and flip() if we do. */
   if ( priv->surface->flags & SDL_DOUBLEBUF ) {
        SDL_Flip(priv->surface);
   }

}

static int sdl_overlay (overlay_buf_t *overlay_buf, int id) 
{
  return 0;
}


static frame_t* sdl_allocate_image_buffer(int width, int height, uint32_t format)
{
    return libvo_common_alloc (width, height);
}

static void sdl_free_image_buffer(frame_t* image)
{
    libvo_common_free (image);
}


LIBVO_EXTERN (sdl, "sdl")

#endif
