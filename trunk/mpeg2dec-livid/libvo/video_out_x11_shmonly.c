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

#include "video_out.h"
#include "video_out_internal.h"
#include "yuv2rgb.h"


#ifdef LIBVO_XSHM
/* since it doesn't seem to be defined on some platforms */
int XShmGetEventBase(Display*);
Bool XShmQueryExtension (Display *);
#endif


static struct x11_priv_s {
/* local data */
    unsigned char *ImageData;
    int image_width;
    int image_height;

/* X11 related variables */
    Display *display;
    Window window;
    GC gc;
    XVisualInfo vinfo;
    XImage *ximage;
    int bpp;

    // XSHM
    XShmSegmentInfo Shminfo; // num_buffers
} x11_priv;

static int _xshm_create (XShmSegmentInfo *Shminfo, int size)	// xxxxxxxxxx
{
    struct x11_priv_s *priv = &x11_priv;
	
    Shminfo->shmid = shmget (IPC_PRIVATE, size, IPC_CREAT | 0777);

    if (Shminfo->shmid < 0) {
	fprintf (stderr, "Shared memory error, disabling (seg id error: %s)\n", strerror (errno));
	return -1;
    }
	
    Shminfo->shmaddr = (char *) shmat(Shminfo->shmid, 0, 0);

    if (Shminfo->shmaddr == ((char *) -1)) {
	if (Shminfo->shmaddr != ((char *) -1))
	    shmdt(Shminfo->shmaddr);
	fprintf (stderr, "Shared memory error, disabling (address error)\n");
	return -1;
    }
		
    Shminfo->readOnly = False;
    XShmAttach(priv->display, Shminfo);

    return 0;
}

static void _xshm_destroy (XShmSegmentInfo *Shminfo)	// xxxxxxxxx
{
    struct x11_priv_s *priv = &x11_priv;
	
    XShmDetach (priv->display, Shminfo);
    shmdt (Shminfo->shmaddr);
    shmctl (Shminfo->shmid, IPC_RMID, 0);
}

static int x11_get_visual_info (void)
{
    struct x11_priv_s *priv = &x11_priv;
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
    struct x11_priv_s *priv = &x11_priv;
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
    struct x11_priv_s *priv = &x11_priv;
    XGCValues gcValues;

    priv->gc = XCreateGC (priv->display, priv->window, 0, &gcValues);
}

static int x11_setup (vo_output_video_attr_t * vo_attr)
{
    int width, height;
    int mode;
    struct x11_priv_s *priv = &x11_priv;

    width = vo_attr->width;
    height = vo_attr->height;

    priv->display = XOpenDisplay (NULL);
    if (! (priv->display)) {
	fprintf (stderr, "Can not open display\n");
	return 1;
    }

    if (x11_get_visual_info ()) {
	fprintf (stderr, "No truecolor visual\n");
	return 1;
    }

    x11_create_window (width, height);
    x11_create_gc ();

    // FIXME set WM_DELETE_WINDOW protocol ? to avoid shm leaks

    // should move this after everything than could potentially fail
    XMapWindow (priv->display, priv->window);

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

    if (XShmQueryExtension (priv->display)) {
	printf ("Using MIT Shared memory extension\n");
    } else {
	printf ("no shm\n");
	exit (1);
    }

    priv->image_width = width;
    priv->image_height = height;

    priv->ximage = XShmCreateImage (priv->display, priv->vinfo.visual, priv->vinfo.depth, ZPixmap, NULL, &priv->Shminfo, width, priv->image_height);

    // If no go, then revert to normal Xlib calls.
    if (!priv->ximage) {
	fprintf (stderr, "Shared memory error\n");
	exit (1);
    }

    if ((_xshm_create (&priv->Shminfo, priv->ximage->bytes_per_line * priv->ximage->height))) {
	XDestroyImage(priv->ximage);
	printf ("shm error\n");
	exit (1);
    }
	
    priv->ximage->data = priv->Shminfo.shmaddr;
    priv->ImageData = (unsigned char *) priv->ximage->data;

    priv->bpp = priv->ximage->bits_per_pixel;

    // If we have blue in the lowest bit then obviously RGB 
    mode = ((priv->ximage->blue_mask & 0x01)) ? MODE_RGB : MODE_BGR;

#ifdef WORDS_BIGENDIAN 
    if (priv->ximage->byte_order != MSBFirst) {
#else
    if (priv->ximage->byte_order != LSBFirst) {
#endif
	fprintf (stderr, "No support fon non-native XImage byte order\n");
	return -1;
    }

    yuv2rgb_init ((priv->vinfo.depth == 24) ? priv->bpp : priv->vinfo.depth, mode);
    return 0;
}

static int x11_close(void *plugin) 
{
    struct x11_priv_s *priv = &x11_priv;

    printf ("Closing video plugin\n");

    if (priv->Shminfo.shmaddr) {
	_xshm_destroy(&priv->Shminfo);
    }

    if (priv->ximage)
	XDestroyImage (priv->ximage);

    if (priv->window)
	XDestroyWindow (priv->display, priv->window);
    XCloseDisplay (priv->display);

    return 0;
}

static void x11_flip_page (void)
{
    struct x11_priv_s *priv = &x11_priv;

    XShmPutImage (priv->display, priv->window, priv->gc, priv->ximage, 
		  0, 0, 0, 0, priv->ximage->width, priv->ximage->height, False); 
    XFlush (priv->display);
}

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
    return libvo_common_alloc (width, height);
}

void x11_free_image_buffer (frame_t* frame)
{
    libvo_common_free (frame);
}

LIBVO_EXTERN (x11,"x11")

#endif
