/* 
 * video_out_x11.c, X11 interface
 *
 *
 * Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. 
 *
 * Hacked into mpeg2dec by
 * 
 * Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * 15 & 16 bpp support added by Franck Sicard <Franck.Sicard@solsoft.fr>
 *
 * Xv image suuport by Gerd Knorr <kraxel@goldbach.in-berlin.de>
 */

#include "config.h"

#ifdef LIBVO_X11

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "log.h"
#include "video_out.h"
#include "video_out_internal.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <errno.h>
#include "yuv2rgb.h"
#ifdef DENT_RESCALE
#include "rescale.h"
#endif

#ifdef LIBVO_XSHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif

#ifdef LIBVO_XV
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>
#endif

// not defined on solaris 2.6... grumbl
Bool XShmQueryExtension (Display *);

static int x11_close		(void *plugin);
static int x11_setup		(vo_output_video_attr_t *attr);
#ifdef LIBVO_XSHM
static int _xshm_create		(XShmSegmentInfo *Shminfo, int size);
static void _xshm_destroy	(XShmSegmentInfo *Shminfo);
#endif
#ifdef LIBVO_XV
static XvImage *_xv_create	(void);
static int _xv_draw_frame	(frame_t *frame);
static int _xv_draw_slice	(uint8_t *src[], int slice_num);
static void _xv_flip_page	(void);
#endif
static int x11_draw_frame	(frame_t *frame);
static int x11_draw_slice	(uint8_t *src[], int slice_num);
static void x11_flip_page	(void);
static int x11_overlay		(overlay_buf_t *overlay_buf, int id);
static void x11_free_image_buffer	(frame_t* image);
static frame_t * x11_allocate_image_buffer (int width, int height, uint32_t format);

#ifdef LIBVO_XV
#define NUM_BUFFERS 6
#else
#define NUM_BUFFERS 1
#endif

static struct x11_priv_s {
/* local data */
	unsigned char *ImageData;
	int image_width;
	int image_height;
        int oldtime;

/* X11 related variables */
	Display *display;
	Window window;
	GC gc;
	XVisualInfo vinfo;
	XImage *ximage;
	int depth, bpp, mode;
	XWindowAttributes attribs;
	int X_already_started;

#ifdef LIBVO_XSHM
	int Shmem_Flag;
	XShmSegmentInfo Shminfo[NUM_BUFFERS];
	int gXErrorFlag;
	int CompletionType;
#endif

#ifdef LIBVO_XV
	unsigned int xv_port;
	unsigned int xv_format;

	XvImage *current_image;
	XvImage *xvimage;
#endif

	int win_width, win_height;

// FIXME: just for now - overlay
	overlay_buf_t overlay_buf;
} x11_priv;


LIBVO_EXTERN (x11,"x11")

#ifdef LIBVO_XSHM
/* since it doesn't seem to be defined on some platforms */
int XShmGetEventBase(Display*);
#endif

static void x11_InstallXErrorHandler ()
{
	//XSetErrorHandler (HandleXError);
	XFlush (x11_priv.display);
}

static void x11_DeInstallXErrorHandler ()
{
	XSetErrorHandler (NULL);
	XFlush (x11_priv.display);
}

