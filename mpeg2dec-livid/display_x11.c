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
 * 15 & 16 bpp support added by Franck Sicard <Franck.Sicard@solsoft.fr>
 */

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <string.h>
#include <errno.h>

#include "mpeg2.h"
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
static unsigned char *ImageData;

/* X11 related variables */
static Display *mydisplay;
static Window mywindow;
static GC mygc;
static XImage *myximage;
static int bpp;
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
static XShmSegmentInfo Shminfo1;
static int gXErrorFlag;
static int CompletionType = -1;

/*
static int HandleXfprintf(stderr,Dpy, Event)
Display *Dpy;
XErrorEvent *Event;
{
    gXErrorFlag = 1;

    return 0;
}
*/

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

   clip = clip_tbl + 384;

   for (i= -384; i< 640; i++)
      clip[i] = (i < 0) ? 0 : ((i > 255) ? 255 : i);
                
   image_height = height;
   image_width = width;

   if (X_already_started)
      return;

   if(getenv("DISPLAY"))
      name = getenv("DISPLAY");
   mydisplay = XOpenDisplay(name);

   if (mydisplay == NULL)
      fprintf(stderr,"Can not open display\n");

   screen = DefaultScreen(mydisplay);

   hint.x = 0;
   hint.y = 10;
   hint.width = image_width;
   hint.height = image_height;
   hint.flags = PPosition | PSize;

   /* Get some colors */

   bg = WhitePixel(mydisplay, screen);
   fg = BlackPixel(mydisplay, screen);

   /* Make the window */

   XGetWindowAttributes(mydisplay, DefaultRootWindow(mydisplay), &attribs);
   bpp = attribs.depth;
   if (bpp != 15 && bpp != 16 && bpp != 24 && bpp != 32) {
      fprintf(stderr,"Only 15,16,24, and 32bpp supported. Trying 24bpp!\n");
      bpp = 24;
   }
   //BEGIN HACK
   //mywindow = XCreateSimpleWindow(mydisplay, DefaultRootWindow(mydisplay),
   //hint.x, hint.y, hint.width, hint.height, 4, fg, bg);
   //
   XMatchVisualInfo(mydisplay,screen,bpp,TrueColor,&vinfo);
   printf("visual id is  %lx\n",vinfo.visualid);

   theCmap   = XCreateColormap(mydisplay, RootWindow(mydisplay,screen), 
                               vinfo.visual, AllocNone);

   xswa.background_pixel = 0;
   xswa.border_pixel     = 1;
   xswa.colormap         = theCmap;
   xswamask = CWBackPixel | CWBorderPixel |CWColormap;


   mywindow = XCreateWindow(mydisplay, RootWindow(mydisplay,screen),
                            hint.x, hint.y, hint.width, hint.height, 4, bpp,CopyFromParent,vinfo.visual,xswamask,&xswa);

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
      if (Shminfo1.shmid < 0 ) {
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
      
      if (Shminfo1.shmaddr == ((char *) -1)) {
         XDestroyImage(myximage);
         if (Shminfo1.shmaddr != ((char *) -1))
            shmdt(Shminfo1.shmaddr);
         if (!Quiet_Flag) {
            fprintf(stderr, "Shared memory error, disabling (address error)\n");
         }
         goto shmemerror;
      }
      myximage->data = Shminfo1.shmaddr;
      ImageData = (unsigned char *) myximage->data;
      Shminfo1.readOnly = False;
      XShmAttach(mydisplay, &Shminfo1);
      
      XSync(mydisplay, False);

      if (gXErrorFlag) {
         /* Ultimate failure here. */
         XDestroyImage(myximage);
         shmdt(Shminfo1.shmaddr);
         if (!Quiet_Flag)
            fprintf(stderr, "Shared memory error, disabling.\n");
         gXErrorFlag = 0;
         goto shmemerror;
      } else {
         shmctl(Shminfo1.shmid, IPC_RMID, 0);
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

#ifdef SH_MEM
   }

   DeInstallXErrorHandler();
#endif
   X_already_started++;
	 bpp = myximage->bits_per_pixel;
}

