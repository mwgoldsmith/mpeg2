
/* 
 * display_x11.c, X11 interface                                               
 *
 *
 * Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. 
 *
 * Hacked into mpeg2dec by
 * 
 * Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <string.h>
#include <errno.h>

#include "mpeg2_internal.h"

int Inverse_Table_6_9[8][4] =
{
  {117504, 138453, 13954, 34903}, /* no sequence_display_extension */
  {117504, 138453, 13954, 34903}, /* ITU-R Rec. 709 (1990) */
  {104597, 132201, 25675, 53279}, /* unspecified */
  {104597, 132201, 25675, 53279}, /* reserved */
  {104448, 132798, 24759, 53109}, /* FCC */
  {104597, 132201, 25675, 53279}, /* ITU-R Rec. 624-4 System B, G */
  {104597, 132201, 25675, 53279}, /* SMPTE 170M */
  {117579, 136230, 16907, 35559}  /* SMPTE 240M (1987) */
};


/* private prototypes */
static void Display_Image (XImage * myximage, unsigned char *ImageData);

/* local data */
static unsigned char *ImageData, *ImageData2;

/* X11 related variables */
static Display *mydisplay;
static Window mywindow;
static GC mygc;
static XImage *myximage, *myximage2;
static int bpp;
static int has32bpp = 0;
static XWindowAttributes attribs;
static int X_already_started = 0;


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
static XShmSegmentInfo Shminfo1, Shminfo2;
static int gXErrorFlag;
static int CompletionType = -1;

static int HandleXfprintf(stderr,Dpy, Event)
Display *Dpy;
XErrorEvent *Event;
{
    gXErrorFlag = 1;

    return 0;
}

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

/* Setup pseudocolor (grayscale) */
void set_colors()
{
	Colormap cmap;
	XColor mycolor[256];
	int i;
	if ((cmap = XCreateColormap(mydisplay, mywindow, attribs.visual,
		AllocAll)) == 0) {	/* allocate all colors */
		fprintf(stderr, "Can't get colors, using existing map\n");
		return;
	}
	for (i = 0; i < 256; i++) {
		mycolor[i].flags = DoRed | DoGreen | DoBlue;
		mycolor[i].pixel = i;
		mycolor[i].red = i << 8;
		mycolor[i].green = i << 8;
		mycolor[i].blue = i << 8;
	}
	XStoreColors(mydisplay, cmap, mycolor, 255);
	XSetWindowColormap(mydisplay, mywindow, cmap);
}

// Clamp to [0,255]
static uint_8 clip_tbl[1024]; /* clipping table */
static uint_8 *clip;

uint_32 image_width;
uint_32 image_height;
uint_32 progressive_sequence = 0;
uint_32 matrix_coefficients = 1;

/* connect to server, create and map window,
 * allocate colors and (shared) memory
 */