static int x11_open (void)
{
	char *display = ":0.0";
	int screen;
	unsigned int fg, bg;
	XSizeHints hint;
	XEvent xev;
	XGCValues xgcv;
	Colormap theCmap;
	XSetWindowAttributes xswa;
	unsigned long xswamask;
#ifdef LIBVO_XV
	unsigned int ver, rel, req, ev, err;
#endif
	struct x11_priv_s *priv = &x11_priv;
	static int opened = 0;

	if (opened)
	    return 0;
	opened = 1;

	if (priv->X_already_started)
		return -1;

	if (getenv ("DISPLAY"))
		display = getenv ("DISPLAY");

	if (!(priv->display = XOpenDisplay (display))) {
		LOG (LOG_ERROR, "Can not open display");
		return -1;
	}

	XGetWindowAttributes (priv->display, DefaultRootWindow(priv->display), &priv->attribs);

	priv->depth = priv->attribs.depth;

	screen = DefaultScreen (priv->display);

	hint.x = 0;
	hint.y = 0;
	hint.width = 320;
	hint.height = 200;
	hint.flags = PPosition | PSize;

	/* Get some colors */

	bg = WhitePixel (priv->display, screen);
	fg = BlackPixel (priv->display, screen);

	/*
	 *
	 * depth in X11 terminology land is the number of bits used to
	 * actually represent the colour.
	 *
	 * bpp in X11 land means how many bits in the frame buffer per
	 * pixel. 
	 *
	 * ex. 15 bit color is 15 bit depth and 16 bpp. Also 24 bit
	 *     color is 24 bit depth, but can be 24 bpp or 32 bpp.
	 */

	switch (priv->depth) {
	case 15:
	case 16:
	case 24:
	case 32:
		break;

	default:
		/* The root window may be 8bit but there might still be
		 * visuals with other bit depths. For example this is the
		 * case on Sun/Solaris machines.
		 */
		priv->depth = 24;
		break;
	}

	XMatchVisualInfo (priv->display, screen, priv->depth, TrueColor, &priv->vinfo);

	theCmap   = XCreateColormap(priv->display, RootWindow(priv->display,screen), priv->vinfo.visual, AllocNone);

	xswa.background_pixel = 0;
	xswa.border_pixel     = 1;
	xswa.colormap         = theCmap;
	xswamask = CWBackPixel | CWBorderPixel | CWColormap;

	priv->window = XCreateWindow (priv->display,
		RootWindow (priv->display,screen),
		hint.x, hint.y, hint.width, hint.height,
		4, priv->depth, CopyFromParent, priv->vinfo.visual,
		xswamask, &xswa);

	XSelectInput (priv->display, priv->window, StructureNotifyMask);

// Tell other applications about this window
	XSetStandardProperties(priv->display, priv->window,
		"Open Media System", "OMS", None, NULL, 0, &hint);

// Map window
	XMapWindow (priv->display, priv->window);

// Wait for map
	do {
		XNextEvent (priv->display, &xev);
	} while (xev.type != MapNotify || xev.xmap.event != priv->window);

	XSelectInput (priv->display, priv->window, NoEventMask);

	XFlush (priv->display);
	XSync (priv->display, False);

	priv->gc = XCreateGC (priv->display, priv->window, 0L, &xgcv);

#ifdef LIBVO_XSHM
	if (XShmQueryExtension (priv->display)) {
		priv->Shmem_Flag = 1;
		priv->CompletionType = XShmGetEventBase(priv->display) + ShmCompletion;
		LOG (LOG_INFO, "Using MIT Shared memory extension");
	} else {
		priv->Shmem_Flag = 0;
	}
#endif

#ifdef LIBVO_XV
	priv->xv_port = 0;

	if (Success == XvQueryExtension (priv->display, &ver, &rel, &req, &ev, &err)) {
		unsigned int formats, adaptors;
		XvAdaptorInfo *ai;
		XvImageFormatValues  *fo;
		int i;

		// check for Xvideo support
		if (Success != XvQueryAdaptors (priv->display, DefaultRootWindow(priv->display), &adaptors, &ai)) {
			LOG (LOG_ERROR, "Xv: XvQueryAdaptors failed");
			return -1;
		}

		// check adaptors
		for (i=0; i<adaptors; i++) {
			if ((ai[i].type & XvInputMask) && (ai[i].type & XvImageMask) && !priv->xv_port) 
				priv->xv_port = ai[i].base_id;
		}

		// check image formats
		if (priv->xv_port) {
			fo = XvListImageFormats (priv->display, priv->xv_port, (int*)&formats);

			for (i=0; i<formats; i++) {
				if (fo[i].id == 0x32315659) {
					priv->xv_format = fo[i].id;
					LOG (LOG_INFO, "using Xvideo port %d for hw scaling", priv->xv_port);
					break;
				}
			}
			if (i == formats) /* no matching image format not */
				priv->xv_port = 0;
		}
	}

	if (priv->xv_port) {
		video_out_x11.draw_frame = _xv_draw_frame;
		video_out_x11.draw_slice = _xv_draw_slice;
		video_out_x11.flip_page = _xv_flip_page;
	} else
#endif
	{
		video_out_x11.draw_frame = x11_draw_frame;
		video_out_x11.draw_slice = x11_draw_slice;
		video_out_x11.flip_page = x11_flip_page;
		video_out_x11.overlay = x11_overlay;
	}

#ifdef DENT_RESCALE
	//rescale_init ((priv->depth == 24) ? priv->bpp : priv->depth, priv->mode);
#endif
	priv->X_already_started++;

	// catch window resizes
	XSelectInput (priv->display, priv->window, StructureNotifyMask);

	LOG (LOG_DEBUG, "Open Called\n");
	return 0;
}


