/* make sure shm is not reused while the server uses it */

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
#include <string.h>	/* memcmp, strcmp */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <inttypes.h>

#ifdef LIBVO_XSHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
/* since it doesn't seem to be defined on some platforms */
int XShmGetEventBase (Display *);
#endif

#ifdef LIBVO_XV
#include <X11/extensions/Xvlib.h>
#define FOURCC_YV12 0x32315659
#endif

#include "video_out.h"
#include "video_out_internal.h"
#include "yuv2rgb.h"


typedef struct x11_instance_s {
    vo_instance_t vo;

    /* local data */
    uint8_t * imagedata;
    int width;
    int height;
    int stride;

    /* X11 related variables */
    Display * display;
    Window window;
    GC gc;
    XVisualInfo vinfo;
    XImage * ximage;

#ifdef LIBVO_XSHM
    int error;
    int shmindex;
    XShmSegmentInfo shminfo[3];
#endif

#ifdef LIBVO_XV
    XvPortID port;
#endif
} x11_instance_t;

static x11_instance_t x11_static_instance;	/* FIXME */

static int x11_get_visual_info (void)
{
    x11_instance_t * this = &x11_static_instance;
    XVisualInfo visualTemplate;
    XVisualInfo * XvisualInfoTable;
    XVisualInfo * XvisualInfo;
    int number;
    int i;

    /* list truecolor visuals for the default screen */
    visualTemplate.class = TrueColor;
    visualTemplate.screen = DefaultScreen (this->display);
    XvisualInfoTable = XGetVisualInfo (this->display,
				       VisualScreenMask | VisualClassMask,
				       &visualTemplate, &number);
    if (XvisualInfoTable == NULL)
	return 1;

    /* find the visual with the highest depth */
    XvisualInfo = XvisualInfoTable;
    for (i = 1; i < number; i++)
	if (XvisualInfoTable[i].depth > XvisualInfo->depth)
	    XvisualInfo = XvisualInfoTable + i;

    this->vinfo = *XvisualInfo;
    XFree (XvisualInfoTable);
    return 0;
}

static void x11_create_window (int width, int height)
{
    x11_instance_t * this = &x11_static_instance;
    XSetWindowAttributes attr;

    attr.background_pixmap = None;
    attr.backing_store = NotUseful;
    attr.event_mask = 0;
    this->window =
	XCreateWindow (this->display, DefaultRootWindow (this->display),
		       0 /* x */, 0 /* y */, width, height,
		       0 /* border_width */, this->vinfo.depth,
		       InputOutput, this->vinfo.visual,
		       CWBackPixmap | CWBackingStore | CWEventMask, &attr);
}

static void x11_create_gc (void)
{
    x11_instance_t * this = &x11_static_instance;
    XGCValues gcValues;

    this->gc = XCreateGC (this->display, this->window, 0, &gcValues);
}

static int x11_create_image (int width, int height)
{
    x11_instance_t * this = &x11_static_instance;

    this->ximage = XCreateImage (this->display,
				 this->vinfo.visual, this->vinfo.depth,
				 ZPixmap, 0, NULL /* data */,
				 width, height, 16, 0);
    if (this->ximage == NULL)
	return 1;

    this->ximage->data =
	malloc (this->ximage->bytes_per_line * this->ximage->height);
    if (this->ximage->data == NULL)
	return 1;

    return 0;
}

static int x11_yuv2rgb_init (void)
{
    x11_instance_t * this = &x11_static_instance;
    int mode;

    /* If we have blue in the lowest bit then "obviously" RGB */
    /* (the guy who wrote this convention never heard of endianness ?) */
    mode = ((this->ximage->blue_mask & 0x01)) ? MODE_RGB : MODE_BGR;

#ifdef WORDS_BIGENDIAN 
    if (this->ximage->byte_order != MSBFirst)
	return 1;
#else
    if (this->ximage->byte_order != LSBFirst)
	return 1;
#endif

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

    yuv2rgb_init (((this->vinfo.depth == 24) ?
		   this->ximage->bits_per_pixel : this->vinfo.depth), mode);

    return 0;
}