void display_init(uint_32 width, uint_32 height)
{
    int screen;
    unsigned int fg, bg;
		sint_32 i;
    char *hello = "I hate X11";
    char *name = ":0.0";
    XSizeHints hint;
    XVisualInfo vinfo;
    XEvent xev;

		XGCValues xgcv;
		Colormap theCmap;
    XSetWindowAttributes xswa;
    unsigned long xswamask;

		Shmem_Flag = 1;
		clip = clip_tbl + 384;

		for (i= -384; i< 640; i++)
			clip[i] = (i < 0) ? 0 : ((i > 255) ? 255 : i);
    	clip[i] = (i < 0) ? 0 : ((i > 255) ? 255 : i);
	
		image_height = height;
		image_width = width;

    if (X_already_started)
			return;

    mydisplay = XOpenDisplay(name);

    if (mydisplay == NULL)
			fprintf(stderr,"Can not open display\n");

    screen = DefaultScreen(mydisplay);

    hint.x = 200;
    hint.y = 200;
    hint.width = image_width;
    hint.height = image_height;
    hint.flags = PPosition | PSize;

    /* Get some colors */

    bg = WhitePixel(mydisplay, screen);
    fg = BlackPixel(mydisplay, screen);

    /* Make the window */

    XGetWindowAttributes(mydisplay, DefaultRootWindow(mydisplay), &attribs);
    bpp = attribs.depth;
    if (bpp != 8 && bpp != 15 && bpp != 16 && bpp != 24 && bpp != 32)
			fprintf(stderr,"Only 8,15,16,24, and 32bpp supported\n");
//BEGIN HACK
    //mywindow = XCreateSimpleWindow(mydisplay, DefaultRootWindow(mydisplay),
		     //hint.x, hint.y, hint.width, hint.height, 4, fg, bg);
				 //
		bpp = 24;
		XMatchVisualInfo(mydisplay,screen,24,TrueColor,&vinfo);
		printf("visual id is  %lx\n",vinfo.visualid);

		theCmap   = XCreateColormap(mydisplay, RootWindow(mydisplay,screen), 
				vinfo.visual, AllocNone);
		xswa.background_pixel = 0;
    xswa.border_pixel     = 1;
		xswa.colormap         = theCmap;
		xswamask = CWBackPixel | CWBorderPixel |CWColormap ;


    mywindow = XCreateWindow(mydisplay, RootWindow(mydisplay,screen),
		     hint.x, hint.y, hint.width, hint.height, 4, 24,CopyFromParent,vinfo.visual,xswamask,&xswa);

    XSelectInput(mydisplay, mywindow, StructureNotifyMask);

    /* Tell other applications about this window */

    XSetStandardProperties(mydisplay, mywindow, hello, hello, None, NULL, 0, &hint);

    /* Map window. */

    XMapWindow(mydisplay, mywindow);

    /* Wait for map. */
    do {
	XNextEvent(mydisplay, &xev);
    }
    while (xev.type != MapNotify || xev.xmap.event != mywindow);
    if (bpp == 8)
	set_colors();

    XSelectInput(mydisplay, mywindow, NoEventMask);

    XFlush(mydisplay);
    XSync(mydisplay, False);
    
    mygc = XCreateGC(mydisplay, mywindow, 0L, &xgcv);
    


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

    if (Shmem_Flag) {
			myximage = XShmCreateImage(mydisplay, vinfo.visual, bpp, 
					ZPixmap, NULL, &Shminfo1, width, 
					image_height);

		if (!progressive_sequence)
			myximage2 = XShmCreateImage(mydisplay, vinfo.visual, bpp, 
					ZPixmap, NULL, &Shminfo2, width, 
					image_height);

	/* If no go, then revert to normal Xlib calls. */

		if (myximage == NULL || (!progressive_sequence && myximage2 == NULL)) 
		{
	    if (myximage != NULL)
				XDestroyImage(myximage);
	    if (!progressive_sequence && myximage2 != NULL)
				XDestroyImage(myximage2);
	    if (!Quiet_Flag)
				fprintf(stderr, "Shared memory error, disabling (Ximage error)\n");
	    goto shmemerror;
		}
	/* Success here, continue. */

		Shminfo1.shmid = shmget(IPC_PRIVATE, 
				 myximage->bytes_per_line * myximage->height ,
					IPC_CREAT | 0777);
	if (!progressive_sequence)
	    Shminfo2.shmid = shmget(IPC_PRIVATE, 
		       myximage2->bytes_per_line * myximage2->height /2,
				    IPC_CREAT | 0777);

	if (Shminfo1.shmid < 0 || (!progressive_sequence && Shminfo2.shmid < 0)) {
	    XDestroyImage(myximage);
	    if (!progressive_sequence)
				XDestroyImage(myximage2);
	    if (!Quiet_Flag)
			{
				printf("%s\n",strerror(errno));
				perror(strerror(errno));
				fprintf(stderr, "Shared memory error, disabling (seg id error)\n");
			}
	    goto shmemerror;
	}
	Shminfo1.shmaddr = (char *) shmat(Shminfo1.shmid, 0, 0);
	Shminfo2.shmaddr = (char *) shmat(Shminfo2.shmid, 0, 0);

	if (Shminfo1.shmaddr == ((char *) -1) ||
	  (!progressive_sequence && Shminfo2.shmaddr == ((char *) -1))) {
	    XDestroyImage(myximage);
	    if (Shminfo1.shmaddr != ((char *) -1))
		shmdt(Shminfo1.shmaddr);
	    if (!progressive_sequence) {
		XDestroyImage(myximage2);
		if (Shminfo2.shmaddr != ((char *) -1))
		    shmdt(Shminfo2.shmaddr);
	    }
	    if (!Quiet_Flag) {
		fprintf(stderr, "Shared memory error, disabling (address error)\n");
	    }
	    goto shmemerror;
	}
	myximage->data = Shminfo1.shmaddr;
	ImageData = (unsigned char *) myximage->data;
	Shminfo1.readOnly = False;
	XShmAttach(mydisplay, &Shminfo1);
	if (!progressive_sequence) {
	    myximage2->data = Shminfo2.shmaddr;
	    ImageData2 = (unsigned char *) myximage2->data;
	    Shminfo2.readOnly = False;
	    XShmAttach(mydisplay, &Shminfo2);
	}
	XSync(mydisplay, False);

	if (gXErrorFlag) {
	    /* Ultimate failure here. */
	    XDestroyImage(myximage);
	    shmdt(Shminfo1.shmaddr);
	    if (!progressive_sequence) {
		XDestroyImage(myximage2);
		shmdt(Shminfo2.shmaddr);
	    }
	    if (!Quiet_Flag)
		fprintf(stderr, "Shared memory error, disabling.\n");
	    gXErrorFlag = 0;
	    goto shmemerror;
	} else {
	    shmctl(Shminfo1.shmid, IPC_RMID, 0);
	    if (!progressive_sequence)
		shmctl(Shminfo2.shmid, IPC_RMID, 0);
	}

	if (!Quiet_Flag) {
	    fprintf(stderr, "Sharing memory.\n");
	}
    } else {
      shmemerror:
	Shmem_Flag = 0;
#endif
	myximage = XGetImage(mydisplay, mywindow, 0, 0,
	    width, image_height, AllPlanes, ZPixmap);
	ImageData = myximage->data;

	if (!progressive_sequence) {
	    myximage2 = XGetImage(mydisplay, DefaultRootWindow(mydisplay), 0,
		0, width, image_height,
		AllPlanes, ZPixmap);
	    ImageData2 = myximage2->data;
	}
#ifdef SH_MEM
    }

    DeInstallXErrorHandler();
#endif
    has32bpp = (myximage->bits_per_pixel > 24) ? 1 : 0;
    X_already_started++;
}

