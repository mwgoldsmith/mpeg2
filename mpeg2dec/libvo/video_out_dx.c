/*
 * video_out_dx.c
 * Copyright (C) 2000-2002 Michel Lespinasse <walken@zoy.org>
 *
 * Authors: Gildas Bazin <gbazin@netcourrier.com>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#ifdef LIBVO_DX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "video_out.h"
#include "convert.h"

#include <ddraw.h>

#define FOURCC_YV12 0x32315659

#define USE_OVERLAY_TRIPLE_BUFFERING 0

/*****************************************************************************
 * DirectDraw GUIDs.
 * Defining them here allows us to get rid of the dxguid library during
 * the linking stage.
 *****************************************************************************/
#include <initguid.h>
DEFINE_GUID (IID_IDirectDraw2, 0xB3A6F3E0,0x2B43,0x11CF,0xA2,0xDE,0x00,0xAA,0x00,0xB9,0x33,0x56);
DEFINE_GUID (IID_IDirectDrawSurface2, 0x57805885,0x6eec,0x11cf,0x94,0x41,0xa8,0x23,0x03,0xc1,0x0e,0x27);

typedef struct {
    LPDIRECTDRAWSURFACE2 p_surface;
} dx_frame_t;

typedef struct {
    vo_instance_t vo;
    dx_frame_t frame[3];
    int index;

    /* local data */
    int width;
    int height;
    int stride;
    int using_overlay;

    /* Window related variables */
    HWND window;
    RECT window_coords;

    /* Display properties */
    int display_bpp;

    /* DirectX related variables */
    HBRUSH hbrush;
    HINSTANCE hddraw_dll;
    LPDIRECTDRAW2 p_ddobject;
    LPDIRECTDRAWSURFACE2 p_display;
    LPDIRECTDRAWCLIPPER  p_clipper;
    LPDIRECTDRAWSURFACE2 p_overlay;
} dx_instance_t;


static void check_events (dx_instance_t * instance)
{
    MSG msg;

    while (PeekMessage (&msg, instance->window, 0, 0, PM_REMOVE)) {
	TranslateMessage (&msg);
	DispatchMessage (&msg);
    }
}

static long FAR PASCAL dx_event_procedure (HWND hwnd, UINT message,
					   WPARAM wParam, LPARAM lParam)
{
    RECT     rect_window;
    POINT    point_window;
    dx_instance_t * instance;

    switch (message) {

    case WM_WINDOWPOSCHANGED:
	instance = (dx_instance_t *) GetWindowLong (hwnd, GWL_USERDATA);

	/* update the window position */
	point_window.x = 0;
	point_window.y = 0;
	ClientToScreen (hwnd, &point_window);
	instance->window_coords.left = point_window.x;
	instance->window_coords.top = point_window.y;

	/* update the window size */
	GetClientRect (hwnd, &rect_window);
	instance->window_coords.right = rect_window.right + point_window.x;
	instance->window_coords.bottom = rect_window.bottom + point_window.y;

	/* update the overlay */
	if (instance->using_overlay && instance->p_overlay
	    && instance->p_display) {
	    DDOVERLAYFX ddofx;
	    DWORD       dwFlags;

	    memset (&ddofx, 0, sizeof (DDOVERLAYFX));
	    ddofx.dwSize = sizeof (DDOVERLAYFX);
	    dwFlags = DDOVER_SHOW | DDOVER_KEYDESTOVERRIDE;
	    IDirectDrawSurface2_UpdateOverlay (instance->p_overlay,
					       NULL, instance->p_display,
					       &instance->window_coords,
					       dwFlags, &ddofx);
	}
	return 0;

    case WM_CLOSE:	/* forbid the user to close the window */
	return 0;

    case WM_DESTROY:	/* just destroy the window */
	PostQuitMessage (0);
	return 0;
    }

    return DefWindowProc (hwnd, message, wParam, lParam);
}

