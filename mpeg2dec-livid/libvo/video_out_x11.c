#define DISP

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "video_out.h"
#include "video_out_internal.h"

LIBVO_EXTERN(x11)

#ifdef HAVE_X11

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <errno.h>
#include "yuv2rgb.h"

static vo_info_t vo_info = 
{
#ifdef HAVE_XV
	"X11 (Xv)",
#else
	"X11",
#endif
	"x11",
	"Aaron Holtzman <aholtzma@ess.engr.uvic.ca>",
	""
};

/* private prototypes */
static void Display_Image (XImage * myximage, unsigned char *ImageData);

/* since it doesn't seem to be defined on some platforms */
int XShmGetEventBase(Display*);

/* local data */
static unsigned char *ImageData;

/* X11 related variables */
static Display *mydisplay;
static Window mywindow;
static GC mygc;
static XImage *myximage;
static int depth, bpp, mode;
static XWindowAttributes attribs;
static int X_already_started = 0;


#ifdef HAVE_XV
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>
static unsigned int ver,rel,req,ev,err;
static unsigned int formats, adaptors,i,xv_port,xv_format;
static int win_width,win_height;
static XvAdaptorInfo        *ai;
static XvImageFormatValues  *fo;
static XvImage *xvimage1;
#endif

#define SH_MEM

#ifdef SH_MEM

#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

//static int HandleXError _ANSI_ARGS_((Display * dpy, XErrorEvent * event));
static void InstallXErrorHandler (void);
static void DeInstallXErrorHandler (void);

static int Shmem_Flag;
static int Quiet_Flag;
static XShmSegmentInfo Shminfo1;
static int gXErrorFlag;
static int CompletionType = -1;

static void InstallXErrorHandler()
{
	//XSetErrorHandler(HandleXError);
	XFlush(mydisplay);
}

static void DeInstallXErrorHandler()
{
	XSetErrorHandler(NULL);
	XFlush(mydisplay);
}

#endif


static uint32_t image_width;
static uint32_t image_height;

/* connect to server, create and map window,
 * allocate colors and (shared) memory
 */