static int x11_common_setup (int width, int height,
			     int (* create_image) (int, int))
{
    x11_instance_t * this = &x11_static_instance;

    if (x11_get_visual_info ()) {
	fprintf (stderr, "No truecolor visual\n");
	return 1;
    }

    x11_create_window (width, height);
    x11_create_gc ();

    if (create_image (width, height)) {
	fprintf (stderr, "Cannot create ximage\n");
	return 1;
    }

    if (x11_yuv2rgb_init ()) {
	fprintf (stderr, "No support for non-native byte order\n");
	return 1;
    }

    if (libvo_common_alloc_frames (libvo_common_alloc_frame, width, height)) {
	fprintf (stderr, "Can not allocate yuv backing buffers\n");
	return 1;
    }

    /* FIXME set WM_DELETE_WINDOW protocol ? to avoid shm leaks */

    XMapWindow (this->display, this->window);

    this->width = width;
    this->height = height;
    this->imagedata = (unsigned char *) this->ximage->data;
    this->stride = this->ximage->bytes_per_line;
    return 0;
}

static void common_close (void)
{
    x11_instance_t * this = &x11_static_instance;

    if (this->window) {
	XFreeGC (this->display, this->gc);
	XDestroyWindow (this->display, this->window);
    }
    if (this->display)
	XCloseDisplay (this->display);
}

extern vo_instance_t x11_vo_instance;

vo_instance_t * vo_x11_setup (vo_instance_t * _this, int width, int height)
{
    x11_instance_t * this;

    if (_this != NULL)
	return NULL;
    this = &x11_static_instance;

    this->display = XOpenDisplay (NULL);
    if (! (this->display)) {
	fprintf (stderr, "Can not open display\n");
	return NULL;
    }

    if (x11_common_setup (width, height, x11_create_image))
	return NULL;

    this->vo = x11_vo_instance;

    return (vo_instance_t *)this;
}

static int x11_close (vo_instance_t * _this)
{
    x11_instance_t * this = (x11_instance_t *)_this;

    libvo_common_free_frames (libvo_common_free_frame);
    if (this->ximage)
	XDestroyImage (this->ximage);
    common_close ();
    return 0;
}

#if 0
static int x11_draw_slice (uint8_t * src[], int slice_num)
{
    x11_instance_t * this = &x11_static_instance;

    yuv2rgb (this->imagedata + this->stride * 16 * slice_num,
	     src[0], src[1], src[2], this->width, 16,
	     this->stride, this->width, this->width >> 1);

    return 0;
}
#endif

static void x11_common_draw_frame (frame_t * frame)
{
    x11_instance_t * this = &x11_static_instance;

    yuv2rgb (this->imagedata, frame->base[0], frame->base[1], frame->base[2],
	     this->width, this->height,
	     this->stride, this->width, this->width >> 1);
}

static void x11_draw_frame (frame_t * frame)
{
    x11_instance_t * this = &x11_static_instance;

    x11_common_draw_frame (frame);

    XPutImage (this->display, this->window, this->gc, this->ximage, 
	       0, 0, 0, 0, this->width, this->height);
    XFlush (this->display);
}

static vo_instance_t x11_vo_instance = {
    vo_x11_setup, x11_close, libvo_common_get_frame, x11_draw_frame
};

#ifdef LIBVO_XSHM
static int xshm_check_extension (void)
{
    x11_instance_t * this = &x11_static_instance;
    int major;
    int minor;
    Bool pixmaps;

    if (XShmQueryVersion (this->display, &major, &minor, &pixmaps) == 0)
	return 1;

    if ((major < 1) || ((major == 1) && (minor < 1)))
	return 1;

    return 0;
}