static int create_window (dx_instance_t * instance)
{
    WNDCLASSEX wc;                                /* window class components */
    RECT       rect_window;
    WNDCLASS   wndclass;
    HDC        hdc;

    /* check if libvo_dx class has already been registered */
    if (!GetClassInfo (GetModuleHandle (NULL), "libvo_dx", &wndclass)) {
	/* it doesn't exist, we need to create it */

	/* create the actual brush */
	instance->hbrush = CreateSolidBrush (RGB(0, 0, 0));

	/* Get the color depth of the display */
	hdc = GetDC (NULL);
	instance->display_bpp = GetDeviceCaps (hdc, BITSPIXEL);
	ReleaseDC (NULL, hdc);

	/* fill in the window class structure */
	wc.cbSize        = sizeof (WNDCLASSEX);
	wc.style         = CS_DBLCLKS;                   /* style: dbl click */
	wc.lpfnWndProc   = (WNDPROC)dx_event_procedure;/* event handler */
	wc.cbClsExtra    = 0;                         /* no extra class data */
	wc.cbWndExtra    = 0;                        /* no extra window data */
	wc.hInstance     = GetModuleHandle (NULL);               /* instance */
	wc.hIcon         = NULL;                                     /* icon */
	wc.hCursor       = LoadCursor (NULL, IDC_ARROW);      /* load cursor */
	wc.hbrBackground = instance->hbrush;             /* background color */
	wc.lpszMenuName  = NULL;                                  /* no menu */
	wc.lpszClassName = "libvo_dx";           /* use a special class */
	wc.hIconSm       = NULL;                                     /* icon */

	/* register the window class */
	if (!RegisterClassEx (&wc)) {
	    fprintf (stderr, "Can not register window class\n");
	    return 1;
	}
    }

    /* when you create a window you give the dimensions you wish it to have.
     * Unfortunatly these dimensions will include the borders and title bar.
     * We use the following function to find out the size of the window
     * corresponding to the useable surface we want */
    rect_window.top    = 10;
    rect_window.left   = 10;
    rect_window.right  = rect_window.left + instance->width;
    rect_window.bottom = rect_window.top + instance->height;
    AdjustWindowRect (&rect_window, WS_OVERLAPPEDWINDOW|WS_SIZEBOX, 0);

    /* create the window */
    instance->window =
	CreateWindow ("libvo_dx",        /* window class */
		      instance->using_overlay ? "mpeg2dec DirectX YUV overlay"
		      : "mpeg2dec DirectX",       /* window title bar text */
		      WS_OVERLAPPEDWINDOW
		      | WS_SIZEBOX,               /* window style */
		      CW_USEDEFAULT,                 /* default X coordinate */
		      0,                             /* default Y coordinate */
		      rect_window.right - rect_window.left, /* window width */
		      rect_window.bottom - rect_window.top, /* window height */
		      NULL,                      /* no parent window */
		      NULL,                      /* no menu in this window */
		      GetModuleHandle(NULL),  /* handle of program instance */
		      NULL);                     /* no additional arguments */

    if (instance->window == NULL) {
	fprintf (stderr, "Can not create window\n");
	return 1;
    }

    /* store a directx_instance pointer into the window local storage
     * (for later use in event_handler).
     * We need to use SetWindowLongPtr when it is available in mingw */
    SetWindowLong (instance->window, GWL_USERDATA, (LONG)instance);

    /* now display the window */
    ShowWindow (instance->window, SW_SHOW);

    return 0;
}

static void dx_close (vo_instance_t * _instance)
{
    dx_instance_t * instance;
    int i;

    instance = (dx_instance_t *) _instance;

    if (instance->using_overlay && instance->p_overlay) {
	IDirectDrawSurface2_Release (instance->p_overlay);
	instance->p_overlay = NULL;
    } else
	for (i = 0; i < 3; i++) {
	    if (instance->frame[i].p_surface != NULL)
		IDirectDrawSurface2_Release (instance->frame[i].p_surface);
	    instance->frame[i].p_surface = NULL;
	}

    if (instance->p_clipper != NULL)
	IDirectDrawClipper_Release (instance->p_clipper);

    if (instance->p_display != NULL)
	IDirectDrawSurface2_Release (instance->p_display);

    if (instance->p_ddobject != NULL)
	IDirectDraw2_Release (instance->p_ddobject);

    if (instance->hddraw_dll != NULL)
	FreeLibrary (instance->hddraw_dll);

    if (instance->window != NULL)
	DestroyWindow (instance->window);
}