static uint32_t 
init(uint32_t width, uint32_t height, uint32_t fullscreen, char *title)
{
	int screen;
	unsigned int fg, bg;
	char *hello = (title == NULL) ? "I hate X11" : title;
	char *name = ":0.0";
	XSizeHints hint;
	XVisualInfo vinfo;
	XEvent xev;

	XGCValues xgcv;
	Colormap theCmap;
	XSetWindowAttributes xswa;
	unsigned long xswamask;

	image_height = height;
	image_width = width;

	if (X_already_started)
		return -1;

	if(getenv("DISPLAY"))
		name = getenv("DISPLAY");

	mydisplay = XOpenDisplay(name);

	if (mydisplay == NULL)
	{
		fprintf(stderr,"Can not open display\n");
		return -1;
	}

	screen = DefaultScreen(mydisplay);

	hint.x = 0;
	hint.y = 0;
	hint.width = image_width;
	hint.height = image_height;
	hint.flags = PPosition | PSize;

	/* Get some colors */

	bg = WhitePixel(mydisplay, screen);
	fg = BlackPixel(mydisplay, screen);

	/* Make the window */

	XGetWindowAttributes(mydisplay, DefaultRootWindow(mydisplay), &attribs);

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

	depth = attribs.depth;

	if (depth != 15 && depth != 16 && depth != 24 && depth != 32) 
	{
		/* The root window may be 8bit but there might still be
		* visuals with other bit depths. For example this is the 
		* case on Sun/Solaris machines.
		*/
		depth = 24;
	}
	//BEGIN HACK
	//mywindow = XCreateSimpleWindow(mydisplay, DefaultRootWindow(mydisplay),
	//hint.x, hint.y, hint.width, hint.height, 4, fg, bg);
	//
	XMatchVisualInfo(mydisplay, screen, depth, TrueColor, &vinfo);

	theCmap   = XCreateColormap(mydisplay, RootWindow(mydisplay,screen), 
	vinfo.visual, AllocNone);

	xswa.background_pixel = 0;
	xswa.border_pixel     = 1;
	xswa.colormap         = theCmap;
	xswamask = CWBackPixel | CWBorderPixel |CWColormap;


	mywindow = XCreateWindow(mydisplay, RootWindow(mydisplay,screen),
	hint.x, hint.y, hint.width, hint.height, 4, depth,CopyFromParent,vinfo.visual,xswamask,&xswa);

	XSelectInput(mydisplay, mywindow, StructureNotifyMask);

	/* Tell other applications about this window */

	XSetStandardProperties(mydisplay, mywindow, hello, hello, None, NULL, 0, &hint);

	/* Map window. */

	XMapWindow(mydisplay, mywindow);

	/* Wait for map. */
	do 
	{
		XNextEvent(mydisplay, &xev);
	}
	while (xev.type != MapNotify || xev.xmap.event != mywindow);

	XSelectInput(mydisplay, mywindow, NoEventMask);

	XFlush(mydisplay);
	XSync(mydisplay, False);

	mygc = XCreateGC(mydisplay, mywindow, 0L, &xgcv);

#ifdef HAVE_XV
	xv_port = 0;
	if (Success == XvQueryExtension(mydisplay,&ver,&rel,&req,&ev,&err)) 
	{
		/* check for Xvideo support */
		if (Success != XvQueryAdaptors(mydisplay,DefaultRootWindow(mydisplay), &adaptors,&ai)) 
		{
			fprintf(stderr,"Xv: XvQueryAdaptors failed");
			return -1;
		}
		/* check adaptors */
		for (i = 0; i < adaptors; i++) 
		{
			if ((ai[i].type & XvInputMask) && (ai[i].type & XvImageMask) && (xv_port == 0)) 
				xv_port = ai[i].base_id;
		}
		/* check image formats */
		if (xv_port != 0) 
		{
			fo = XvListImageFormats(mydisplay, xv_port, (int*)&formats);

			for(i = 0; i < formats; i++) 
			{
				fprintf(stderr, "Xvideo image format: 0x%x (%4.4s) %s\n", fo[i].id, 
						(char*)&fo[i].id, (fo[i].format == XvPacked) ? "packed" : "planar");

				if (0x32315659 == fo[i].id) 
				{
					xv_format = fo[i].id;
					break;
				}
			}
			if (i == formats) /* no matching image format not */
				xv_port = 0;
		}

		if (xv_port != 0) 
		{
			fprintf(stderr,"using Xvideo port %d for hw scaling\n",
			xv_port);

			/* allocate XvImages.  FIXME: no error checking, without
			* mit-shm this will bomb... */
			xvimage1 = XvShmCreateImage(mydisplay, xv_port, xv_format, 0,
			image_width, image_height,
			&Shminfo1);
			Shminfo1.shmid    = shmget(IPC_PRIVATE, xvimage1->data_size,
			IPC_CREAT | 0777);
			Shminfo1.shmaddr  = (char *) shmat(Shminfo1.shmid, 0, 0);
			Shminfo1.readOnly = False;
			xvimage1->data = Shminfo1.shmaddr;
			XShmAttach(mydisplay, &Shminfo1);
			XSync(mydisplay, False);
			shmctl(Shminfo1.shmid, IPC_RMID, 0);

			/* so we can do grayscale while testing... */
			memset(xvimage1->data,128,xvimage1->data_size);

			/* catch window resizes */
			XSelectInput(mydisplay, mywindow, StructureNotifyMask);
			win_width  = image_width;
			win_height = image_height;

			/* all done (I hope...) */
			X_already_started++;
			return 0;
		}
	}
#endif

#ifdef SH_MEM
	if (XShmQueryExtension(mydisplay))
		Shmem_Flag = 1;
	else 
	{
		Shmem_Flag = 0;
		if (!Quiet_Flag)
			fprintf(stderr, "Shared memory not supported\nReverting to normal Xlib\n");
	}
	if (Shmem_Flag)
		CompletionType = XShmGetEventBase(mydisplay) + ShmCompletion;

	InstallXErrorHandler();

	if (Shmem_Flag) 
	{
		myximage = XShmCreateImage(mydisplay, vinfo.visual, 
		depth, ZPixmap, NULL, &Shminfo1, width, image_height);

		/* If no go, then revert to normal Xlib calls. */

		if (myximage == NULL ) 
		{
			if (myximage != NULL)
				XDestroyImage(myximage);
			if (!Quiet_Flag)
				fprintf(stderr, "Shared memory error, disabling (Ximage error)\n");

			goto shmemerror;
		}
		/* Success here, continue. */

		Shminfo1.shmid = shmget(IPC_PRIVATE, 
		myximage->bytes_per_line * myximage->height ,
		IPC_CREAT | 0777);
		if (Shminfo1.shmid < 0 ) 
		{
			XDestroyImage(myximage);
			if (!Quiet_Flag)
			{
				printf("%s\n",strerror(errno));
				perror(strerror(errno));
				fprintf(stderr, "Shared memory error, disabling (seg id error)\n");
			}
			goto shmemerror;
		}
		Shminfo1.shmaddr = (char *) shmat(Shminfo1.shmid, 0, 0);

		if (Shminfo1.shmaddr == ((char *) -1)) 
		{
			XDestroyImage(myximage);
			if (Shminfo1.shmaddr != ((char *) -1))
				shmdt(Shminfo1.shmaddr);
			if (!Quiet_Flag) 
				fprintf(stderr, "Shared memory error, disabling (address error)\n");
			goto shmemerror;
		}
		myximage->data = Shminfo1.shmaddr;
		ImageData = (unsigned char *) myximage->data;
		Shminfo1.readOnly = False;
		XShmAttach(mydisplay, &Shminfo1);

		XSync(mydisplay, False);

		if (gXErrorFlag) 
		{
			/* Ultimate failure here. */
			XDestroyImage(myximage);
			shmdt(Shminfo1.shmaddr);
			if (!Quiet_Flag)
				fprintf(stderr, "Shared memory error, disabling.\n");
			gXErrorFlag = 0;
			goto shmemerror;
		} 
		else 
		{
			shmctl(Shminfo1.shmid, IPC_RMID, 0);
		}

		if (!Quiet_Flag) 
		{
			fprintf(stderr, "Sharing memory.\n");
		}
	} 
	else 
	{
		shmemerror:
		Shmem_Flag = 0;
#endif
		myximage = XGetImage(mydisplay, mywindow, 0, 0,
		width, image_height, AllPlanes, ZPixmap);
		ImageData = myximage->data;
#ifdef SH_MEM
	}

	DeInstallXErrorHandler();
#endif

	bpp = myximage->bits_per_pixel;

	// If we have blue in the lowest bit then obviously RGB 
	mode = ((myximage->blue_mask & 0x01) != 0) ? MODE_RGB : MODE_BGR;
#ifdef WORDS_BIGENDIAN 
	if (myximage->byte_order != MSBFirst)
#else
	if (myximage->byte_order != LSBFirst) 
#endif
	{
		fprintf( stderr, "No support fon non-native XImage byte order!\n" );
		return -1;
	}

	/* 
	 * If depth is 24 then it may either be a 3 or 4 byte per pixel
	 * format. We can't use bpp because then we would lose the 
	 * distinction between 15/16bit depth (2 byte formate assumed).
	 *
	 * FIXME - change yuv2rgb_init to take both depth and bpp
	 * parameters
	 */
	yuv2rgb_init((depth == 24) ? bpp : depth, mode);

	X_already_started++;
	return 0;
}

