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


static struct x11_priv_s {
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
} x11_priv;

static int x11_get_visual_info (void)
{
    struct x11_priv_s * priv = &x11_priv;
    XVisualInfo visualTemplate;
    XVisualInfo * XvisualInfoTable;
    XVisualInfo * XvisualInfo;
    int number;
    int i;

    /* list truecolor visuals for the default screen */
    visualTemplate.class = TrueColor;
    visualTemplate.screen = DefaultScreen (priv->display);
    XvisualInfoTable = XGetVisualInfo (priv->display,
				       VisualScreenMask | VisualClassMask,
				       &visualTemplate, &number);
    if (XvisualInfoTable == NULL)
	return 1;

    /* find the visual with the highest depth */
    XvisualInfo = XvisualInfoTable;
    for (i = 1; i < number; i++)
	if (XvisualInfoTable[i].depth > XvisualInfo->depth)
	    XvisualInfo = XvisualInfoTable + i;

    priv->vinfo = *XvisualInfo;
    XFree (XvisualInfoTable);
    return 0;
}

static void x11_create_window (int width, int height)
{
    struct x11_priv_s * priv = &x11_priv;
    XSetWindowAttributes attr;

    attr.background_pixmap = None;
    attr.backing_store = NotUseful;
    attr.event_mask = 0;
    priv->window =
	XCreateWindow (priv->display, DefaultRootWindow (priv->display),
		       0 /* x */, 0 /* y */, width, height,
		       0 /* border_width */, priv->vinfo.depth,
		       InputOutput, priv->vinfo.visual,
		       CWBackPixmap | CWBackingStore | CWEventMask, &attr);
}

static void x11_create_gc (void)
{
    struct x11_priv_s * priv = &x11_priv;
    XGCValues gcValues;

    priv->gc = XCreateGC (priv->display, priv->window, 0, &gcValues);
}

static int x11_create_image (int width, int height)
{
    struct x11_priv_s * priv = &x11_priv;

    priv->ximage = XCreateImage (priv->display,
				 priv->vinfo.visual, priv->vinfo.depth,
				 ZPixmap, 0, NULL /* data */,
				 width, height, 16, 0);
    if (priv->ximage == NULL)
	return 1;

    priv->ximage->data =
	malloc (priv->ximage->bytes_per_line * priv->ximage->height);
    if (priv->ximage->data == NULL)
	return 1;

    return 0;
}

static int x11_yuv2rgb_init (void)
{
    struct x11_priv_s * priv = &x11_priv;
    int mode;

    /* If we have blue in the lowest bit then "obviously" RGB */
    /* (the guy who wrote this convention never heard of endianness ?) */
    mode = ((priv->ximage->blue_mask & 0x01)) ? MODE_RGB : MODE_BGR;

#ifdef WORDS_BIGENDIAN 
    if (priv->ximage->byte_order != MSBFirst)
	return 1;
#else
    if (priv->ximage->byte_order != LSBFirst)
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

    yuv2rgb_init (((priv->vinfo.depth == 24) ?
		   priv->ximage->bits_per_pixel : priv->vinfo.depth), mode);

    return 0;
}

static int x11_common_setup (int width, int height,
			     int (* create_image) (int, int))
{
    struct x11_priv_s * priv = &x11_priv;

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

    XMapWindow (priv->display, priv->window);

    priv->width = width;
    priv->height = height;
    priv->imagedata = (unsigned char *) priv->ximage->data;
    priv->stride = priv->ximage->bytes_per_line;
    return 0;
}

static void common_close (void)
{
    struct x11_priv_s * priv = &x11_priv;

    if (priv->window) {
	XFreeGC (priv->display, priv->gc);
	XDestroyWindow (priv->display, priv->window);
    }
    if (priv->display)
	XCloseDisplay (priv->display);
}

static int x11_setup (int width, int height)
{
    struct x11_priv_s * priv = &x11_priv;

    priv->display = XOpenDisplay (NULL);
    if (! (priv->display)) {
	fprintf (stderr, "Can not open display\n");
	return 1;
    }

    return x11_common_setup (width, height, x11_create_image);
}

static int x11_close (void)
{
    struct x11_priv_s * priv = &x11_priv;

    libvo_common_free_frames (libvo_common_free_frame);
    if (priv->ximage)
	XDestroyImage (priv->ximage);
    common_close ();
    return 0;
}

#if 0
static int x11_draw_slice (uint8_t *src[], int slice_num)
{
    struct x11_priv_s * priv = &x11_priv;

    yuv2rgb (priv->imagedata + priv->stride * 16 * slice_num,
	     src[0], src[1], src[2], priv->width, 16,
	     priv->stride, priv->width, priv->width >> 1);

    return 0;
}
#endif

