// fix event handling. wait for unmap before close display.

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
#include <string.h>	// strerror
#include <errno.h>	// errno
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <inttypes.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <string.h>	// memcmp
#include <sys/socket.h>	// getsockname, getpeername
#include <netinet/in.h>	// struct sockaddr_in

#include "video_out.h"
#include "video_out_internal.h"
#include "yuv2rgb.h"


#ifdef LIBVO_XSHM
/* since it doesn't seem to be defined on some platforms */
int XShmGetEventBase (Display *);
#endif


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

    // XSHM
    XShmSegmentInfo shminfo; // num_buffers
} x11_priv;

static int x11_check_local (void)
{
    struct x11_priv_s * priv = &x11_priv;
    int fd;
    struct sockaddr_in me;
    struct sockaddr_in peer;
    int len;

    fd = ConnectionNumber (priv->display);

    len = sizeof (me);
    if (getsockname (fd, &me, &len))
	return 1;	// should not happen, assume remote display

    if (me.sin_family == PF_UNIX)
	return 0;	// display is local, using unix domain socket

    if (me.sin_family != PF_INET)
	return 1;	// unknown protocol, assume remote display

    len = sizeof (peer);
    if (getpeername (fd, &peer, &len))
	return 1;	// should not happen, assume remote display

    if (peer.sin_family != PF_INET)
	return 1;	// should not happen, assume remote display

    if (memcmp (&(me.sin_addr), &(peer.sin_addr), sizeof(me.sin_addr)))
	return 1;	// display is remote, using tcp/ip socket

    return 0;		// display is local, using tcp/ip socket
}

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

static int xshm_create_image (int width, int height)
{
    struct x11_priv_s * priv = &x11_priv;

    priv->ximage = XShmCreateImage (priv->display,
				    priv->vinfo.visual, priv->vinfo.depth,
				    ZPixmap, NULL /* data */,
				    &(priv->shminfo), width, height);
    if (priv->ximage == NULL)
	return 1;

    priv->shminfo.shmid =
	shmget (IPC_PRIVATE,
		priv->ximage->bytes_per_line * priv->ximage->height,
		IPC_CREAT | 0777);
    if (priv->shminfo.shmid == -1)
	return 1;

    priv->shminfo.shmaddr = shmat (priv->shminfo.shmid, 0, 0);
    if (priv->shminfo.shmaddr == (char *)-1)
	return 1;

    priv->shminfo.readOnly = True;
    if (! (XShmAttach (priv->display, &(priv->shminfo))))
	return 1;

    priv->ximage->data = priv->shminfo.shmaddr;
    return 0;
}

static void xshm_destroy_image (void)
{
    struct x11_priv_s * priv = &x11_priv;

    if (priv->ximage) {
	if (priv->shminfo.shmaddr != (char *)-1) {
	    XShmDetach (priv->display, &(priv->shminfo));
	    shmdt (priv->shminfo.shmaddr);
	}
	if (priv->shminfo.shmid != -1)
	    shmctl (priv->shminfo.shmid, IPC_RMID, 0);
	XDestroyImage (priv->ximage);
    }
}

static int x11_get_visual_info (void)
{
    struct x11_priv_s * priv = &x11_priv;
    XVisualInfo visualTemplate;
    XVisualInfo * XvisualInfoTable;
    XVisualInfo * XvisualInfo;
    int number;
    int i;

    // list truecolor visuals for the default screen
    visualTemplate.class = TrueColor;
    visualTemplate.screen = DefaultScreen (priv->display);
    XvisualInfoTable =
	XGetVisualInfo (priv->display,
			VisualScreenMask | VisualClassMask, &visualTemplate,
			&number);
    if (XvisualInfoTable == NULL)
	return 1;

    // find the visual with the highest depth
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

static int x11_yuv2rgb_init (void)
{
    struct x11_priv_s * priv = &x11_priv;
    int mode;

    // If we have blue in the lowest bit then "obviously" RGB
    // (walken : the guy who wrote this convention never heard of endianness ?)
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

static int x11_setup (vo_output_video_attr_t * vo_attr)
{
    struct x11_priv_s * priv = &x11_priv;
    int width, height;

    width = vo_attr->width;
    height = vo_attr->height;

    priv->display = XOpenDisplay (NULL);
    if (! (priv->display)) {
	fprintf (stderr, "Can not open display\n");
	return 1;
    }

    if (x11_check_local ()) {
	fprintf (stderr, "Can not use xshm on a remote display\n");
	return 1;
    }

    if (xshm_check_extension ()) {
	fprintf (stderr, "No xshm extension\n");
	return 1;
    }

    if (x11_get_visual_info ()) {
	fprintf (stderr, "No truecolor visual\n");
	return 1;
    }

    x11_create_window (width, height);
    x11_create_gc ();

    if (xshm_create_image (width, height)) {
	fprintf (stderr, "Cannot create shm image\n");
	return 1;
    }

    if (x11_yuv2rgb_init ()) {
	fprintf (stderr, "No support for non-native byte order\n");
	return 1;
    }

    // FIXME set WM_DELETE_WINDOW protocol ? to avoid shm leaks

    // should move this after everything than could potentially fail
    XMapWindow (priv->display, priv->window);

    priv->width = width;
    priv->height = height;
    priv->imagedata = (unsigned char *) priv->ximage->data;
    priv->stride = priv->ximage->bytes_per_line;

    return 0;
}

static int x11_close (void * dummy)
{
    struct x11_priv_s * priv = &x11_priv;

    xshm_destroy_image ();

    if (priv->window) {
	XFreeGC (priv->display, priv->gc);
	XDestroyWindow (priv->display, priv->window);
    }
    if (priv->display)
	XCloseDisplay (priv->display);

    return 0;
}

static void x11_flip_page (void)
{
    struct x11_priv_s * priv = &x11_priv;

    XShmPutImage (priv->display, priv->window, priv->gc, priv->ximage, 
		  0, 0, 0, 0, priv->width, priv->height, False);
    XFlush (priv->display);
}

static int x11_draw_slice (uint8_t *src[], int slice_num)
{
    struct x11_priv_s * priv = &x11_priv;

    yuv2rgb (priv->imagedata + priv->stride * 16 * slice_num,
	     src[0], src[1], src[2], priv->width, 16,
	     priv->stride, priv->width, priv->width >> 1);

    return 0;
}

static int x11_draw_frame (frame_t *frame)
{
    struct x11_priv_s * priv = &x11_priv;

    yuv2rgb (priv->imagedata, frame->base[0], frame->base[1], frame->base[2],
	     priv->width, priv->height,
	     priv->stride, priv->width, priv->width >> 1);

    return 0;
}

static frame_t * x11_allocate_image_buffer (int width, int height,
					    uint32_t format)
{
    return libvo_common_alloc (width, height);
}

void x11_free_image_buffer (frame_t* frame)
{
    libvo_common_free (frame);
}

LIBVO_EXTERN (x11, "x11")

#endif