/**
 * connect to server, create and map window,
 * allocate colors and (shared) memory
 **/

static int x11_setup (vo_output_video_attr_t *attr)
{
	XSizeHints hint;
	struct x11_priv_s *priv = &x11_priv;

	x11_open ();

	priv->image_width = attr->width;
	priv->image_height = attr->height;

	hint.x = 0;
	hint.y = 0;
	hint.width = priv->image_width;
	hint.height = priv->image_height;
	hint.flags = PPosition | PSize;

	XResizeWindow (priv->display, priv->window, priv->image_width, priv->image_height);

	if (attr->title)
		XSetStandardProperties (priv->display, priv->window,
			attr->title, attr->title, None, NULL, 0, &hint);
	
#ifdef LIBVO_XV
	if (priv->xv_port) {
		priv->xvimage = _xv_create();

		if (priv->xvimage != NULL) {
			// catch window resizes
			XSelectInput (priv->display, priv->window, StructureNotifyMask);
			priv->win_width  = priv->image_width;
			priv->win_height = priv->image_height;

			priv->X_already_started++;
			return 0;
		}
	}
#endif

#ifdef LIBVO_XSHM
	x11_InstallXErrorHandler ();

	if (priv->Shmem_Flag) {
		
		priv->ximage = XShmCreateImage (priv->display, priv->vinfo.visual, priv->depth, ZPixmap, NULL, &priv->Shminfo[0], attr->width, priv->image_height);

		if ((_xshm_create (&priv->Shminfo[0], priv->ximage->bytes_per_line * priv->ximage->height))) {
			XDestroyImage(priv->ximage);
			goto shmemerror;
		}
	
		// If no go, then revert to normal Xlib calls.
		if (!priv->ximage) {
			LOG (LOG_ERROR, "Shared memory error, disabling (Ximage error)");
			goto shmemerror;
		}

		if ((_xshm_create (&priv->Shminfo[0], priv->ximage->bytes_per_line * priv->ximage->height))) {
			XDestroyImage(priv->ximage);
			goto shmemerror;
		}
	
		priv->ximage->data = priv->Shminfo[0].shmaddr;
		priv->ImageData = (unsigned char *) priv->ximage->data;
	} else {
shmemerror:
		priv->Shmem_Flag = 0;
#endif
		priv->ximage = XGetImage (priv->display, priv->window, 0, 0,
			attr->width, priv->image_height, AllPlanes, ZPixmap);
		priv->ImageData = priv->ximage->data;
#ifdef LIBVO_XSHM
	}

	x11_DeInstallXErrorHandler();
#endif

	priv->bpp = priv->ximage->bits_per_pixel;

	// If we have blue in the lowest bit then obviously RGB 
	priv->mode = ((priv->ximage->blue_mask & 0x01)) ? MODE_RGB : MODE_BGR;

#ifdef WORDS_BIGENDIAN 
	if (priv->ximage->byte_order != MSBFirst)
#else
	if (priv->ximage->byte_order != LSBFirst) 
#endif
	{
		LOG (LOG_ERROR, "No support fon non-native XImage byte order");
		return -1;
	}

#ifdef DENT_RESCALE
	rescale_init ((priv->depth == 24) ? priv->bpp : priv->depth, priv->mode);
	rescale_set_factors (priv->image_width, priv->image_height, priv->win_width, priv->win_height);
#endif

	yuv2rgb_init ((priv->depth == 24) ? priv->bpp : priv->depth, priv->mode);
	return 0;
}


static int x11_close(void *plugin) 
{
#ifdef LIBVO_XSHM
	int foo = 0;
	struct x11_priv_s *priv = &x11_priv;

	LOG (LOG_INFO, "Closing video plugin");
	
	if (x11_priv.Shmem_Flag) {
		while (priv->Shminfo[foo].shmaddr) {
			_xshm_destroy(&priv->Shminfo[foo]);
			LOG (LOG_INFO, "destroying shm segment %d", foo);
			foo++;
		}
				
		if (x11_priv.ximage)
			XDestroyImage (x11_priv.ximage);
	}
#endif
	if (x11_priv.window)
		XDestroyWindow (x11_priv.display, x11_priv.window);
	XCloseDisplay (x11_priv.display);
	x11_priv.X_already_started = 0;

	return 0;
}