static void x11_common_draw_frame (frame_t * frame)
{
    struct x11_priv_s * priv = &x11_priv;

    yuv2rgb (priv->imagedata, frame->base[0], frame->base[1], frame->base[2],
	     priv->width, priv->height,
	     priv->stride, priv->width, priv->width >> 1);
}

static void x11_draw_frame (frame_t * frame)
{
    struct x11_priv_s * priv = &x11_priv;

    x11_common_draw_frame (frame);

    XPutImage (priv->display, priv->window, priv->gc, priv->ximage, 
	       0, 0, 0, 0, priv->width, priv->height);
    XFlush (priv->display);
}

vo_output_video_t video_out_x11 = {
    "x11",
    x11_setup, x11_close, libvo_common_get_frame, x11_draw_frame
};

#ifdef LIBVO_XSHM
static int xshm_check_extension (void)
{
    struct x11_priv_s * priv = &x11_priv;
    int major;
    int minor;
    Bool pixmaps;

    if (XShmQueryVersion (priv->display, &major, &minor, &pixmaps) == 0)
	return 1;

    if ((major < 1) || ((major == 1) && (minor < 1)))
	return 1;

    return 0;
}

static int x11_handle_error (Display * display, XErrorEvent * error)
{
    struct x11_priv_s * priv = &x11_priv;

    priv->error = 1;
    return 0;
}

static void * xshm_create_shm (int size)
{
    struct x11_priv_s * priv = &x11_priv;
    XShmSegmentInfo * shminfo;

    shminfo = priv->shminfo + priv->shmindex;

    shminfo->shmid = shmget (IPC_PRIVATE, size, IPC_CREAT | 0777);
    if (shminfo->shmid == -1)
	return NULL;

    shminfo->shmaddr = shmat (shminfo->shmid, 0, 0);
    if (shminfo->shmaddr == (char *)-1)
	return NULL;

    /* XShmAttach fails on remote displays, so we have to catch this event */

    XSync (priv->display, False);
    priv->error = 0;
    XSetErrorHandler (x11_handle_error);

    shminfo->readOnly = True;
    if (! (XShmAttach (priv->display, shminfo)))
	return NULL;

    XSync (priv->display, False);
    XSetErrorHandler (NULL);
    if (priv->error)
	return NULL;

    shmctl (shminfo->shmid, IPC_RMID, 0);

    priv->shmindex++;
    return shminfo->shmaddr;
}

static void xshm_destroy_shm (void)
{
    struct x11_priv_s * priv = &x11_priv;
    XShmSegmentInfo * shminfo;

    priv->shmindex--;
    shminfo = priv->shminfo + priv->shmindex;

    if (shminfo->shmaddr != (char *)-1) {
	XShmDetach (priv->display, shminfo);
	shmdt (shminfo->shmaddr);
    }
}

static int xshm_create_image (int width, int height)
{
    struct x11_priv_s * priv = &x11_priv;

    priv->ximage = XShmCreateImage (priv->display,
				    priv->vinfo.visual, priv->vinfo.depth,
				    ZPixmap, NULL /* data */,
				    priv->shminfo, width, height);
    if (priv->ximage == NULL)
	return 1;

    priv->ximage->data = xshm_create_shm (priv->ximage->bytes_per_line *
					  priv->ximage->height);
    if (priv->ximage->data == NULL)
	return 1;

    return 0;
}

static int xshm_setup (int width, int height)
{
    struct x11_priv_s * priv = &x11_priv;

    priv->display = XOpenDisplay (NULL);
    if (! (priv->display)) {
	fprintf (stderr, "Can not open display\n");
	return 1;
    }

    if (xshm_check_extension ()) {
	fprintf (stderr, "No xshm extension\n");
	return 1;
    }

    return x11_common_setup (width, height, xshm_create_image);
}

static int xshm_close (void)
{
    struct x11_priv_s * priv = &x11_priv;

    libvo_common_free_frames (libvo_common_free_frame);
    if (priv->ximage) {
	xshm_destroy_shm ();
	XDestroyImage (priv->ximage);
    }
    common_close ();
    return 0;
}

static void xshm_draw_frame (frame_t * frame)
{
    struct x11_priv_s * priv = &x11_priv;

    x11_common_draw_frame (frame);

    XShmPutImage (priv->display, priv->window, priv->gc, priv->ximage, 
		  0, 0, 0, 0, priv->width, priv->height, False);
    XSync (priv->display, False);
}

vo_output_video_t video_out_xshm = {
    "xshm",
    xshm_setup, xshm_close, libvo_common_get_frame, xshm_draw_frame
};
#endif

#ifdef LIBVO_XV
static int xv_check_extension (void)
{
    struct x11_priv_s * priv = &x11_priv;
    unsigned int version;
    unsigned int release;
    unsigned int dummy;

    if (XvQueryExtension (priv->display, &version, &release,
			  &dummy, &dummy, &dummy) != Success)
	return 1;

    if ((version < 2) || ((version == 2) && (release < 2)))
	return 1;

    return 0;
}

