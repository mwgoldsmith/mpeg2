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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <oms/oms.h>
#include <oms/plugin/output_video.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <errno.h>
#include "yuv2rgb.h"

#ifdef HAVE_XV
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>
#endif

static int _x11_open		(plugin_t *plugin, void *name);
static int _x11_close		(plugin_t *plugin);
static int _x11_setup		(plugin_output_video_attr_t *attr);
static int draw_frame		(uint8_t *src[]);
static int draw_slice		(uint8_t *src[], uint32_t slice_num);
static void flip_page		(void);
static void free_image_buffer	(vo_image_buffer_t* image);
static vo_image_buffer_t *allocate_image_buffer (uint32_t height, uint32_t width, uint32_t format);


static struct x11_priv_s {
/* local data */
	unsigned char *ImageData;
	uint32_t image_width;
	uint32_t image_height;

/* X11 related variables */
	Display *display;
	Window window;
	GC gc;
	XImage *ximage;
	int depth, bpp, mode;
	XWindowAttributes attribs;
	int X_already_started;

#ifdef HAVE_XV
	unsigned int ver,rel,req,ev,err;
	unsigned int formats, adaptors,xv_port,xv_format;
	int win_width,win_height;
	XvAdaptorInfo        *ai;
	XvImageFormatValues  *fo;
	XvImage *xvimage[1];
#endif

} _x11_priv;

static plugin_output_video_t video_x11 = {
        priv:		&_x11_priv,
        open:		_x11_open,
        close:		_x11_close,
        setup:		_x11_setup,
	draw_frame:	draw_frame,
	draw_slice:	draw_slice,
        flip_page:	flip_page,
        allocate_image_buffer:	allocate_image_buffer,
        free_image_buffer:	free_image_buffer
};

/* private prototypes */
static void Display_Image (XImage * ximage, uint8_t *ImageData);

/* since it doesn't seem to be defined on some platforms */
int XShmGetEventBase(Display*);

#define SH_MEM

#ifdef SH_MEM

#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

//static int HandleXError _ANSI_ARGS_((Display * dpy, XErrorEvent * event));
static void InstallXErrorHandler (void);
static void DeInstallXErrorHandler (void);

static int Shmem_Flag;
static XShmSegmentInfo Shminfo1;
static int gXErrorFlag;
static int CompletionType = -1;

static void InstallXErrorHandler()
{
	//XSetErrorHandler(HandleXError);
	XFlush(_x11_priv.display);
}

static void DeInstallXErrorHandler()
{
	XSetErrorHandler(NULL);
	XFlush(_x11_priv.display);
}

#endif


static int _x11_open (plugin_t *plugin, void *name)
{
	LOG (LOG_DEBUG, "Open Called\n");
	return 0;
}

/**
 * connect to server, create and map window,
 * allocate colors and (shared) memory
 **/

