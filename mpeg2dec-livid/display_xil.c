/* 
 * display_xil.c, Sun XIL / mediaLib interface
 *
 *
 * Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved.
 *
 * Adapted from display_x11.c (mpeg2dec) for XIL support by
 * Håkan Hjort <d95hjort@dtek.chalmers.se>
 */

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <string.h>
#include <errno.h>

#include "mpeg2.h"
#include "mpeg2_internal.h"
#include "yuv2rgb.h"

#include <xil/xil.h>


/* X11 related variables */
static Display *mydisplay;
static Window mywindow;
static GC mygc;
static int bpp;
static XWindowAttributes attribs;
static int X_already_started = 0;

/* XIL variables */
static XilSystemState xilstate;
static int bands;
static int resized;
static float horizontal_factor, vertical_factor;
static XilImage renderimage, copyimage, displayimage;



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

// Clamp to [0,255]
static uint_8 clip_tbl[1024]; /* clipping table */
static uint_8 *clip;

uint_32 image_width;
uint_32 image_height;
uint_32 progressive_sequence = 0;





/* connect to server, create and map window,
 * allocate colors and (shared) memory
 */
int display_init(uint_32 width, uint_32 height, int fullscreen, char *title)
{
   int screen;
   int i;
   char *hello = (title == NULL) ? "I love XIL" : title;
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

   if (X_already_started) {
     // ??? Add code to resize window....
     return;
   }
   
   printf( "x= %i y= %i\n", width, height);
  
   
   if(getenv("DISPLAY"))
      name = getenv("DISPLAY");
   mydisplay = XOpenDisplay(name);

   if (mydisplay == NULL)
   {
      fprintf(stderr,"Can not open display\n");
      return(0);
   }

   screen = DefaultScreen(mydisplay);

   hint.x = 0;
   hint.y = 10;
   hint.width = image_width;
   hint.height = image_height;
   hint.flags = PPosition | PSize;

   /* Make the window */
   
   if (XMatchVisualInfo(mydisplay, screen, 24, TrueColor, &vinfo) ) {
     bpp = 24;
     bands = 3;
   } else if (XMatchVisualInfo(mydisplay, screen, 8, PseudoColor, &vinfo) ) {
     bpp = 8;
     bands = 1;
   } else {
     fprintf(stderr, "Failed to find a suitable visual" );
     return(0);
   }
   printf("visual id is  %lx\n",vinfo.visualid);

   theCmap   = XCreateColormap(mydisplay, DefaultRootWindow(mydisplay), 
                               vinfo.visual, AllocNone);

   xswa.background_pixel = 0;
   xswa.border_pixel     = 1;
   xswa.colormap         = theCmap;
   xswamask = CWBackPixel | CWBorderPixel |CWColormap;


   mywindow = XCreateWindow(mydisplay, RootWindow(mydisplay,screen),
                            hint.x, hint.y, hint.width, hint.height,
			    4, bpp, CopyFromParent, vinfo.visual,
			    xswamask, &xswa);

   XSelectInput(mydisplay, mywindow, StructureNotifyMask);

   /* Tell other applications about this window */

   XSetStandardProperties(mydisplay, mywindow, hello, hello, None,
			  NULL, 0, &hint);
   

   /* Map window. */

   XMapWindow(mydisplay, mywindow);

   /* Wait for map. */
   do {
      XNextEvent(mydisplay, &xev);
   }
   while (xev.type != MapNotify || xev.xmap.event != mywindow);

   XSelectInput(mydisplay, mywindow, StructureNotifyMask);

   XFlush(mydisplay);
   XSync(mydisplay, False);
    
   mygc = XCreateGC(mydisplay, mywindow, 0L, &xgcv);
    
   // XIL sends an error message to stderr if xil_open fails 
   if ((xilstate = xil_open()) == NULL)
     return(0);

   // Install error handler
   //  if (xil_install_error_handler(State, error_handler) == XIL_FAILURE)
   //    error("unable to install error handler for XIL");

   // XIL sends error message to stderr if xil_create_from_window fail
   if (!(displayimage = xil_create_from_window(xilstate, mydisplay, mywindow)))
     return(0);

   xil_set_synchronize(displayimage, 1);
   
   renderimage = xil_create(xilstate, image_width, image_height, 4, XIL_BYTE);
   if (renderimage == NULL) {
     fprintf(stderr, "XIL error, failed to create image\n" );
   }
   copyimage = xil_create_child(renderimage, 0, 0, 
				image_width, image_height, 1, 3);
   
   //Humm... maybe we should do some more error checking.
   yuv2rgb_init( 32, MODE_BGR ); 
   X_already_started++;
   return(-1);  // non-zero == success.
}

void Terminate_Display_Process() {
  getchar();	/* wait for enter to remove window */
  if (displayimage) xil_destroy(displayimage);
  if (copyimage) xil_destroy(copyimage);
  if (renderimage) xil_destroy(renderimage);
  xil_close(xilstate);
  XDestroyWindow(mydisplay, mywindow);
  XCloseDisplay(mydisplay);
  X_already_started = 0;
}


void resize() {
  Window root;
  int x, y;
  unsigned int w, h, b, d;
  XGetGeometry(mydisplay, mywindow, &root, &x, &y, &w, &h, &b, &d);
  
  if( w == image_width && h == image_height ) {
    resized = 0;
    horizontal_factor = 1.0;
    vertical_factor = 1.0;
  } else {
    // indicate resize
    resized = 1;
    horizontal_factor = ((float) w)/image_width;
    vertical_factor = ((float) h)/image_height;
  }
  // to avoid new events for the time being
  XSelectInput(mydisplay, mywindow, NoEventMask);
  
  // Create new image with new geometry
  xil_destroy(displayimage);
  displayimage = xil_create_from_window(xilstate, mydisplay, mywindow);
  xil_set_synchronize(displayimage, 1);
  XSelectInput(mydisplay, mywindow, StructureNotifyMask);

}


void Display_First_Field(void) { /* nothing */ }
void Display_Second_Field(void) { /* nothing */ }


int display_frame(uint_8 *src[])
{
  XilMemoryStorage xilstorage;
  XEvent event;
  
  xil_export(renderimage);
  xil_get_memory_storage(renderimage, &xilstorage);
  yuv2rgb(xilstorage.byte.data, src[0], src[1], src[2],
	  image_width, image_height, 
	  xilstorage.byte.scanline_stride,
	  image_width, image_width/2);
  xil_import(renderimage, TRUE);
   
  if (resized){
    xil_scale(copyimage, displayimage, "bilinear", 
	      horizontal_factor, vertical_factor);
  }
  else {
    xil_copy(copyimage, displayimage);
  }
  
  if (XCheckWindowEvent(mydisplay, mywindow, StructureNotifyMask, &event))
    resize();

  return(-1);  // non-zero == success.
}