static int dx_init (dx_instance_t *instance)
{
    HRESULT dxresult;
    HRESULT (WINAPI *OurDirectDrawCreate)(GUID *,LPDIRECTDRAW *,IUnknown *);
    LPDIRECTDRAW p_ddobject = NULL;
    DDSURFACEDESC ddsd;
    LPDIRECTDRAWSURFACE p_display;

    /* load direct draw DLL */
    instance->hddraw_dll = LoadLibrary ("DDRAW.DLL");
    if (instance->hddraw_dll == NULL)
	goto error;

    OurDirectDrawCreate = 
	(void *) GetProcAddress (instance->hddraw_dll, "DirectDrawCreate");
    if (OurDirectDrawCreate == NULL)
	goto error;

    /* Initialize DirectDraw */
    dxresult = OurDirectDrawCreate (NULL, &p_ddobject, NULL);
    if (dxresult != DD_OK)
	goto error;

    /* Get the IDirectDraw2 interface */
    dxresult = IDirectDraw_QueryInterface (p_ddobject, &IID_IDirectDraw2,
					   (LPVOID *)&instance->p_ddobject);
    /* Release the unused interface */
    IDirectDraw_Release (p_ddobject); p_ddobject = NULL;
    if (dxresult != DD_OK)
	goto error;

    /* Set DirectDraw Cooperative level, ie what control we want over Windows
     * display */
    dxresult = IDirectDraw_SetCooperativeLevel (instance->p_ddobject,
						instance->window,
						DDSCL_NORMAL);
    if (dxresult != DD_OK)
	goto error;

    /* Now get the primary surface. This surface is what you actually see
     * on your screen */
    memset (&ddsd, 0, sizeof (DDSURFACEDESC));
    ddsd.dwSize = sizeof (DDSURFACEDESC);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    dxresult = IDirectDraw2_CreateSurface (instance->p_ddobject, &ddsd,
					   &p_display, NULL);
    if (dxresult != DD_OK)
	goto error;

    dxresult =
	IDirectDrawSurface_QueryInterface (p_display,
					   &IID_IDirectDrawSurface2,
					   (LPVOID *)&instance->p_display);
    IDirectDrawSurface_Release (p_display); p_display = NULL;
    if (dxresult != DD_OK)
	goto error;

    /* Create a clipper that will be used only in non-overlay mode */
    dxresult = IDirectDraw2_CreateClipper (instance->p_ddobject, 0,
					   &instance->p_clipper, NULL);
    if (dxresult != DD_OK)
	goto error;

    /* associate the clipper to the window */
    dxresult = IDirectDrawClipper_SetHWnd (instance->p_clipper, 0,
					   instance->window);
    if (dxresult != DD_OK)
	goto error;

    /* associate the clipper with the surface */
    dxresult = IDirectDrawSurface_SetClipper (instance->p_display,
					      instance->p_clipper);
    if (dxresult != DD_OK)
	goto error;

    return 0;

error:
    fprintf (stderr, "cannot initialize DirectX\n");
    dx_close ((vo_instance_t *)instance);
    return 1;
}

static LPDIRECTDRAWSURFACE2 dx_alloc_frame (dx_instance_t * instance)
{
    HRESULT dxresult;
    LPDIRECTDRAWSURFACE p_surface;
    LPDIRECTDRAWSURFACE2 p_surface2;
    DDSURFACEDESC ddsd;

    memset (&ddsd, 0, sizeof (DDSURFACEDESC));
    ddsd.dwSize = sizeof (DDSURFACEDESC);
    ddsd.ddpfPixelFormat.dwSize = sizeof (DDPIXELFORMAT);
    ddsd.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_CAPS;
    ddsd.dwHeight = instance->height;
    ddsd.dwWidth = instance->width;

    if (instance->using_overlay) {
	ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
	ddsd.ddpfPixelFormat.dwFourCC = FOURCC_YV12;
	ddsd.dwFlags |= DDSD_PIXELFORMAT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY;
#if USE_OVERLAY_TRIPLE_BUFFERING
	ddsd.dwFlags |= DDSD_BACKBUFFERCOUNT;
	ddsd.ddsCaps.dwCaps |= DDSCAPS_COMPLEX | DDSCAPS_FLIP;
#endif

	ddsd.dwBackBufferCount = 2;
    } else
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

    dxresult = IDirectDraw2_CreateSurface (instance->p_ddobject,
					   &ddsd, &p_surface, NULL);
    if (dxresult != DD_OK)
	return NULL;

    /* Try to get a newer DirectX interface */
    dxresult =
	IDirectDrawSurface_QueryInterface (p_surface,
					   &IID_IDirectDrawSurface2,
					   (LPVOID *)&p_surface2);
    IDirectDrawSurface_Release (p_surface); /* Release old interface */
    if (dxresult != DD_OK) {
	fprintf (stderr, "cannot get IDirectDrawSurface2 interface\n");
	return NULL;
    }

    if (instance->using_overlay) {
	/* update the overlay */
	if (instance->using_overlay && p_surface2 && instance->p_display) {
	    DDOVERLAYFX ddofx;
	    DWORD       dwFlags;

	    memset (&ddofx, 0, sizeof (DDOVERLAYFX));
	    ddofx.dwSize = sizeof (DDOVERLAYFX);
	    dwFlags = DDOVER_SHOW | DDOVER_KEYDESTOVERRIDE;
	    IDirectDrawSurface2_UpdateOverlay (p_surface2,
					       NULL, instance->p_display,
					       &instance->window_coords,
					       dwFlags, &ddofx);
	}
    }

    return p_surface2;
}