void Terminate_Display_Process() {
   getchar();	/* wait for enter to remove window */
#ifdef SH_MEM
    if (Shmem_Flag) {
	XShmDetach(mydisplay, &Shminfo1);
	XDestroyImage(myximage);
	shmdt(Shminfo1.shmaddr);
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

static void display_frame_32bpp_420(const uint_8 * py,
                                    const uint_8 * pv,
                                    const uint_8 * pu,
                                    uint_8 * image,
                                    int h_size,
                                    int v_size,
                                    int bpp);
static void display_frame_16bpp_420(const uint_8 * py,
                                    const uint_8 * pv,
                                    const uint_8 * pu,
                                    uint_8 * image,
                                    int h_size,
                                    int v_size,
                                    int bpp);
void display_frame(uint_8 *src[])
{
   if (bpp==32 || bpp==24) {
      display_frame_32bpp_420(src[0],src[1],src[2],ImageData,
                              image_width, image_height,
                              bpp);
   } else if (bpp == 15 || bpp == 16) {
      display_frame_16bpp_420(src[0],src[1],src[2],ImageData,
                              image_width, image_height,
                              bpp);
   }
   Display_Image(myximage, ImageData);
}

/* do 32 and 24 bpp output */
static void display_frame_32bpp_420(const uint_8 * py, const uint_8 * pv, 
		const uint_8 * pu, uint_8 * image, int h_size, int v_size, int bpp)
{
	sint_32 Y,U,V;
	sint_32 g_common,b_common,r_common;
	uint_32 x,y;

	uint_8 *dst_line_1;
	uint_8 *dst_line_2;
	const uint_8* py_line_1;
	const uint_8* py_line_2;
	volatile char prefetch;

	int byte_per_line=h_size*4;

	int crv,cbu,cgu,cgv;

	/* matrix coefficients */
	crv = Inverse_Table_6_9[matrix_coefficients][0];
	cbu = Inverse_Table_6_9[matrix_coefficients][1];
	cgu = Inverse_Table_6_9[matrix_coefficients][2];
	cgv = Inverse_Table_6_9[matrix_coefficients][3];


	dst_line_1 = dst_line_2 =  image;
	dst_line_2 = dst_line_1 + byte_per_line;

	py_line_1 = py;
	py_line_2 = py + h_size;

	for (y = 0; y < v_size / 2; y++) 
	{
		for (x = 0; x < h_size / 2; x++) 
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
			*dst_line_1++ = clip[(Y+r_common)>>16];
			*dst_line_1++ = clip[(Y-g_common)>>16];
			*dst_line_1++ = clip[(Y+b_common)>>16];
			dst_line_1++;

			//Pixel II
			Y = 76309 * ((*py_line_1++) - 16);

			*dst_line_1++ = clip[(Y+r_common)>>16];
			*dst_line_1++ = clip[(Y-g_common)>>16];
			*dst_line_1++ = clip[(Y+b_common)>>16];
			dst_line_1++;

			//Pixel III
			prefetch = py_line_2[32];
			Y = 76309 * ((*py_line_2++) - 16);

			*dst_line_2++ = clip[(Y+r_common)>>16];
			*dst_line_2++ = clip[(Y-g_common)>>16];
			*dst_line_2++ = clip[(Y+b_common)>>16];
			dst_line_2++;

			//Pixel IV
			Y = 76309 * ((*py_line_2++) - 16);

			*dst_line_2++ = clip[(Y+r_common)>>16];
			*dst_line_2++ = clip[(Y-g_common)>>16];
			*dst_line_2++ = clip[(Y+b_common)>>16];
			dst_line_2++;
		}

		py_line_1 += h_size;
		py_line_2 += h_size;
		dst_line_1 += byte_per_line;
		dst_line_2 += byte_per_line;
	}
}

/* do 16 and 15 bpp output */
static void display_frame_16bpp_420(const uint_8 * py, const uint_8 * pv, 
		const uint_8 * pu, uint_8 * image, int h_size, int v_size, int bpp)
{
	uint_32 U,V;
	uint_32 pixel_idx;
	uint_32 x,y;

	uint_16 *dst_line_1;
	uint_16 *dst_line_2;
	const uint_8* py_line_1;
	const uint_8* py_line_2;
	uint_8  r,v,b;

	static uint_16 * lookUpTable=NULL;

	int i,j,k;

	//not sure if this is a win using the LUT. Can someone try
	//the direct calculation (like in the 32bpp) and compare?
	if (lookUpTable==NULL) 
	{
		lookUpTable=malloc((1<<18)*sizeof(uint_16));

		for (i=0;i<(1<<6);++i) 
		{ /* Y */
			int Y=i<<2;

			for(j=0;j<(1<<6);++j) 
			{ /* Cr */
				int Cb=j<<2;

				for(k=0;k<(1<<6);k++) 
				{ /* Cb */
					int Cr=k<<2;

					/*
					R = clp[(int)(*Y + 1.371*(*Cr-128))];  
					V = clp[(int)(*Y - 0.698*(*Cr-128) - 0.336*(*Cr-128))]; 
					B = clp[(int)(*Y++ + 1.732*(*Cb-128))];
					*/
					r=clip[(Y*1000 + 1371*(Cr-128))/1000] >>3;
					v=clip[(Y*1000 - 698*(Cr-128) - 336*(Cr-128))/1000] >> (bpp==16?2:3);
					b=clip[(Y*1000 + 1732*(Cb-128))/1000] >> 3;
					lookUpTable[i|(j<<6)|(k<<12)] = r | (v << 5) | (b <<  (bpp==16?11:10));
				}
			}
		}
	}

	dst_line_1 = dst_line_2 =  (uint_16*) image;
	dst_line_2 = dst_line_1 + h_size;

	py_line_1 = py;
	py_line_2 = py + h_size;

	for (y = 0; y < v_size / 2; y++) 
	{
		for (x = 0; x < h_size / 2; x++) 
		{
			//Common to all four pixels
			U = (*pu++)>>2;
			V = (*pv++)>>2;
			pixel_idx=U<<6|V<<12;

			//Pixel I
			*dst_line_1++=lookUpTable[(*py_line_1++)>>2|pixel_idx];

			//Pixel II
			*dst_line_1++=lookUpTable[(*py_line_1++)>>2|pixel_idx];

			//Pixel III
			*dst_line_2++=lookUpTable[(*py_line_2++)>>2|pixel_idx];

			//Pixel IV
			*dst_line_2++=lookUpTable[(*py_line_2++)>>2|pixel_idx];
		}
		py_line_1 += h_size;
		py_line_2 += h_size;
		dst_line_1 += h_size;
		dst_line_2 += h_size;
	}
}