#ifdef LIBVO_XSHM
static int _xshm_create (XShmSegmentInfo *Shminfo, int size)
{
	struct x11_priv_s *priv = &x11_priv;
	
	Shminfo->shmid = shmget (IPC_PRIVATE, size, IPC_CREAT | 0777);

	if (Shminfo->shmid < 0) {
		LOG (LOG_ERROR, "Shared memory error, disabling (seg id error: %s)", strerror (errno));
		return -1;
	}
	
	Shminfo->shmaddr = (char *) shmat(Shminfo->shmid, 0, 0);

	if (Shminfo->shmaddr == ((char *) -1)) {
		if (Shminfo->shmaddr != ((char *) -1))
			shmdt(Shminfo->shmaddr);
		LOG (LOG_ERROR, "Shared memory error, disabling (address error)");
		return -1;
	}
		
	Shminfo->readOnly = False;
	XShmAttach(priv->display, Shminfo);

	return 0;
}

static void _xshm_destroy (XShmSegmentInfo *Shminfo)
{
	struct x11_priv_s *priv = &x11_priv;
	
	XShmDetach (priv->display, Shminfo);
	shmdt (Shminfo->shmaddr);
	shmctl (Shminfo->shmid, IPC_RMID, 0);
}
#endif

#ifdef LIBVO_XV
static XvImage* _xv_create ()
{
	int foo = 0;
	XvImage *xvimage;
	XShmSegmentInfo *Shminfo;
	struct x11_priv_s *priv = &x11_priv;

	if (!(xvimage = malloc (sizeof (XvImage))))
		return NULL;

#ifdef LIBVO_XSHM	
	if (priv->Shmem_Flag) {
		//finds unused Shmarea
		do
		{
			Shminfo = &priv->Shminfo[foo];
			foo++;
		}
		while (Shminfo->shmaddr != NULL);
				
		xvimage = XvShmCreateImage (priv->display, priv->xv_port, 
				priv->xv_format, 0, priv->image_width, priv->image_height, 
				Shminfo);

		_xshm_create (Shminfo, xvimage->data_size);
		xvimage->data = Shminfo->shmaddr;

		// so we can do grayscale while testing...
		memset (xvimage->data, 128, xvimage->data_size);
	
		return xvimage;
	}
#endif
	xvimage = XvCreateImage (priv->display, priv->xv_port, 
		priv->xv_format, 0, priv->image_width, priv->image_height);

	xvimage->data = malloc(xvimage->data_size);

	// so we can do grayscale while testing...
	memset (xvimage->data, 128, xvimage->data_size);
	
	return xvimage;
}
#endif


static inline int _check_event (void)
{
	struct x11_priv_s *priv = &x11_priv;
	Window root;
	XEvent event;
	int x, y;
	unsigned int w, h, b, d;

	if (XCheckWindowEvent(priv->display, priv->window, StructureNotifyMask, &event)) {
		XGetGeometry(priv->display, priv->window, &root, &x, &y, &w, &h, &b, &d);
		priv->win_width  = w;
		priv->win_height = h;
		LOG (LOG_DEBUG, "win resize: %dx%d", priv->win_width, priv->win_height);                

		return 1;
	}

	/*Fullscreen stuff
         *
         *FIXME this should go up top in place of NoEventMask, it didnt work
	 *there though
         *It was breaking the StructureNotifyMask
         *Other then that the rest was working
         */
        //XSelectInput (priv->display, priv->window, ButtonPressMask);
	
	
        /* 
	   if(XCheckMaskEvent(priv->display, ButtonPressMask, &event)) {
	   LOG(LOG_DEBUG,"BUTTON PRESS AT %ld", event.xbutton.time);
	   if(event.xbutton.time-oldtime<250)//1/4 second
	   //exit(1); //just to test
           //_x11.priv.fullscreen=1; //something like this
           LOG(LOG_DEBUG,"double BUTTON PRESS AT %ld", event.xbutton.time);
	   
           oldtime=event.xbutton.time;
           }*/

	return 0;
}


#ifdef LIBVO_XV
static void _xv_flip_page (void)
{
	struct x11_priv_s *priv = &x11_priv;

	_check_event ();

#ifdef LIBVO_XSHM
	if (priv->Shmem_Flag)
		XvShmPutImage (priv->display, priv->xv_port, priv->window,
			priv->gc, priv->current_image,
			0, 0,  priv->image_width, priv->image_height,
			0, 0,  priv->win_width, priv->win_height,
			False);
	else
#endif
		XvPutImage (priv->display, priv->xv_port, priv->window,
			priv->gc, priv->current_image,
			0, 0,  priv->image_width, priv->image_height,
			0, 0,  priv->win_width, priv->win_height);
	
	XFlush (priv->display);
}
#endif


