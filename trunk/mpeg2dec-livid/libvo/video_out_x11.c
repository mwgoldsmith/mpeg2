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
#include <string.h>	/* strcmp */
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


typedef struct x11_frame_s {
    vo_frame_t vo;

#ifdef LIBVO_XV
    XvImage * xvimage;
#endif
} x11_frame_t;

typedef struct x11_instance_s {
    vo_instance_t vo;
    int prediction_index;
    vo_frame_t * frame_ptr[3];
    x11_frame_t frame[3];

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
    XShmSegmentInfo shminfo;
#endif

#ifdef LIBVO_XV
    XvPortID port;
#endif
} x11_instance_t;

static int x11_get_visual_info (x11_instance_t * this)
{
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

static void x11_create_window (x11_instance_t * this, int width, int height)
{
    XSetWindowAttributes attr;

    attr.backing_store = NotUseful;
    attr.event_mask = 0;
    /* fucking sun blows me - you have to create a colormap there... */
    attr.colormap = XCreateColormap (this->display,
				     RootWindow (this->display,
						 this->vinfo.screen),
				     this->vinfo.visual, AllocNone);
    this->window =
	XCreateWindow (this->display, DefaultRootWindow (this->display),
		       0 /* x */, 0 /* y */, width, height,
		       0 /* border_width */, this->vinfo.depth,
		       InputOutput, this->vinfo.visual,
		       CWBackingStore | CWEventMask | CWColormap, &attr);
}

static void x11_create_gc (x11_instance_t * this)
{
    XGCValues gcValues;

    this->gc = XCreateGC (this->display, this->window, 0, &gcValues);
}

static int x11_create_image (x11_instance_t * this, int width, int height)
{
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

static int x11_yuv2rgb_init (x11_instance_t * this)
{
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

static int x11_common_setup (x11_instance_t * this, int width, int height,
			     int (* create_image) (x11_instance_t *, int, int),
			     void (* draw_image) (vo_frame_t * frame))
{
    if (x11_get_visual_info (this)) {
	fprintf (stderr, "No truecolor visual\n");
	return 1;
    }

    x11_create_window (this, width, height);
    x11_create_gc (this);

    if (create_image (this, width, height)) {
	fprintf (stderr, "Cannot create ximage\n");
	return 1;
    }

    if (x11_yuv2rgb_init (this)) {
	fprintf (stderr, "No support for non-native byte order\n");
	return 1;
    }

    if (libvo_common_alloc_frames ((vo_instance_t *)this, width, height,
				   sizeof (x11_frame_t), draw_image)) {
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

static void common_close (x11_instance_t * this)
{
    if (this->window) {
	XFreeGC (this->display, this->gc);
	XDestroyWindow (this->display, this->window);
    }
    if (this->display)
	XCloseDisplay (this->display);
}

static void x11_common_draw_frame (x11_instance_t * this, vo_frame_t * frame)
{
    yuv2rgb (this->imagedata, frame->base[0], frame->base[1], frame->base[2],
	     this->width, this->height,
	     this->stride, this->width, this->width >> 1);
}

static void x11_draw_frame (vo_frame_t * frame)
{
    x11_instance_t * this;

    this = (x11_instance_t *) frame->this;

    x11_common_draw_frame (this, frame);
    XPutImage (this->display, this->window, this->gc, this->ximage, 
	       0, 0, 0, 0, this->width, this->height);
    XFlush (this->display);
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

static void x11_close (vo_instance_t * _this)
{
    x11_instance_t * this = (x11_instance_t *)_this;

    libvo_common_free_frames ((vo_instance_t *)this);
    if (this->ximage)
	XDestroyImage (this->ximage);
    common_close (this);
}

vo_instance_t * vo_x11_setup (vo_instance_t * _this, int width, int height)
{
    x11_instance_t * this;

    if (_this != NULL)
	return NULL;
    this = malloc (sizeof (x11_instance_t));
    if (this == NULL)
	return NULL;

    this->display = XOpenDisplay (NULL);
    if (! (this->display)) {
	fprintf (stderr, "Can not open display\n");
	return NULL;
    }

    if (x11_common_setup (this, width, height, x11_create_image,
			  x11_draw_frame))
	return NULL;

    this->vo.reinit = vo_x11_setup;
    this->vo.close = x11_close;
    this->vo.get_frame = libvo_common_get_frame;

    return (vo_instance_t *)this;
}

#ifdef LIBVO_XSHM
static int xshm_check_extension (x11_instance_t * this)
{
    int major;
    int minor;
    Bool pixmaps;

    if (XShmQueryVersion (this->display, &major, &minor, &pixmaps) == 0)
	return 1;

    if ((major < 1) || ((major == 1) && (minor < 1)))
	return 1;

    return 0;
}

static int shmerror = 0;

static int x11_handle_error (Display * display, XErrorEvent * error)
{
    shmerror = 1;
    return 0;
}

static void * xshm_create_shm (x11_instance_t * this, int size)
{
    this->shminfo.shmid = shmget (IPC_PRIVATE, size, IPC_CREAT | 0777);
    if (this->shminfo.shmid == -1)
	return NULL;

    this->shminfo.shmaddr = shmat (this->shminfo.shmid, 0, 0);
    if (this->shminfo.shmaddr == (char *)-1)
	return NULL;

    /* on linux the IPC_RMID only kicks off once everyone detaches the shm */
    /* doing this early avoids shm leaks when we are interrupted. */
    /* this would break the solaris port though :-/ */
    /* shmctl (this->shminfo.shmid, IPC_RMID, 0); */

    /* XShmAttach fails on remote displays, so we have to catch this event */

    XSync (this->display, False);
    XSetErrorHandler (x11_handle_error);

    this->shminfo.readOnly = True;
    if (! (XShmAttach (this->display, &(this->shminfo))))
	shmerror = 1;

    XSync (this->display, False);
    XSetErrorHandler (NULL);
    if (shmerror)
	return NULL;

    return this->shminfo.shmaddr;
}

static void xshm_destroy_shm (x11_instance_t * this)
{
    if (this->shminfo.shmaddr != (char *)-1) {
	XShmDetach (this->display, &(this->shminfo));
	shmdt (this->shminfo.shmaddr);
    }
    if (this->shminfo.shmid != -1)
	shmctl (this->shminfo.shmid, IPC_RMID, 0);
}

static int xshm_create_image (x11_instance_t * this, int width, int height)
{
    this->ximage = XShmCreateImage (this->display,
				    this->vinfo.visual, this->vinfo.depth,
				    ZPixmap, NULL /* data */,
				    &(this->shminfo), width, height);
    if (this->ximage == NULL)
	return 1;

    this->ximage->data = xshm_create_shm (this, (this->ximage->bytes_per_line *
						 this->ximage->height));
    if (this->ximage->data == NULL)
	return 1;

    return 0;
}

static void xshm_draw_frame (vo_frame_t * frame)
{
    x11_instance_t * this;

    this = (x11_instance_t *) frame->this;

    x11_common_draw_frame (this, frame);

    XShmPutImage (this->display, this->window, this->gc, this->ximage, 
		  0, 0, 0, 0, this->width, this->height, False);
    XSync (this->display, False);
}

static void xshm_close (vo_instance_t * _this)
{
    x11_instance_t * this = (x11_instance_t *)_this;

    libvo_common_free_frames ((vo_instance_t *)this);
    if (this->ximage) {
	xshm_destroy_shm (this);
	XDestroyImage (this->ximage);
    }
    common_close (this);
}

vo_instance_t * vo_xshm_setup (vo_instance_t * _this, int width, int height)
{
    x11_instance_t * this;

    if (_this != NULL)
	return NULL;
    this = malloc (sizeof (x11_instance_t));
    if (this == NULL)
	return NULL;

    this->display = XOpenDisplay (NULL);
    if (! (this->display)) {
	fprintf (stderr, "Can not open display\n");
	return NULL;
    }

    if (xshm_check_extension (this)) {
	fprintf (stderr, "No xshm extension\n");
	return NULL;
    }

    if (x11_common_setup (this, width, height, xshm_create_image,
			  xshm_draw_frame))
	return NULL;

    this->vo.reinit = vo_xshm_setup;
    this->vo.close = xshm_close;
    this->vo.get_frame = libvo_common_get_frame;

    return (vo_instance_t *)this;
}
#endif

#ifdef LIBVO_XV
static int xv_check_extension (x11_instance_t * this)
{
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

static int xv_check_yv12 (x11_instance_t * this, XvPortID port)
{
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

static int xv_get_port (x11_instance_t * this)
{
    int adaptors;
    int i;
    unsigned long j;
    XvAdaptorInfo * adaptorInfo;

    XvQueryAdaptors (this->display, this->window, &adaptors, &adaptorInfo);

    for (i = 0; i < adaptors; i++)
	if (adaptorInfo[i].type & XvImageMask)
	    for (j = 0; j < adaptorInfo[i].num_ports; j++)
		if ((! (xv_check_yv12 (this, adaptorInfo[i].base_id + j))) &&
		    (XvGrabPort (this->display, adaptorInfo[i].base_id + j,
				 0) == Success)) {
		    this->port = adaptorInfo[i].base_id + j;
		    XvFreeAdaptorInfo (adaptorInfo);
		    return 0;
		}

    XvFreeAdaptorInfo (adaptorInfo);
    return 1;
}

static void xv_draw_frame (vo_frame_t * _frame)
{
    x11_frame_t * frame;
    x11_instance_t * this;

    frame = (x11_frame_t *)_frame;
    this = (x11_instance_t *)frame->vo.this;

    XvPutImage (this->display, this->port, this->window, this->gc,
		frame->xvimage, 0, 0, this->width, this->height,
		0, 0, this->width, this->height);
    XFlush (this->display);
}

static int xv_alloc_frames (x11_instance_t * this, int width, int height)
{
    int size;
    uint8_t * alloc;
    int i;

    size = width * height / 4;
    alloc = malloc (18 * size);
    if (alloc == NULL)
	return 1;

    for (i = 0; i < 3; i++) {
	this->frame_ptr[i] = (vo_frame_t *)(this->frame + i);
	this->frame[i].vo.base[0] = alloc;
	this->frame[i].vo.base[1] = alloc + 5 * size;
	this->frame[i].vo.base[2] = alloc + 4 * size;
	this->frame[i].vo.copy = NULL;
	this->frame[i].vo.draw = xv_draw_frame;
	this->frame[i].vo.this = (vo_instance_t *)this;
	this->frame[i].xvimage = XvCreateImage (this->display, this->port,
						FOURCC_YV12, alloc,
						width, height);
	if ((this->frame[i].xvimage == NULL) ||
	    (this->frame[i].xvimage->data_size != 6 * size))	/* FIXME */
	    return 1;

	alloc += 6 * size;
    }

    return 0;
}

static void xv_free_frames (x11_instance_t * this)
{
    int i;

    free (this->frame[0].vo.base[0]);
    for (i = 0; i < 3; i++)
	XFree (this->frame[i].xvimage);
}

static int xv_common_setup (x11_instance_t * this, int width, int height,
			    int (* alloc_frames) (x11_instance_t *, int, int))
{
    if (xv_check_extension (this)) {
	fprintf (stderr, "No xv extension\n");
	return 1;
    }

    if (x11_get_visual_info (this)) {
	fprintf (stderr, "No truecolor visual\n");
	return 1;
    }

    x11_create_window (this, width, height);
    x11_create_gc (this);

    if (xv_get_port (this)) {
	fprintf (stderr, "Cannot find xv port\n");
	return 1;
    }

    if (alloc_frames (this, width, height)) {
	fprintf (stderr, "Cannot create xvimage\n");
	return 1;
    }

    /* FIXME set WM_DELETE_WINDOW protocol ? to avoid shm leaks */

    XMapWindow (this->display, this->window);

    this->width = width;
    this->height = height;

    return 0;
}

static void xv_close (vo_instance_t * _this)
{
    x11_instance_t * this = (x11_instance_t *)_this;

    xv_free_frames (this);
    XvUngrabPort (this->display, this->port, 0);
    common_close (this);
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

vo_instance_t * vo_xv_setup (vo_instance_t * _this, int width, int height)
{
    x11_instance_t * this;

    if (_this != NULL)
	return NULL;
    this = malloc (sizeof (x11_instance_t));
    if (this == NULL)
	return NULL;

    this->display = XOpenDisplay (NULL);
    if (! (this->display)) {
	fprintf (stderr, "Can not open display\n");
	return NULL;
    }

    if (xv_common_setup (this, width, height, xv_alloc_frames))
	return NULL;

    this->vo.reinit = vo_xv_setup;
    this->vo.close = xv_close;
    this->vo.get_frame = libvo_common_get_frame;

    return (vo_instance_t *)this;
}

static void xvshm_draw_frame (vo_frame_t * _frame)
{
    x11_frame_t * frame;
    x11_instance_t * this;

    frame = (x11_frame_t *)_frame;
    this = (x11_instance_t *)frame->vo.this;

    XvShmPutImage (this->display, this->port, this->window, this->gc,
		   frame->xvimage, 0, 0, this->width, this->height,
		   0, 0, this->width, this->height, False);
    XSync (this->display, False);
}

static int xvshm_alloc_frames (x11_instance_t * this, int width, int height)
{
    int size;
    uint8_t * alloc;
    int i;

    size = width * height / 4;
    alloc = xshm_create_shm (this, 18 * size);
    if (alloc == NULL)
	return 1;

    for (i = 0; i < 3; i++) {
	this->frame_ptr[i] = (vo_frame_t *)(this->frame + i);
	this->frame[i].vo.base[0] = alloc;
	this->frame[i].vo.base[1] = alloc + 5 * size;
	this->frame[i].vo.base[2] = alloc + 4 * size;
	this->frame[i].vo.copy = NULL;
	this->frame[i].vo.draw = xvshm_draw_frame;
	this->frame[i].vo.this = (vo_instance_t *)this;
	this->frame[i].xvimage =
	    XvShmCreateImage (this->display, this->port, FOURCC_YV12, alloc,
			      width, height, &(this->shminfo));
	if ((this->frame[i].xvimage == NULL) ||
	    (this->frame[i].xvimage->data_size != 6 * size))	/* FIXME */
	    return 1;

	alloc += 6 * size;
    }

    return 0;
}

static void xvshm_free_frames (x11_instance_t * this)
{
    int i;

    xshm_destroy_shm (this);
    for (i = 0; i < 3; i++)
	XFree (this->frame[i].xvimage);
}

static void xvshm_close (vo_instance_t * _this)
{
    x11_instance_t * this = (x11_instance_t *)_this;

    xvshm_free_frames (this);
    XvUngrabPort (this->display, this->port, 0);
    common_close (this);
}

vo_instance_t * vo_xvshm_setup (vo_instance_t * _this, int width, int height)
{
    x11_instance_t * this;

    if (_this != NULL)
	return NULL;
    this = malloc (sizeof (x11_instance_t));
    if (this == NULL)
	return NULL;

    this->display = XOpenDisplay (NULL);
    if (! (this->display)) {
	fprintf (stderr, "Can not open display\n");
	return NULL;
    }

    if (xshm_check_extension (this)) {
	fprintf (stderr, "No xshm extension\n");
	return NULL;
    }

    if (xv_common_setup (this, width, height, xvshm_alloc_frames))
	return NULL;

    this->vo.reinit = vo_xvshm_setup;
    this->vo.close = xvshm_close;
    this->vo.get_frame = libvo_common_get_frame;

    return (vo_instance_t *)this;
}
#endif

#endif
