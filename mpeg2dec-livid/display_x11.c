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
 *
 * Xv image suuport by Gerd Knorr <kraxel@goldbach.in-berlin.de>
 */

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <string.h>
#include <errno.h>

#include <config.h>
#include "libmpeg2/mpeg2.h"
#include "libmpeg2/mpeg2_internal.h"
#include "yuv2rgb.h"


/* private prototypes */
static void Display_Image (XImage * myximage, unsigned char *ImageData);

/* local data */
static unsigned char *ImageData;

/* X11 related variables */
static Display *mydisplay;
static Window mywindow;
static GC mygc;
static XImage *myximage;
static int bpp, mode;
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


uint_32 image_width;
uint_32 image_height;
uint_32 progressive_sequence = 0;

/* connect to server, create and map window,
 * allocate colors and (shared) memory
 */
void display_init(uint_32 width, uint_32 height)
{
   int screen;
   unsigned int fg, bg;
   char *hello = "I hate X11";
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
    
#ifdef HAVE_XV
   xv_port = 0;
   if (Success == XvQueryExtension(mydisplay,&ver,&rel,&req,&ev,&err)) {
       /* check for Xvideo support */
       if (Success != XvQueryAdaptors(mydisplay,DefaultRootWindow(mydisplay),
				      &adaptors,&ai)) {
	   fprintf(stderr,"Xv: XvQueryAdaptors failed");
	   exit(1);
       }
       /* check adaptors */
       for (i = 0; i < adaptors; i++) {
	   if ((ai[i].type & XvInputMask) &&
	       (ai[i].type & XvImageMask) &&
	       (xv_port == 0)) {
	       xv_port = ai[i].base_id;
	   }
       }
       /* check image formats */
       if (xv_port != 0) {
	   fo = XvListImageFormats(mydisplay, xv_port, (int*)&formats);
	   for(i = 0; i < formats; i++) {
	       fprintf(stderr, "Xvideo image format: 0x%x (%4.4s) %s\n",
		       fo[i].id,
		       (char*)&fo[i].id,
		       (fo[i].format == XvPacked) ? "packed" : "planar");
	       if (0x32315659 == fo[i].id) {
		   xv_format = fo[i].id;
		   break;
	       }
	   }
	   if (i == formats)
	       /* no matching image format not */
	       xv_port = 0;
       }
       if (xv_port != 0) {
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
	   return;
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
       exit(1);
     }
   yuv2rgb_init(bpp, mode);
   
   X_already_started++;
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

void display_frame(uint_8 *src[])
{
#ifdef HAVE_XV
    Window root;
    XEvent event;
    int x, y;
    unsigned int w, h, b, d;
    
    if (xv_port != 0) {
	if (XCheckWindowEvent(mydisplay, mywindow,
			      StructureNotifyMask, &event)) {
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
	XvShmPutImage(mydisplay, xv_port, mywindow, mygc, xvimage1,
		      0, 0,  image_width, image_height,
		      0, 0,  win_width, win_height,
		      False);
	XFlush(mydisplay);
	return;
    }
#endif

  if (bpp==32) {
     yuv2rgb(ImageData, src[0], src[1], src[2],
	     image_width, image_height, 
	     image_width*4, image_width, image_width/2 );
   } else if (bpp == 24) {
     yuv2rgb(ImageData, src[0], src[1], src[2],
	     image_width, image_height, 
	     image_width*3, image_width, image_width/2 );
   } else if (bpp == 15 || bpp == 16) {
     yuv2rgb(ImageData, src[0], src[1], src[2],
	     image_width, image_height, 
	     image_width*2, image_width, image_width/2 );
   }
   
   Display_Image(myximage, ImageData);
}