void Terminate_Display_Process()
{
    getchar();	/* wait for enter to remove window */
#ifdef SH_MEM
    if (Shmem_Flag) {
	XShmDetach(mydisplay, &Shminfo1);
	XDestroyImage(myximage);
	shmdt(Shminfo1.shmaddr);
	if (!progressive_sequence) {
	    XShmDetach(mydisplay, &Shminfo2);
	    XDestroyImage(myximage2);
	    shmdt(Shminfo2.shmaddr);
	}
    }
#endif
    XDestroyWindow(mydisplay, mywindow);
    XCloseDisplay(mydisplay);
    X_already_started = 0;
}

static void Display_Image(myximage, ImageData)
XImage *myximage;
unsigned char *ImageData;
{
#ifdef SH_MEM
    if (Shmem_Flag) {
	XShmPutImage(mydisplay, mywindow, mygc, myximage,
		0, 0, 0, 0, myximage->width, myximage->height, True);
	XFlush(mydisplay);

#if 0
	//I don't know why this code is here, but it craashes
	//when I don't compile with -pg. Very odd.
	while (1) {
	    XEvent xev;

	    XNextEvent(mydisplay, &xev);
	    if (xev.type == CompletionType)
		break;
	}
#endif
    } else

#endif
	XPutImage(mydisplay, mywindow, mygc, myximage, 0, 0,
		0, 0, myximage->width, myximage->height);
}

void Display_First_Field(void) { /* nothing */ }
void Display_Second_Field(void) { /* nothing */ }

unsigned char *dst, *py, *pu, *pv;
static unsigned char *u444 = 0, *v444, *u422, *v422;
int x, y, Y, U, V, r, g, b, pixel;
int crv, cbu, cgu, cgv;


static void display_frame_32bpp_420(void);

void display_frame(uint_8 *src[])
{
	/* matrix coefficients */
	crv = Inverse_Table_6_9[matrix_coefficients][0];
	cbu = Inverse_Table_6_9[matrix_coefficients][1];
	cgu = Inverse_Table_6_9[matrix_coefficients][2];
	cgv = Inverse_Table_6_9[matrix_coefficients][3];
	dst = ImageData;

	py = src[0];
	pv = src[1];
	pu = src[2];

	display_frame_32bpp_420();
	Display_Image(myximage, ImageData);
}