static int x11_handle_error (Display * display, XErrorEvent * error)
{
    x11_instance_t * this = &x11_static_instance;

    this->error = 1;
    return 0;
}

static void * xshm_create_shm (int size)
{
    x11_instance_t * this = &x11_static_instance;
    XShmSegmentInfo * shminfo;

    shminfo = this->shminfo + this->shmindex;

    shminfo->shmid = shmget (IPC_PRIVATE, size, IPC_CREAT | 0777);
    if (shminfo->shmid == -1)
	return NULL;

    shminfo->shmaddr = shmat (shminfo->shmid, 0, 0);
    if (shminfo->shmaddr == (char *)-1)
	return NULL;

    /* on linux the IPC_RMID only kicks off once everyone detaches the shm */
    /* doing this early avoids shm leaks when we are interrupted. */
    /* this would break the solaris port though :-/ */
    /* shmctl (shminfo->shmid, IPC_RMID, 0); */

    /* XShmAttach fails on remote displays, so we have to catch this event */

    XSync (this->display, False);
    this->error = 0;
    XSetErrorHandler (x11_handle_error);

    shminfo->readOnly = True;
    if (! (XShmAttach (this->display, shminfo)))
	return NULL;

    XSync (this->display, False);
    XSetErrorHandler (NULL);
    if (this->error)
	return NULL;

    this->shmindex++;
    return shminfo->shmaddr;
}

static void xshm_destroy_shm (void)
{
    x11_instance_t * this = &x11_static_instance;
    XShmSegmentInfo * shminfo;

    this->shmindex--;
    shminfo = this->shminfo + this->shmindex;

    if (shminfo->shmaddr != (char *)-1) {
	XShmDetach (this->display, shminfo);
	shmdt (shminfo->shmaddr);
    }
    if (shminfo->shmid != -1)
	shmctl (shminfo->shmid, IPC_RMID, 0);
}

static int xshm_create_image (int width, int height)
{
    x11_instance_t * this = &x11_static_instance;

    this->ximage = XShmCreateImage (this->display,
				    this->vinfo.visual, this->vinfo.depth,
				    ZPixmap, NULL /* data */,
				    this->shminfo, width, height);
    if (this->ximage == NULL)
	return 1;

    this->ximage->data = xshm_create_shm (this->ximage->bytes_per_line *
					  this->ximage->height);
    if (this->ximage->data == NULL)
	return 1;

    return 0;
}

extern vo_instance_t xshm_vo_instance;

vo_instance_t * vo_xshm_setup (vo_instance_t * _this, int width, int height)
{
    x11_instance_t * this;

    if (_this != NULL)
	return NULL;
    this = &x11_static_instance;

    this->display = XOpenDisplay (NULL);
    if (! (this->display)) {
	fprintf (stderr, "Can not open display\n");
	return NULL;
    }

    if (xshm_check_extension ()) {
	fprintf (stderr, "No xshm extension\n");
	return NULL;
    }

    if (x11_common_setup (width, height, xshm_create_image))
	return NULL;

    this->vo = xshm_vo_instance;

    return (vo_instance_t *)this;
}

static int xshm_close (vo_instance_t * _this)
{
    x11_instance_t * this = (x11_instance_t *)_this;

    libvo_common_free_frames (libvo_common_free_frame);
    if (this->ximage) {
	xshm_destroy_shm ();
	XDestroyImage (this->ximage);
    }
    common_close ();
    return 0;
}

static void xshm_draw_frame (frame_t * frame)
{
    x11_instance_t * this = &x11_static_instance;

    x11_common_draw_frame (frame);

    XShmPutImage (this->display, this->window, this->gc, this->ximage, 
		  0, 0, 0, 0, this->width, this->height, False);
    XSync (this->display, False);
}

static vo_instance_t xshm_vo_instance = {
    vo_xshm_setup, xshm_close, libvo_common_get_frame, xshm_draw_frame
};
#endif