static int _x11_setup (plugin_output_video_attr_t *attr)
{
	int screen;
	unsigned int fg, bg;
	char *name = ":0.0";
	XSizeHints hint;
	XVisualInfo vinfo;
	XEvent xev;

	XGCValues xgcv;
	Colormap theCmap;
	XSetWindowAttributes xswa;
	unsigned long xswamask;

	_x11_priv.image_width = attr->width;
	_x11_priv.image_height = attr->height;

	if (_x11_priv.X_already_started)
		return -1;

	if (getenv ("DISPLAY"))
		name = getenv ("DISPLAY");

	_x11_priv.display = XOpenDisplay (name);

	if (!_x11_priv.display) {
		LOG (LOG_ERROR, "Can not open display\n");
		return -1;
	}

	screen = DefaultScreen(_x11_priv.display);

	hint.x = 0;
	hint.y = 0;
	hint.width = _x11_priv.image_width;
	hint.height = _x11_priv.image_height;
	hint.flags = PPosition | PSize;

	/* Get some colors */

	bg = WhitePixel(_x11_priv.display, screen);
	fg = BlackPixel(_x11_priv.display, screen);

	/* Make the window */

	XGetWindowAttributes(_x11_priv.display, DefaultRootWindow(_x11_priv.display), &_x11_priv.attribs);

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

	_x11_priv.depth = _x11_priv.attribs.depth;

	if (_x11_priv.depth != 15 && _x11_priv.depth != 16 && _x11_priv.depth != 24 && _x11_priv.depth != 32) {
		/* The root window may be 8bit but there might still be
		 * visuals with other bit depths. For example this is the
		 * case on Sun/Solaris machines.
		 */
		_x11_priv.depth = 24;
	}
	//BEGIN HACK
	//_x11_priv.window = XCreateSimpleWindow(_x11_priv.display, DefaultRootWindow(_x11_priv.display),
	//hint.x, hint.y, hint.width, hint.height, 4, fg, bg);
	//
	XMatchVisualInfo(_x11_priv.display, screen, _x11_priv.depth, TrueColor, &vinfo);

	theCmap   = XCreateColormap(_x11_priv.display, RootWindow(_x11_priv.display,screen), 
	vinfo.visual, AllocNone);

	xswa.background_pixel = 0;
	xswa.border_pixel     = 1;
	xswa.colormap         = theCmap;
	xswamask = CWBackPixel | CWBorderPixel |CWColormap;

	_x11_priv.window = XCreateWindow(_x11_priv.display, RootWindow(_x11_priv.display,screen),
	hint.x, hint.y, hint.width, hint.height, 4, _x11_priv.depth, CopyFromParent,vinfo.visual,xswamask,&xswa);

	XSelectInput (_x11_priv.display, _x11_priv.window, StructureNotifyMask);

// Tell other applications about this window
	if (attr->title)
		XSetStandardProperties(_x11_priv.display, _x11_priv.window, attr->title, attr->title, None, NULL, 0, &hint);
	else
		XSetStandardProperties(_x11_priv.display, _x11_priv.window, "X11", "X11", None, NULL, 0, &hint);

// Map window
	XMapWindow(_x11_priv.display, _x11_priv.window);

// Wait for map
	do {
		XNextEvent(_x11_priv.display, &xev);
	} while (xev.type != MapNotify || xev.xmap.event != _x11_priv.window);

	XSelectInput(_x11_priv.display, _x11_priv.window, NoEventMask);

	XFlush(_x11_priv.display);
	XSync(_x11_priv.display, False);

	_x11_priv.gc = XCreateGC(_x11_priv.display, _x11_priv.window, 0L, &xgcv);

#ifdef HAVE_XV
	_x11_priv.xv_port = 0;
	if (Success == XvQueryExtension(_x11_priv.display,&_x11_priv.ver,&_x11_priv.rel,&_x11_priv.req,&_x11_priv.ev,&_x11_priv.err)) {
		int i;

		/* check for Xvideo support */
		if (Success != XvQueryAdaptors(_x11_priv.display,DefaultRootWindow(_x11_priv.display), &_x11_priv.adaptors,&_x11_priv.ai)) {
			LOG (LOG_ERROR, "Xv: XvQueryAdaptors failed");
			return -1;
		}
		/* check adaptors */
		for (i=0; i < _x11_priv.adaptors; i++) {
			if ((_x11_priv.ai[i].type & XvInputMask) && (_x11_priv.ai[i].type & XvImageMask) && !_x11_priv.xv_port) 
				_x11_priv.xv_port = _x11_priv.ai[i].base_id;
		}
		/* check image formats */
		if (_x11_priv.xv_port) {
			_x11_priv.fo = XvListImageFormats(_x11_priv.display, _x11_priv.xv_port, (int*)&_x11_priv.formats);

			for (i=0; i < _x11_priv.formats; i++) {
				LOG (LOG_ERROR, "Xvideo image format: 0x%x (%4.4s) %s", _x11_priv.fo[i].id, 
						(char*)&_x11_priv.fo[i].id, (_x11_priv.fo[i].format == XvPacked) ? "packed" : "planar");

				if (0x32315659 == _x11_priv.fo[i].id) {
					_x11_priv.xv_format = _x11_priv.fo[i].id;
					break;
				}
			}
			if (i == _x11_priv.formats) /* no matching image format not */
				_x11_priv.xv_port = 0;
		}

		if (_x11_priv.xv_port) {
			LOG (LOG_INFO, "using Xvideo port %d for hw scaling\n", _x11_priv.xv_port);

/* allocate XvImages.  FIXME: no error checking, without mit-shm this will bomb... */
			_x11_priv.xvimage[0] = XvShmCreateImage (_x11_priv.display, _x11_priv.xv_port, _x11_priv.xv_format, 0,
				_x11_priv.image_width, _x11_priv.image_height, &Shminfo1);

			Shminfo1.shmid    = shmget(IPC_PRIVATE, _x11_priv.xvimage[0]->data_size,
			IPC_CREAT | 0777);
			Shminfo1.shmaddr  = (char *) shmat(Shminfo1.shmid, 0, 0);
			Shminfo1.readOnly = False;
			_x11_priv.xvimage[0]->data = Shminfo1.shmaddr;
			XShmAttach (_x11_priv.display, &Shminfo1);
			XSync (_x11_priv.display, False);
			shmctl (Shminfo1.shmid, IPC_RMID, 0);

			/* so we can do grayscale while testing... */
			memset (_x11_priv.xvimage[0]->data, 128, _x11_priv.xvimage[0]->data_size);

			/* catch window resizes */
			XSelectInput (_x11_priv.display, _x11_priv.window, StructureNotifyMask);
			_x11_priv.win_width  = _x11_priv.image_width;
			_x11_priv.win_height = _x11_priv.image_height;

			/* all done (I hope...) */
			_x11_priv.X_already_started++;
			return 0;
		}
	}
#endif

#ifdef SH_MEM
	if (XShmQueryExtension(_x11_priv.display))
		Shmem_Flag = 1;
	else {
		Shmem_Flag = 0;
		LOG (LOG_INFO, "Shared memory not supported - Reverting to normal Xlib");
	}

	if (Shmem_Flag)
		CompletionType = XShmGetEventBase(_x11_priv.display) + ShmCompletion;

	InstallXErrorHandler();

	if (Shmem_Flag) 
	{
		_x11_priv.ximage = XShmCreateImage(_x11_priv.display, vinfo.visual, 
		_x11_priv.depth, ZPixmap, NULL, &Shminfo1, attr->width, _x11_priv.image_height);

		/* If no go, then revert to normal Xlib calls. */

		if (!_x11_priv.ximage) {
			if (_x11_priv.ximage)
				XDestroyImage(_x11_priv.ximage);
			LOG (LOG_ERROR, "Shared memory error, disabling (Ximage error)");

			goto shmemerror;
		}
		/* Success here, continue. */

		Shminfo1.shmid = shmget(IPC_PRIVATE, 
		_x11_priv.ximage->bytes_per_line * _x11_priv.ximage->height ,
		IPC_CREAT | 0777);
		if (Shminfo1.shmid < 0 ) {
			XDestroyImage(_x11_priv.ximage);

			LOG (LOG_ERROR, "Shared memory error, disabling (seg id error: %s)", strerror (errno));
			goto shmemerror;
		}
		Shminfo1.shmaddr = (char *) shmat(Shminfo1.shmid, 0, 0);

		if (Shminfo1.shmaddr == ((char *) -1)) {
			XDestroyImage(_x11_priv.ximage);
			if (Shminfo1.shmaddr != ((char *) -1))
				shmdt(Shminfo1.shmaddr);
			LOG (LOG_ERROR, "Shared memory error, disabling (address error)");
			goto shmemerror;
		}
		_x11_priv.ximage->data = Shminfo1.shmaddr;
		_x11_priv.ImageData = (unsigned char *) _x11_priv.ximage->data;
		Shminfo1.readOnly = False;
		XShmAttach(_x11_priv.display, &Shminfo1);

		XSync(_x11_priv.display, False);

		if (gXErrorFlag) {
			/* Ultimate failure here. */
			XDestroyImage(_x11_priv.ximage);
			shmdt(Shminfo1.shmaddr);
			LOG (LOG_ERROR, "Shared memory error, disabling.\n");
			gXErrorFlag = 0;
			goto shmemerror;
		} else 
			shmctl(Shminfo1.shmid, IPC_RMID, 0);

		LOG (LOG_DEBUG, "Sharing memory");
	} else {
		shmemerror:
		Shmem_Flag = 0;
#endif
		_x11_priv.ximage = XGetImage(_x11_priv.display, _x11_priv.window, 0, 0,
		attr->width, _x11_priv.image_height, AllPlanes, ZPixmap);
		_x11_priv.ImageData = _x11_priv.ximage->data;
#ifdef SH_MEM
	}

	DeInstallXErrorHandler();
#endif

	_x11_priv.bpp = _x11_priv.ximage->bits_per_pixel;

	// If we have blue in the lowest bit then obviously RGB 
	_x11_priv.mode = ((_x11_priv.ximage->blue_mask & 0x01) != 0) ? MODE_RGB : MODE_BGR;
#ifdef WORDS_BIGENDIAN 
	if (_x11_priv.ximage->byte_order != MSBFirst)
#else
	if (_x11_priv.ximage->byte_order != LSBFirst) 
#endif
	{
		LOG (LOG_ERROR, "No support fon non-native XImage byte order");
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
	yuv2rgb_init((_x11_priv.depth == 24) ? _x11_priv.bpp : _x11_priv.depth, _x11_priv.mode);

	_x11_priv.X_already_started++;
	return 0;
}


static int
_x11_close(plugin_t *plugin) 
{
	getchar();	/* wait for enter to remove window */
#ifdef SH_MEM
	if (Shmem_Flag) 
	{
		XShmDetach(_x11_priv.display, &Shminfo1);
		XDestroyImage(_x11_priv.ximage);
		shmdt(Shminfo1.shmaddr);
	}
#endif
	XDestroyWindow(_x11_priv.display, _x11_priv.window);
	XCloseDisplay(_x11_priv.display);
	_x11_priv.X_already_started = 0;

	return 0;
}

static void 
Display_Image(XImage *ximage, uint8_t *ImageData)
{
#ifdef SH_MEM
	if (Shmem_Flag) 
	{
		XShmPutImage(_x11_priv.display, _x11_priv.window, _x11_priv.gc, ximage, 
				0, 0, 0, 0, ximage->width, ximage->height, True); 
		XFlush(_x11_priv.display);
	} 
	else
#endif
	{
		XPutImage(_x11_priv.display, _x11_priv.window, _x11_priv.gc, ximage, 0, 0, 0, 0, 
				ximage->width, ximage->height);
	}
}

#ifdef HAVE_XV
static inline void flip_page_xv (void)
{
	Window root;
	XEvent event;
	int x, y;
	unsigned int w, h, b, d;

	if (_x11_priv.xv_port) {
		if (XCheckWindowEvent(_x11_priv.display, _x11_priv.window, StructureNotifyMask, &event)) {
			XGetGeometry(_x11_priv.display, _x11_priv.window, &root, &x, &y, &w, &h, &b, &d);
			_x11_priv.win_width  = w;
			_x11_priv.win_height = h;
			LOG (LOG_DEBUG, "win resize: %dx%d",_x11_priv.win_width,_x11_priv.win_height);                
		}

		XvShmPutImage(_x11_priv.display, _x11_priv.xv_port, _x11_priv.window, _x11_priv.gc, _x11_priv.xvimage[0],
		0, 0,  _x11_priv.image_width, _x11_priv.image_height,
		0, 0,  _x11_priv.win_width, _x11_priv.win_height,
		False);
		XFlush(_x11_priv.display);
		return;
	}
}
#endif

static inline void flip_page_x11 (void)
{
	Display_Image (_x11_priv.ximage, _x11_priv.ImageData);
}


static void flip_page(void)
{
#ifdef HAVE_XV
	if (_x11_priv.xv_port)
		return flip_page_xv();
	else
#endif
		return flip_page_x11();
}

#ifdef HAVE_XV
static inline int
draw_slice_xv(uint8_t *src[], uint32_t slice_num)
{
	uint8_t *dst;

	dst = _x11_priv.xvimage[0]->data + _x11_priv.image_width * 16 * slice_num;

	memcpy (dst,src[0],_x11_priv.image_width*16);
	dst = _x11_priv.xvimage[0]->data + _x11_priv.image_width * _x11_priv.image_height + _x11_priv.image_width * 4 * slice_num;
	memcpy (dst, src[2],_x11_priv.image_width*4);
	dst = _x11_priv.xvimage[0]->data + _x11_priv.image_width * _x11_priv.image_height * 5 / 4 + _x11_priv.image_width * 4 * slice_num;
	memcpy (dst, src[1],_x11_priv.image_width*4);

	return 0;  
}
#endif

static inline int
draw_slice_x11(uint8_t *src[], uint32_t slice_num)
{
	uint8_t *dst;

	dst = _x11_priv.ImageData + _x11_priv.image_width * 16 * (_x11_priv.bpp/8) * slice_num;

	yuv2rgb(dst , src[0], src[1], src[2], 
			_x11_priv.image_width, 16, 
			_x11_priv.image_width*(_x11_priv.bpp/8), _x11_priv.image_width, _x11_priv.image_width/2 );

	//Display_Image(_x11_priv.ximage, _x11_priv.ImageData);
	//getchar();
	return 0;
}

static int
draw_slice(uint8_t *src[], uint32_t slice_num)
{
#ifdef HAVE_XV
	if (_x11_priv.xv_port)
		return draw_slice_xv(src,slice_num);
	else
#endif
		return draw_slice_x11(src,slice_num);
}

#ifdef HAVE_XV
static inline int
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

	if (_x11_priv.xv_port) {
		if (XCheckWindowEvent(_x11_priv.display, _x11_priv.window, StructureNotifyMask, &event)) {
			XGetGeometry(_x11_priv.display, _x11_priv.window, &root, &x, &y, &w, &h, &b, &d);
			_x11_priv.win_width  = w;
			_x11_priv.win_height = h;
			LOG (LOG_DEBUG, "win resize: %dx%d",_x11_priv.win_width,_x11_priv.win_height);
		}
		memcpy (_x11_priv.xvimage[0]->data,src[0],_x11_priv.image_width*_x11_priv.image_height);
		memcpy (_x11_priv.xvimage[0]->data+_x11_priv.image_width*_x11_priv.image_height,
			src[2],_x11_priv.image_width*_x11_priv.image_height/4);
		memcpy (_x11_priv.xvimage[0]->data+_x11_priv.image_width*_x11_priv.image_height*5/4,
			src[1],_x11_priv.image_width*_x11_priv.image_height/4);
		XvShmPutImage(_x11_priv.display, _x11_priv.xv_port, _x11_priv.window, _x11_priv.gc, _x11_priv.xvimage[0],
			0, 0,  _x11_priv.image_width, _x11_priv.image_height,
			0, 0,  _x11_priv.win_width, _x11_priv.win_height,
			False);
		XFlush (_x11_priv.display);
		return 0;  
	}
}
#endif