static void dx_start_fbuf (vo_instance_t * _instance,
			   uint8_t * buf[3], void * id)
{
    dx_instance_t * instance = (dx_instance_t *) _instance;
    LPDIRECTDRAWSURFACE2 p_surface, p_backbuffer;
    DDSURFACEDESC ddsd;
    HRESULT dxresult;

    /* Events loop */
    check_events (instance);

    /* Find the surface */
    if (instance->using_overlay)
	p_surface = instance->p_overlay;
    else
	p_surface = (LPDIRECTDRAWSURFACE2)id;

    /* Get the back buffer */
    memset (&ddsd.ddsCaps, 0, sizeof (DDSCAPS));
    ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
    if (DD_OK != IDirectDrawSurface2_GetAttachedSurface (p_surface,
							 &ddsd.ddsCaps,
							 &p_backbuffer)) {
	/* front buffer is the same as back buffer */
	p_backbuffer = p_surface;
    }

    /* Lock the surface to get a valid pointer to the picture buffer */
    memset (&ddsd, 0, sizeof (DDSURFACEDESC));
    ddsd.dwSize = sizeof (DDSURFACEDESC);
    dxresult = IDirectDrawSurface2_Lock (p_backbuffer, NULL, &ddsd,
					 DDLOCK_NOSYSLOCK | DDLOCK_WAIT, NULL);
    if (dxresult == DDERR_SURFACELOST) {
	/* Your surface can be lost so be sure
	 * to check this and restore it if needed */
	IDirectDrawSurface2_Restore (p_backbuffer);
	dxresult = IDirectDrawSurface2_Lock (p_backbuffer, NULL, &ddsd,
					     DDLOCK_NOSYSLOCK | DDLOCK_WAIT,
					     NULL);
    }
    if (dxresult != DD_OK) {
	fprintf (stderr, "cannot lock surface\n");
	return;
    }

    /* Unlock the Surface */
    IDirectDrawSurface2_Unlock (p_backbuffer, NULL);

    buf[0] = ddsd.lpSurface;
    instance->stride = ddsd.lPitch;
    buf[2] = buf[0] + instance->stride * instance->height;
    buf[1] = buf[2] + ((instance->stride * instance->height) >> 2);
}

static void dx_setup_fbuf (vo_instance_t * _instance,
			   uint8_t * buf[3], void ** id)
{
    dx_instance_t * instance = (dx_instance_t *) _instance;

    instance->frame[instance->index].p_surface = dx_alloc_frame(instance);
    *id = instance->frame[instance->index].p_surface;
    dx_start_fbuf (_instance, buf, *id);
    instance->index++;
}