#ifdef LIBVO_XV
static int xv_check_extension (void)
{
    x11_instance_t * this = &x11_static_instance;
    unsigned int version;
    unsigned int release;
    unsigned int dummy;

    if (XvQueryExtension (this->display, &version, &release,
			  &dummy, &dummy, &dummy) != Success)
	return 1;

    if ((version < 2) || ((version == 2) && (release < 2)))
	return 1;

    return 0;
}

static int xv_check_yv12 (XvPortID port)
{
    x11_instance_t * this = &x11_static_instance;
    XvImageFormatValues * formatValues;
    int formats;
    int i;

    formatValues = XvListImageFormats (this->display, port, &formats);
    for (i = 0; i < formats; i++)
	if ((formatValues[i].id == FOURCC_YV12) &&
	    (! (strcmp (formatValues[i].guid, "YV12")))) {
	    XFree (formatValues);
	    return 0;
	}
    XFree (formatValues);
    return 1;
}

static int xv_get_port (void)
{
    x11_instance_t * this = &x11_static_instance;
    int adaptors;
    int i;
    unsigned long j;
    XvAdaptorInfo * adaptorInfo;

    XvQueryAdaptors (this->display, this->window, &adaptors, &adaptorInfo);

    for (i = 0; i < adaptors; i++)
	if (adaptorInfo[i].type & XvImageMask)
	    for (j = 0; j < adaptorInfo[i].num_ports; j++)
		if ((! (xv_check_yv12 (adaptorInfo[i].base_id + j))) &&
		    (XvGrabPort (this->display, adaptorInfo[i].base_id + j,
				 0) == Success)) {
		    this->port = adaptorInfo[i].base_id + j;
		    XvFreeAdaptorInfo (adaptorInfo);
		    return 0;
		}

    XvFreeAdaptorInfo (adaptorInfo);
    return 1;
}

static int xv_alloc_frame (frame_t * frame, int width, int height)
{
    x11_instance_t * this = &x11_static_instance;
    XvImage * xvimage;

    xvimage = XvCreateImage (this->display, this->port, FOURCC_YV12,
			     NULL /* data */, width, height);
    if ((xvimage == NULL) || (xvimage->data_size == 0))
	return 1;

    xvimage->data = malloc (xvimage->data_size);
    if (xvimage->data == NULL)
	return 1;

    frame->private = xvimage;
    frame->base[0] = xvimage->data;
    frame->base[1] = xvimage->data + width * height * 5 / 4;
    frame->base[2] = xvimage->data + width * height;

    return 0;
}

static void xv_free_frame (frame_t * frame)
{
    XFree (frame->private);
}

static int xv_common_setup (int width, int height,
			    int (* alloc_frame) (frame_t *, int, int))
{
    x11_instance_t * this = &x11_static_instance;

    if (xv_check_extension ()) {
	fprintf (stderr, "No xv extension\n");
	return 1;
    }

    if (x11_get_visual_info ()) {
	fprintf (stderr, "No truecolor visual\n");
	return 1;
    }

    x11_create_window (width, height);
    x11_create_gc ();

    if (xv_get_port ()) {
	fprintf (stderr, "Cannot find xv port\n");
	return 1;
    }

    if (libvo_common_alloc_frames (alloc_frame, width, height)) {
	fprintf (stderr, "Cannot create xvimage\n");
	return 1;
    }

    /* FIXME set WM_DELETE_WINDOW protocol ? to avoid shm leaks */

    XMapWindow (this->display, this->window);

    this->width = width;
    this->height = height;

    return 0;
}

extern vo_instance_t xv_vo_instance;