static void display_frame_32bpp_420(void)
{
	uint_32 r,g,b;
	uint_32 Y,U,V;
	uint_32 g_common,b_common,r_common;
	uint_32 x,y;

	uint_32 *dst_line_1;
	uint_32 *dst_line_2;
	uint_8* py_line_1;
	uint_8* py_line_2;
	volatile char prefetch;


	dst_line_1 = dst_line_2 =  ImageData;
	dst_line_2 = dst_line_1 + image_width;

	py_line_1 = py;
	py_line_2 = py + image_width;

	for (y = 0; y < image_height / 2; y++) 
	{
		for (x = 0; x < image_width / 2; x++) 
		{

			//Common to all four pixels
			prefetch = pu[32];
			U = (*pu++) - 128;
			prefetch = pv[32];
			V = (*pv++) - 128;

			g_common = cgu * U + cgu * V - 32768;
			b_common = cbu * U + 32768;
			r_common = crv * V;

			//Pixel I
			prefetch = py_line_1[32];
			Y = 76309 * ((*py_line_1++) - 16);
			r = clip[(Y+r_common)>>16];
			g = clip[(Y-g_common)>>16];
			b = clip[(Y+b_common)>>16];

			pixel = (b<<16)|(g<<8)|r;
			*dst_line_1++ = pixel;
			
			//Pixel II
			Y = 76309 * ((*py_line_1++) - 16);

			r = clip[(Y+r_common)>>16];
			g = clip[(Y-g_common)>>16];
			b = clip[(Y+b_common)>>16];

			pixel = (b<<16)|(g<<8)|r;
			*dst_line_1++ = pixel;
			
			//Pixel III
			prefetch = py_line_2[32];
			Y = 76309 * ((*py_line_2++) - 16);

			r = clip[(Y+r_common)>>16];
			g = clip[(Y-g_common)>>16];
			b = clip[(Y+b_common)>>16];

			pixel = (b<<16)|(g<<8)|r;
			*dst_line_2++ = pixel;

			//Pixel IV
			Y = 76309 * ((*py_line_2++) - 16);

			r = clip[(Y+r_common)>>16];
			g = clip[(Y-g_common)>>16];
			b = clip[(Y+b_common)>>16];

			pixel = (b<<16)|(g<<8)|r;
			*dst_line_2++ = pixel;
		}

		py_line_1 += image_width;
		py_line_2 += image_width;
		dst_line_1 += image_width;
		dst_line_2 += image_width;
	}
}

#if 0
static void display_frame_32bpp_422(void)
{
	uint_32 tmp;
	uint_32 r,g,b;
	uint_32 Y,U,V;
	uint_32 x,y;

	//FIXME optimize similar to 420
	for (y = 0; y < image_height; y++) 
	{
		for (x = 0; x < image_width; x++) 
		{
			Y = 76309 * ((*py++) - 16);
			tmp = y * Chroma_Width + (x>>1);
			U = pu[tmp] - 128;
			V = pv[tmp] - 128;

			r = clip[(Y+crv*V)>>16];
			g = clip[(Y-cgu*U-cgv*V + 32768)>>16];
			b = clip[(Y+cbu*U + 32768)>>16];

			pixel = (r<<16)|(g<<8)|b;
			*(unsigned int *)dst = pixel;
			dst+=4;
		}
	}
}
void display_frame_32bpp_444(void)
{
	uint_32 r,g,b;
	uint_32 Y,U,V;
	uint_32 x,y;

	for (y = 0; y < image_height; y++) 
	{
		for (x = 0; x < image_width; x++) 
		{
			Y = 76309 * ((*py++) - 16);
			U = (*pu++) - 128;
			V = (*pv++) - 128;

			r = clip[(Y+crv*V)>>16];
			g = clip[(Y-cgu*U-cgv*V + 32768)>>16];
			b = clip[(Y+cbu*U + 32768)>>16];

			pixel = (r<<16)|(g<<8)|b;
			*(unsigned int *)dst = pixel;
			dst+=4;
		}
	}
}
#endif