static void x11_flip_page (void)
{
	struct x11_priv_s *priv = &x11_priv;

	if (_check_event ()) {
#ifdef DENT_RESCALE
		rescale_set_factors (priv->win_width, priv->win_height, priv->image_width, priv->image_height);
#endif
	}

#ifdef DENT_RESCALE
	overlay_rgb (priv->ImageData, &priv->overlay_buf, priv->image_width, priv->image_height);
	rescale (priv->ImageData);
#endif

#ifdef LIBVO_XSHM
	if (priv->Shmem_Flag) {
		XShmPutImage (priv->display, priv->window, priv->gc, priv->ximage, 
			0, 0, 0, 0, priv->ximage->width, priv->ximage->height, True); 
		XFlush (priv->display);
	} else
#endif
	{
		XPutImage(priv->display, priv->window, priv->gc, priv->ximage,
			0, 0, 0, 0, priv->ximage->width, priv->ximage->height);
		XFlush (priv->display);
	}
}


static int x11_overlay	(overlay_buf_t *overlay_buf, int id)
{
	struct x11_priv_s *priv = &x11_priv;

	if (priv->overlay_buf.data)
		free (priv->overlay_buf.data);

fprintf (stderr, "width: %d height: %d", overlay_buf->width, overlay_buf->height);

	memcpy (&priv->overlay_buf, overlay_buf, sizeof (overlay_buf_t));

	return 0;
}


#ifdef LIBVO_XV
static int _xv_draw_slice (uint8_t *src[], int slice_num)
{
	struct x11_priv_s *priv = &x11_priv;
	uint8_t *dst;

	priv->current_image = priv->xvimage;

	dst = priv->xvimage->data + priv->image_width * 16 * slice_num;

	memcpy (dst,src[0],priv->image_width*16);
	dst = priv->xvimage->data + priv->image_width * priv->image_height + priv->image_width * 4 * slice_num;
	memcpy (dst, src[2],priv->image_width*4);
	dst = priv->xvimage->data + priv->image_width * priv->image_height * 5 / 4 + priv->image_width * 4 * slice_num;
	memcpy (dst, src[1],priv->image_width*4);

	return 0;  
}
#endif


static int x11_draw_slice (uint8_t *src[], int slice_num)
{
	struct x11_priv_s *priv = &x11_priv;
	uint8_t *dst;

	dst = priv->ImageData + priv->image_width * 16 * (priv->bpp/8) * slice_num;

	yuv2rgb (dst, src[0], src[1], src[2], 
			priv->image_width, 16, 
			priv->image_width*(priv->bpp/8), priv->image_width, priv->image_width/2 );

	return 0;
}


#ifdef LIBVO_XV
static int _xv_draw_frame (frame_t *frame)
{
	struct x11_priv_s *priv = &x11_priv;

	_check_event ();

	priv->current_image = frame->private;
	
	return 0;  
}
#endif

static int x11_draw_frame (frame_t *frame)
{
	struct x11_priv_s *priv = &x11_priv;

	yuv2rgb(priv->ImageData, frame->base[0], frame->base[1], frame->base[2],
		priv->image_width, priv->image_height, 
		priv->image_width*(priv->bpp/8), priv->image_width, priv->image_width/2 );

	return 0; 
}

static frame_t* x11_allocate_image_buffer (int width, int height, uint32_t format)
{
        frame_t *frame;

	if (!(frame = malloc (sizeof (frame_t))))
		return NULL;

#ifdef LIBVO_XV
	if (x11_priv.xv_port) {
		frame->private = _xv_create ();

		frame->base[0] = ((XvImage*) frame->private)->data;
		frame->base[2] = frame->base[0] + width * height;
		frame->base[1] = frame->base[0] + width * height * 5 / 4;

		return frame;
	}
	else
#endif

        //we only know how to do 4:2:0 planar yuv right now.
        if (!(frame->private = malloc (width * height * 3 / 2))) {
                free (frame);
                return NULL;
	}
	
	frame->base[0] = frame->private;
	frame->base[1] = frame->base[0] + width * height;
	frame->base[2] = frame->base[0] + width * height * 5 / 4;

        return frame;
}


void x11_free_image_buffer (frame_t* frame)
{
#ifdef LIBVO_XV
	if (x11_priv.xv_port)
		XFree(frame->private);
	else
#endif
	free (frame->base);
	free (frame);
}

#endif