static inline int draw_frame_x11(uint8_t *src[])
{
	yuv2rgb(_x11_priv.ImageData, src[0], src[1], src[2],
		_x11_priv.image_width, _x11_priv.image_height, 
		_x11_priv.image_width*(_x11_priv.bpp/8), _x11_priv.image_width, _x11_priv.image_width/2 );

	Display_Image(_x11_priv.ximage, _x11_priv.ImageData);
	return 0; 
}

static int draw_frame(uint8_t *src[])
{
#ifdef HAVE_XV
	if (_x11_priv.xv_port)
		return draw_frame_xv(src);
	else
#endif
		return draw_frame_x11(src);
}

//FIXME this should allocate AGP memory via agpgart and then we
//can use AGP transfers to the framebuffer



static vo_image_buffer_t* allocate_image_buffer(uint32_t width, uint32_t height, uint32_t format)
{
        vo_image_buffer_t *image;
        uint32_t image_size;

        image = malloc(sizeof(vo_image_buffer_t));

        if(!image)
                return NULL;

        image->height = height;
        image->width = width;
        image->format = format;
        
        //we only know how to do 4:2:0 planar yuv right now.
        image_size = width * height * 3 / 2;
        image->base = malloc(image_size);

        if(!image->base) {
                free(image);
                return NULL;
	}

        return image;
}

void    
free_image_buffer(vo_image_buffer_t* image)
{
        free(image->base);
        free(image);
}

/**     
 * Initialize Plugin.
 **/    

int plugin_init (char *whoami)
{               
        pluginRegister (whoami,
                PLUGIN_ID_OUTPUT_VIDEO,
                "x11",
		NULL,
		NULL,
                &video_x11);

        return 0;
}  
   
   
/** 
 * Cleanup Plugin.
 **/
 
void plugin_exit (void)
{       
}