static const vo_info_t*
get_info(void)
{
	return &vo_info;
}


static void 
Terminate_Display_Process(void) 
{
	getchar();	/* wait for enter to remove window */
#ifdef SH_MEM
	if (Shmem_Flag) 
	{
		XShmDetach(mydisplay, &Shminfo1);
		XDestroyImage(myximage);
		shmdt(Shminfo1.shmaddr);
	}
#endif
	XDestroyWindow(mydisplay, mywindow);
	XCloseDisplay(mydisplay);
	X_already_started = 0;
}

static void 
Display_Image(XImage *myximage, uint8_t *ImageData)
{
#ifdef DISP
#ifdef SH_MEM
	if (Shmem_Flag) 
	{
		XShmPutImage(mydisplay, mywindow, mygc, myximage, 
				0, 0, 0, 0, myximage->width, myximage->height, True); 
		XFlush(mydisplay);
	} 
	else
#endif
	{
		XPutImage(mydisplay, mywindow, mygc, myximage, 0, 0, 0, 0, 
				myximage->width, myximage->height);
	}
#endif
}

#ifdef HAVE_XV
static inline void
flip_page_xv(void)
{
	Window root;
	XEvent event;
	int x, y;
	unsigned int w, h, b, d;

	if (xv_port != 0) 
	{
		if (XCheckWindowEvent(mydisplay, mywindow, StructureNotifyMask, &event))
		{
			XGetGeometry(mydisplay, mywindow, &root, &x, &y, &w, &h, &b, &d);
			win_width  = w;
			win_height = h;
			fprintf(stderr,"win resize: %dx%d\n",win_width,win_height);                
		}

#ifdef DISP
		XvShmPutImage(mydisplay, xv_port, mywindow, mygc, xvimage1,
		0, 0,  image_width, image_height,
		0, 0,  win_width, win_height,
		False);
		XFlush(mydisplay);
#endif
		return;
	}
}
#endif