vo_instance_t * vo_xv_setup (vo_instance_t * _this, int width, int height)
{
    x11_instance_t * this;

    if (_this != NULL)
	return NULL;
    this = &x11_static_instance;

    this->display = XOpenDisplay (NULL);
    if (! (this->display)) {
	fprintf (stderr, "Can not open display\n");
	return NULL;
    }

    if (xv_common_setup (width, height, xv_alloc_frame))
	return NULL;

    this->vo = xv_vo_instance;

    return (vo_instance_t *)this;
}

static int xv_close (vo_instance_t * _this)
{
    x11_instance_t * this = (x11_instance_t *)_this;

    libvo_common_free_frames (xv_free_frame);
    XvUngrabPort (this->display, this->port, 0);
    common_close ();
    return 0;
}

#if 0
static int xv_draw_slice (uint8_t * src[], int slice_num)
{
    x11_instance_t * this = &x11_static_instance;

    memcpy (this->xvimage->data + this->width * 16 * slice_num,
	    src[0], this->width * 16);
    memcpy (this->xvimage->data + this->width * (this->height + 4 * slice_num),
	    src[2], this->width * 4);
    memcpy (this->xvimage->data +
	    this->width * (this->height * 5 / 4 + 4 * slice_num),
	    src[1], this->width * 4);

    return 0;
}
#endif

static void xv_draw_frame (frame_t * frame)
{
    x11_instance_t * this = &x11_static_instance;

    XvPutImage (this->display, this->port, this->window, this->gc,
		frame->private, 0, 0, this->width, this->height,
		0, 0, this->width, this->height);
    XFlush (this->display);
}

static vo_instance_t xv_vo_instance = {
    vo_xv_setup, xv_close, libvo_common_get_frame, xv_draw_frame
};

static int xvshm_alloc_frame (frame_t * frame, int width, int height)
{
    x11_instance_t * this = &x11_static_instance;
    XvImage * xvimage;

    xvimage = XvShmCreateImage (this->display, this->port, FOURCC_YV12,
				NULL /* data */, width, height,
				this->shminfo + this->shmindex);
    if ((xvimage == NULL) || (xvimage->data_size == 0))
	return 1;

    xvimage->data = xshm_create_shm (xvimage->data_size);
    if (xvimage->data == NULL)
	return 1;

    frame->private = xvimage;
    frame->base[0] = xvimage->data;
    frame->base[1] = xvimage->data + width * height * 5 / 4;
    frame->base[2] = xvimage->data + width * height;

    return 0;
}

static void xvshm_free_frame (frame_t * frame)
{
    xshm_destroy_shm ();
    XFree (frame->private);
}

extern vo_instance_t xvshm_vo_instance;

vo_instance_t * vo_xvshm_setup (vo_instance_t * _this, int width, int height)
{
    x11_instance_t * this;

    if (_this != NULL)
	return NULL;
    this = &x11_static_instance;

    this->display = XOpenDisplay (NULL);
    if (! (this->display)) {
	fprintf (stderr, "Can not open display\n");
	return NULL;
    }

    if (xshm_check_extension ()) {
	fprintf (stderr, "No xshm extension\n");
	return NULL;
    }

    if (xv_common_setup (width, height, xvshm_alloc_frame))
	return NULL;

    this->vo = xvshm_vo_instance;

    return (vo_instance_t *)this;
}

static int xvshm_close (vo_instance_t * _this)
{
    x11_instance_t * this = (x11_instance_t *)_this;

    libvo_common_free_frames (xvshm_free_frame);
    XvUngrabPort (this->display, this->port, 0);
    common_close ();
    return 0;
}

static void xvshm_draw_frame (frame_t * frame)
{
    x11_instance_t * this = &x11_static_instance;

    XvShmPutImage (this->display, this->port, this->window, this->gc,
		   frame->private, 0, 0, this->width, this->height,
		   0, 0, this->width, this->height, False);
    XSync (this->display, False);
}

static vo_instance_t xvshm_vo_instance = {
    vo_xvshm_setup, xvshm_close, libvo_common_get_frame, xvshm_draw_frame
};
#endif

#endif