static void dx_copy_yuv_picture (dx_instance_t * instance,
				 uint8_t * buf[3], void * id)
{
    uint8_t * dest_buf[3];
    int i;

    dx_start_fbuf ((vo_instance_t *)instance, dest_buf, id);

    for (i = 0; i < (instance->height >> 1); i++) {
	memcpy (dest_buf[0], buf[0] + 2 * i * instance->width,
		instance->width);
	dest_buf[0] += instance->stride;
	memcpy (dest_buf[0], buf[0] + (2 * i + 1) * instance->width,
		instance->width);
	dest_buf[0] += instance->stride;
	memcpy (dest_buf[1], buf[1] + i * instance->width /2,
		instance->width / 2);
	dest_buf[1] += (instance->stride /2);
	memcpy (dest_buf[2], buf[2] + i * instance->width /2,
		instance->width / 2);
	dest_buf[2] += (instance->stride /2);
    }
}

static void dx_draw_frame (vo_instance_t * _instance,
			   uint8_t * buf[3], void * id)
{
    dx_instance_t * instance = (dx_instance_t *) _instance;
    HRESULT dxresult;

    dx_copy_yuv_picture (instance, buf, id);

    dxresult =
	IDirectDrawSurface2_Flip (instance->p_overlay, NULL, DDFLIP_WAIT);
    if (dxresult == DDERR_SURFACELOST) {
	/* restore surfaces and try again */
	IDirectDrawSurface2_Restore (instance->p_display);
	IDirectDrawSurface2_Restore (instance->p_overlay);
	IDirectDrawSurface2_Flip (instance->p_overlay, NULL, DDFLIP_WAIT);
    }
}

static void dxrgb_draw_frame (vo_instance_t * _instance,
			      uint8_t * buf[3], void * id)
{
    dx_instance_t * instance = (dx_instance_t *) _instance;
    LPDIRECTDRAWSURFACE2 p_surface;
    HRESULT dxresult;
    DDBLTFX  ddbltfx;

    p_surface = (LPDIRECTDRAWSURFACE2) id;

    /* We ask for the "NOTEARING" option */
    memset (&ddbltfx, 0, sizeof (DDBLTFX));
    ddbltfx.dwSize = sizeof(DDBLTFX);
    ddbltfx.dwDDFX = DDBLTFX_NOTEARING;

    /* Blit video surface to display */
    dxresult = IDirectDrawSurface2_Blt (instance->p_display,
					&instance->window_coords,
					p_surface, NULL, DDBLT_WAIT, &ddbltfx);
    if (dxresult == DDERR_SURFACELOST) {
	/* restore surface and try again */
	IDirectDrawSurface2_Restore (instance->p_display);
	IDirectDrawSurface2_Blt (instance->p_display, &instance->window_coords,
				 p_surface, NULL, DDBLT_WAIT, &ddbltfx);
    }
}

static int dx_setup (vo_instance_t * _instance, int width, int height,
		     vo_setup_result_t * result)
{
    dx_instance_t * instance = (dx_instance_t *) _instance;

    instance->width = width;
    instance->height = height;
    instance->index = 0;

    if (create_window (instance))
	goto error;

    if (dx_init (instance))
	goto error;

    if (instance->using_overlay) {
	instance->p_overlay = dx_alloc_frame (instance);
	if (!instance->p_overlay)
	    goto error;

	result->convert = NULL;
    } else
	result->convert = convert_rgb (CONVERT_RGB, instance->display_bpp);

    return 0;

error:
    dx_close (_instance);
    return 1;

}

vo_instance_t * dx_open (void)
{
    dx_instance_t * instance;

    instance = malloc (sizeof (dx_instance_t));
    if (instance == NULL)
	return NULL;

    memset (instance, 0, sizeof (dx_instance_t));
    instance->vo.setup = dx_setup;
    instance->vo.setup_fbuf = dx_setup_fbuf;
    instance->vo.set_fbuf = NULL;
    instance->vo.start_fbuf = dx_start_fbuf;
    instance->vo.draw = dxrgb_draw_frame;
    instance->vo.discard = NULL;
    instance->vo.close = dx_close;

    return (vo_instance_t *) instance;
}

vo_instance_t * vo_dx_open (void)
{
    vo_instance_t * instance;

    instance = dx_open ();
    if (instance) {
	((dx_instance_t *)instance)->vo.setup_fbuf = NULL;
	((dx_instance_t *)instance)->vo.start_fbuf = NULL;
	((dx_instance_t *)instance)->vo.draw = dx_draw_frame; 
	((dx_instance_t *)instance)->using_overlay = 1;
    }
    return instance;
}

vo_instance_t * vo_dxrgb_open (void)
{
    return dx_open ();
}
#endif