static inline void
flip_page_x11(void)
{
	Display_Image(myximage, ImageData);
}


static void
flip_page(void)
{
#if HAVE_XV
	if (xv_port != 0)
		return flip_page_xv();
	else
#endif
		return flip_page_x11();
}

#if HAVE_XV
static inline uint32_t
draw_slice_xv(uint8_t *src[], uint32_t slice_num)
{
	uint8_t *dst;

	dst = xvimage1->data + image_width * 16 * slice_num;

	memcpy(dst,src[0],image_width*16);
	dst = xvimage1->data + image_width * image_height + image_width * 4 * slice_num;
	memcpy(dst, src[2],image_width*4);
	dst = xvimage1->data + image_width * image_height * 5 / 4 + image_width * 4 * slice_num;
	memcpy(dst, src[1],image_width*4);

	return 0;  
}
#endif

static inline uint32_t
draw_slice_x11(uint8_t *src[], uint32_t slice_num)
{
	uint8_t *dst;

	dst = ImageData + image_width * 16 * (bpp/8) * slice_num;

	yuv2rgb(dst , src[0], src[1], src[2], 
			image_width, 16, 
			image_width*(bpp/8), image_width, image_width/2 );

	//Display_Image(myximage, ImageData);
	//getchar();
	return 0;
}

static uint32_t
draw_slice(uint8_t *src[], uint32_t slice_num)
{
#if HAVE_XV
	if (xv_port != 0)
		return draw_slice_xv(src,slice_num);
	else
#endif
		return draw_slice_x11(src,slice_num);
}

#ifdef HAVE_XV
static inline uint32_t 
draw_frame_xv(uint8_t *src[])
{
	Window root;
	XEvent event;
	int x, y;
	unsigned int w, h, b, d;
    
	//FIXME XV is borked wrt to slices
	printf("Xv support is broken\nFeel free to fix it -ah\n");
	exit(1);
	//FIXME XV is borked wrt to slices

	if (xv_port != 0) 
	{
		if (XCheckWindowEvent(mydisplay, mywindow, StructureNotifyMask, &event)) 
		{
			XGetGeometry(mydisplay, mywindow, &root, &x, &y, &w, &h, &b, &d);
			win_width  = w;
			win_height = h;
			fprintf(stderr,"win resize: %dx%d\n",win_width,win_height);
		}
		memcpy(xvimage1->data,src[0],image_width*image_height);
		memcpy(xvimage1->data+image_width*image_height,
		src[2],image_width*image_height/4);
		memcpy(xvimage1->data+image_width*image_height*5/4,
		src[1],image_width*image_height/4);
#ifdef DISP
		XvShmPutImage(mydisplay, xv_port, mywindow, mygc, xvimage1,
		0, 0,  image_width, image_height,
		0, 0,  win_width, win_height,
		False);
		XFlush(mydisplay);
#endif
		return 0;  
	}
}
#endif

static inline uint32_t 
draw_frame_x11(uint8_t *src[])
{
	yuv2rgb(ImageData, src[0], src[1], src[2],
		image_width, image_height, 
		image_width*(bpp/8), image_width, image_width/2 );

	Display_Image(myximage, ImageData);
	return 0; 
}

static uint32_t
draw_frame(uint8_t *src[])
{
#if HAVE_XV
	if (xv_port != 0)
		return draw_frame_xv(src);
	else
#endif
		return draw_frame_x11(src);
}

//FIXME this should allocate AGP memory via agpgart and then we
//can use AGP transfers to the framebuffer
static vo_image_buffer_t* 
allocate_image_buffer(uint32_t height, uint32_t width, uint32_t format)
{
	//use the generic fallback
	return allocate_image_buffer_common(height,width,format);
}

static void	
free_image_buffer(vo_image_buffer_t* image)
{
	//use the generic fallback
	free_image_buffer_common(image);
}

#else /* HAVE_X11 */

LIBVO_DUMMY_FUNCTIONS(x11);

#endif