static int xv_check_yv12 (XvPortID port)
{
    struct x11_priv_s * priv = &x11_priv;
    XvImageFormatValues * formatValues;
    int formats;
    int i;

    formatValues = XvListImageFormats (priv->display, port, &formats);
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
    struct x11_priv_s * priv = &x11_priv;
    int adaptors;
    int i;
    unsigned long j;
    XvAdaptorInfo * adaptorInfo;

    XvQueryAdaptors (priv->display, priv->window, &adaptors, &adaptorInfo);

    for (i = 0; i < adaptors; i++)
	if (adaptorInfo[i].type & XvImageMask)
	    for (j = 0; j < adaptorInfo[i].num_ports; j++)
		if ((! (xv_check_yv12 (adaptorInfo[i].base_id + j))) &&
		    (XvGrabPort (priv->display, adaptorInfo[i].base_id + j,
				 0) == Success)) {
		    priv->port = adaptorInfo[i].base_id + j;
		    XvFreeAdaptorInfo (adaptorInfo);
		    return 0;
		}

    XvFreeAdaptorInfo (adaptorInfo);
    return 1;
}

static int xv_alloc_frame (frame_t * frame, int width, int height)
{
    struct x11_priv_s * priv = &x11_priv;
    XvImage * xvimage;

    xvimage = XvCreateImage (priv->display, priv->port, FOURCC_YV12,
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
    struct x11_priv_s * priv = &x11_priv;

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

    XMapWindow (priv->display, priv->window);

    priv->width = width;
    priv->height = height;

    return 0;
}

static int xv_setup (int width, int height)
{
    struct x11_priv_s * priv = &x11_priv;

    priv->display = XOpenDisplay (NULL);
    if (! (priv->display)) {
	fprintf (stderr, "Can not open display\n");
	return 1;
    }

    return xv_common_setup (width, height, xv_alloc_frame);
}

static int xv_close (void)
{
    struct x11_priv_s * priv = &x11_priv;

    libvo_common_free_frames (xv_free_frame);
    XvUngrabPort (priv->display, priv->port, 0);
    common_close ();
    return 0;
}

#if 0
static int xv_draw_slice (uint8_t *src[], int slice_num)
{
    struct x11_priv_s * priv = &x11_priv;

    memcpy (priv->xvimage->data + priv->width * 16 * slice_num,
	    src[0], priv->width * 16);
    memcpy (priv->xvimage->data + priv->width * (priv->height + 4 * slice_num),
	    src[2], priv->width * 4);
    memcpy (priv->xvimage->data +
	    priv->width * (priv->height * 5 / 4 + 4 * slice_num),
	    src[1], priv->width * 4);

    return 0;
}
#endif

static void xv_draw_frame (frame_t * frame)
{
    struct x11_priv_s * priv = &x11_priv;

    XvPutImage (priv->display, priv->port, priv->window, priv->gc,
		frame->private, 0, 0, priv->width, priv->height,
		0, 0, priv->width, priv->height);
    XFlush (priv->display);
}

vo_output_video_t video_out_xv = {
    "xv",
    xv_setup, xv_close, libvo_common_get_frame, xv_draw_frame
};

static int xvshm_alloc_frame (frame_t * frame, int width, int height)
{
    struct x11_priv_s * priv = &x11_priv;
    XvImage * xvimage;

    xvimage = XvShmCreateImage (priv->display, priv->port, FOURCC_YV12,
				NULL /* data */, width, height,
				priv->shminfo + priv->shmindex);
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

static int xvshm_setup (int width, int height)
{
    struct x11_priv_s * priv = &x11_priv;

    priv->display = XOpenDisplay (NULL);
    if (! (priv->display)) {
	fprintf (stderr, "Can not open display\n");
	return 1;
    }

    if (xshm_check_extension ()) {
	fprintf (stderr, "No xshm extension\n");
	return 1;
    }

    return xv_common_setup (width, height, xvshm_alloc_frame);
}

static int xvshm_close (void)
{
    struct x11_priv_s * priv = &x11_priv;

    libvo_common_free_frames (xvshm_free_frame);
    XvUngrabPort (priv->display, priv->port, 0);
    common_close ();
    return 0;
}

static void xvshm_draw_frame (frame_t * frame)
{
    struct x11_priv_s * priv = &x11_priv;

    XvShmPutImage (priv->display, priv->port, priv->window, priv->gc,
		   frame->private, 0, 0, priv->width, priv->height,
		   0, 0, priv->width, priv->height, False);
    XSync (priv->display, False);
}

vo_output_video_t video_out_xvshm = {
    "xvshm",
    xvshm_setup, xvshm_close, libvo_common_get_frame, xvshm_draw_frame
};
#endif

#endif
