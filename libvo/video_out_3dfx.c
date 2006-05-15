//PLUGIN_INFO(INFO_NAME, "3DFX (/dev/3dfx)");
//PLUGIN_INFO(INFO_AUTHOR, "Colin Cross <colin@mit.edu>");

/* 
 *    video_out_3dfx.c
 *
 *	Copyright (C) Colin Cross Apr 2000
 *
 *  This file heavily based off of video_out_mga.c of Aaron Holtzman's
 *  mpeg2dec
 *	
 *  mpeg2dec is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  mpeg2dec is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <config.h>

#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <wchar.h>
#include <signal.h>
#include <inttypes.h>
//#include <linux/types.h>

#include <X11/Xlib.h>
#include <X11/extensions/xf86dga.h>
#include <X11/Xutil.h>

#include <oms/oms.h>
#include <oms/log.h>
#include <oms/plugin/output_video.h>

#include "3dfx.h"

static uint32_t is_fullscreen = 1;

static uint32_t vidwidth;
static uint32_t vidheight;

static uint32_t screenwidth;
static uint32_t screenheight;
static uint32_t screendepth = 2; //Only 16bpp supported right now

static uint32_t dispwidth = 640; // You can change these to whatever you want
static uint32_t dispheight = 360; // 16:9 screen ratio??
static uint32_t dispx;
static uint32_t dispy;

static uint32_t *vidpage0;
static uint32_t *vidpage1;
static uint32_t *vidpage2;

static uint32_t vidpage0offset;
static uint32_t vidpage1offset;
static uint32_t vidpage2offset;

// Current pointer into framebuffer where display is located
static uint32_t targetoffset;

static uint32_t page_space;

static voodoo_io_reg *reg_IO;
static voodoo_2d_reg *reg_2d;
static voodoo_yuv_reg *reg_YUV;
static voodoo_yuv_fb *fb_YUV;

static uint32_t *memBase0, *memBase1;
static uint32_t baseAddr0, baseAddr1;


/* X11 related variables */
static Display *display;
static Window mywindow;
static int bpp;
static XWindowAttributes attribs;
static int X_already_started = 0;

static int _3dfx_open (plugin_t *plugin, void *name);
static int _3dfx_close (plugin_t *plugin);
static int _3dfx_setup (plugin_output_video_attr_t *attr);
static int _3dfx_draw_frame (uint8_t *src[]);
static int _3dfx_draw_slice (uint8_t *src[], uint32_t slice_num);
static void _3dfx_flip_page (void) ;
static vo_image_buffer_t* allocate_image_buffer(uint32_t height, uint32_t width, uint32_t format);
static void free_image_buffer(vo_image_buffer_t* image);

static struct dfx_priv_s {
        int fd;
} _dfx_priv;

static plugin_output_video_t video_3dfx = {
	open:           _3dfx_open,
	close:          _3dfx_close,
	setup:          _3dfx_setup,
	draw_frame:     _3dfx_draw_frame,
	draw_slice:     _3dfx_draw_slice,
	flip_page:      _3dfx_flip_page,
	allocate_image_buffer:  allocate_image_buffer,
	free_image_buffer:      free_image_buffer
};


/**
 *
 **/

static int _3dfx_open (plugin_t *plugin, void *name)
{
	if ((_dfx_priv.fd = open ((char *) name, O_RDWR)) < 0) {
		LOG (LOG_ERROR, "Can't open %s\n", (char *) name);
		return -1;
	}

	return 0;
}


/**
 *
 **/

static int _3dfx_close (plugin_t *plugin)
{
	close (_dfx_priv.fd);
	_dfx_priv.fd = -1;

	return 0;
}


/**
 *
 **/

static void restore (void) 
{
	//reg_IO->vidDesktopStartAddr = vidpage0offset;
	XF86DGADirectVideo (display,0,0);
}


/**
 *
 **/

static void sighup (int foo) 
{
	//reg_IO->vidDesktopStartAddr = vidpage0offset;
	XF86DGADirectVideo (display,0,0);
	//exit(0);
}


/**
 *
 **/

static void restore_regs(voodoo_2d_reg *regs) 
{
	reg_2d->commandExtra = regs->commandExtra;
	reg_2d->clip0Min = regs->clip0Min;
	reg_2d->clip0Max = regs->clip0Max;

	reg_2d->srcBaseAddr = regs->srcBaseAddr;
	reg_2d->srcXY = regs->srcXY;
	reg_2d->srcFormat = regs->srcFormat;
	reg_2d->srcSize = regs->srcSize;

	reg_2d->dstBaseAddr = regs->dstBaseAddr;
	reg_2d->dstXY = regs->dstXY;
	reg_2d->dstFormat = regs->dstFormat;

	reg_2d->dstSize = regs->dstSize;
	reg_2d->command = 0;
}


/**
 *
 **/

static uint32_t create_window(Display *display) 
{
	int screen;
	unsigned int fg, bg;
	char *hello = "I hate X11";
	XSizeHints hint;
	XVisualInfo vinfo;
	XEvent xev;

	Colormap theCmap;
	XSetWindowAttributes xswa;
	unsigned long xswamask;

	if (X_already_started)
		return -1;

	screen = DefaultScreen(display);

	hint.x = 0;
	hint.y = 10;
	hint.width = dispwidth;
	hint.height = dispheight;
	hint.flags = PPosition | PSize;

	bg = WhitePixel(display, screen);
	fg = BlackPixel(display, screen);

	XGetWindowAttributes(display, DefaultRootWindow(display), &attribs);
	if ((bpp = attribs.depth) != 16) {
		LOG (LOG_ERROR, "Only 16bpp supported!");
		return -1;
	}

	XMatchVisualInfo(display,screen,bpp,TrueColor,&vinfo);
	printf("visual id is  %lx\n",vinfo.visualid);

	theCmap = XCreateColormap(display, RootWindow(display,screen),
			vinfo.visual, AllocNone);

	xswa.background_pixel = 0;
	xswa.border_pixel     = 1;
	xswa.colormap         = theCmap;
	xswamask = CWBackPixel | CWBorderPixel |CWColormap;


	mywindow = XCreateWindow(display, RootWindow(display,screen),
				 hint.x, hint.y, hint.width, hint.height, 4, bpp,CopyFromParent,vinfo.visual,xswamask,&xswa);

	XSelectInput(display, mywindow, StructureNotifyMask);

	/* Tell other applications about this window */

	XSetStandardProperties(display, mywindow, hello, hello, None, NULL, 0, &hint);

	/* Map window. */

	XMapWindow(display, mywindow);

	/* Wait for map. */
	do 
	{
		XNextEvent(display, &xev);
	}
	while (xev.type != MapNotify || xev.xmap.event != mywindow);

	XSelectInput(display, mywindow, NoEventMask);

	XFlush(display);
	XSync(display, False);

	X_already_started++;
	return 0;
}


/**
 *
 **/

static void dump_yuv_planar (uint32_t *y, uint32_t *u, uint32_t *v, uint32_t to, uint32_t width, uint32_t height) 
{
	// YUV conversion works like this:
	//
	//		 We write the Y, U, and V planes separately into 3dfx YUV Planar memory
	//		 region.  The nice chip then takes these and packs them into the YUYV
	//		 format in the regular frame buffer, starting at yuvBaseAddr, page 2 here.
	//		 Then we tell the 3dfx to do a Screen to Screen Stretch BLT to copy all 
	//		 of the data on page 2 onto page 1, converting it to 16 bpp RGB as it goes.
	//		 The result is a nice image on page 1 ready for display. 

	uint32_t j;
	uint32_t y_imax, uv_imax, jmax;

	reg_YUV->yuvBaseAddr = to;
	reg_YUV->yuvStride = screenwidth*2;

	LOG (LOG_DEBUG,  "video_out_3dfx: starting planar dump\n");
	jmax = height>>1;	// vidheight/2, height of U and V planes
	y_imax = width>>2;	// Y plane is twice as wide as U and V planes
	uv_imax = width>>3;	// vidwidth/2/4, width of U and V planes in 32-bit words

	for (j=0;j<jmax;j++) {
		memcpy(fb_YUV->U + (uint32_t) VOODOO_YUV_STRIDE*  j       ,
			u + (uint32_t) uv_imax*  j       , uv_imax<<2);
		memcpy(fb_YUV->V + (uint32_t) VOODOO_YUV_STRIDE*  j       ,
			v + (uint32_t) uv_imax*  j       , uv_imax<<2);
		memcpy(fb_YUV->Y + (uint32_t) VOODOO_YUV_STRIDE* (j<<1)   ,
			y + (uint32_t) y_imax * (j<<1)   , y_imax<<2);
		memcpy(fb_YUV->Y + (uint32_t) VOODOO_YUV_STRIDE*((j<<1)+1),
			y + (uint32_t) y_imax *((j<<1)+1), y_imax<<2);
	}
	LOG (LOG_DEBUG, "video_out_3dfx: done planar dump\n");
}


/**
 *
 **/

static void screen_to_screen_stretch_blt(uint32_t to, uint32_t from, uint32_t width, uint32_t height) 
{
	//FIXME - this function should be called by a show_frame function that
	//        uses a series of blts to show only those areas not covered
	//        by another window
	voodoo_2d_reg saved_regs;

	LOG (LOG_DEBUG, "video_out_3dfx: saving registers\n");
	// Save VGA regs (so X kinda works when we're done)
	saved_regs = *reg_2d;

// The following lines set up the screen to screen stretch blt from page2 to page 1

	LOG (LOG_DEBUG, "video_out_3dfx: setting blt registers\n");
	reg_2d->commandExtra = 4; //disable colorkeying, enable wait for v-refresh (0100b)
	reg_2d->clip0Min = 0;
	reg_2d->clip0Max = 0xFFFFFFFF; //no clipping

	reg_2d->srcBaseAddr = from;
	reg_2d->srcXY = 0;
	reg_2d->srcFormat = screenwidth*2 | VOODOO_BLT_FORMAT_YUYV; // | 1<<21;
	reg_2d->srcSize = vidwidth | (vidheight << 16);

	reg_2d->dstBaseAddr = to;
	reg_2d->dstXY = 0;
	reg_2d->dstFormat = screenwidth*2 | VOODOO_BLT_FORMAT_16;

	reg_2d->dstSize = width | (height << 16);

	LOG (LOG_DEBUG, "video_out_3dfx: starting blt\n");
	// Executes screen to screen stretch blt
	reg_2d->command = 2 | 1<<8 | 0xCC<<24;

	LOG (LOG_DEBUG, "video_out_3dfx: restoring regs\n");
	restore_regs(&saved_regs);

	LOG (LOG_DEBUG, "video_out_3dfx: done blt\n");
}


/**
 *
 **/

static void update_target (void) 
{
	uint32_t xp, yp, w, h, b, d;
	Window root;

	XGetGeometry(display,mywindow,&root,&xp,&yp,&w,&h,&b,&d);
	XTranslateCoordinates(display,mywindow,root,0,0,&xp,&yp,&root);
	dispx = (uint32_t) xp;
	dispy = (uint32_t) yp;
	dispwidth = (uint32_t) w;
	dispheight = (uint32_t) h;

	if (is_fullscreen) 
		targetoffset = vidpage0offset + (screenheight - dispheight)/2*screenwidth*screendepth + (screenwidth-dispwidth)/2*screendepth;
	else 
		targetoffset = vidpage0offset + (dispy*screenwidth + dispx)*screendepth;
}


/**
 *
 **/

static int _3dfx_setup (plugin_output_video_attr_t *attr)
{
	pioData data;
	uint32_t retval;

	if (getenv("DISPLAY"))
		display = XOpenDisplay (getenv("DISPLAY"));
	else
		display = XOpenDisplay(":0.0");

	screenwidth = XDisplayWidth(display,0);
	screenheight = XDisplayHeight(display,0);

	page_space = screenwidth*screenheight*screendepth;
	vidpage0offset = 0;
	vidpage1offset = page_space;  // Use third and fourth pages
	vidpage2offset = page_space*2;

	signal(SIGALRM,sighup);
	//alarm(120);

	// Store sizes for later
	vidwidth = attr->width;
	vidheight = attr->height;

	is_fullscreen = attr->fullscreen = 0;
	if (!is_fullscreen) 
		create_window(display);

	// Ask 3dfx driver for base memory address 0
	data.port = 0x10; // PCI_BASE_ADDRESS_0_LINUX;
	data.size = 4;
	data.value = &baseAddr0;
	data.device = 0;
	if ((retval = ioctl(_dfx_priv.fd,_IOC(_IOC_READ,'3',3,0),&data)) < 0)
		goto error;

	// Ask 3dfx driver for base memory address 1
	data.port = 0x14; // PCI_BASE_ADDRESS_1_LINUX;
	data.size = 4;
	data.value = &baseAddr1;
	data.device = 0;
	if ((retval = ioctl(_dfx_priv.fd,_IOC(_IOC_READ,'3',3,0),&data)) < 0) 
		goto error;

	// Map all 3dfx memory areas
	memBase0 = mmap(0,0x1000000,PROT_READ | PROT_WRITE,MAP_SHARED,_dfx_priv.fd,baseAddr0);
	memBase1 = mmap(0,3*page_space,PROT_READ | PROT_WRITE,MAP_SHARED,_dfx_priv.fd,baseAddr1);
	if (memBase0 == (uint32_t *) 0xFFFFFFFF || memBase1 == (uint32_t *) 0xFFFFFFFF) 
	{
		printf("Couldn't map 3dfx memory areas: %p,%p,%d\n", 
		 memBase0,memBase1,errno);
	}  

	// Set up global pointers
	reg_IO  = (void *)memBase0 + VOODOO_IO_REG_OFFSET;
	reg_2d  = (void *)memBase0 + VOODOO_2D_REG_OFFSET;
	reg_YUV = (void *)memBase0 + VOODOO_YUV_REG_OFFSET;
	fb_YUV  = (void *)memBase0 + VOODOO_YUV_PLANE_OFFSET;

	vidpage0 = (void *)memBase1 + (unsigned long int)vidpage0offset;
	vidpage1 = (void *)memBase1 + (unsigned long int)vidpage1offset;
	vidpage2 = (void *)memBase1 + (unsigned long int)vidpage2offset;

	// Clear pages 1,2,3 
	// leave page 0, that belongs to X.
	// So does part of 1.  Oops.
	memset(vidpage1,0x00,page_space);
	memset(vidpage2,0x00,page_space);

	if (is_fullscreen) 
		memset(vidpage0,0x00,page_space);


#ifndef VOODOO_DEBUG
	// Show page 0 (unblanked)
	reg_IO->vidDesktopStartAddr = vidpage0offset;

	/* Stop X from messing with my video registers!
		 Find a better way to do this?
		 Currently I use DGA to tell XF86 to not screw with registers, but I can't really use it
		 to do FB stuff because I need to know the absolute FB position and offset FB position
		 to feed to BLT command 
	*/
	//XF86DGADirectVideo(display,0,XF86DGADirectGraphics); //| XF86DGADirectMouse | XF86DGADirectKeyb);
#endif

	/* fd is deliberately not closed - if it were, mmaps might be released??? */

	atexit(restore);

	LOG (LOG_DEBUG, "(display) 3dfx initialized %p\n",memBase1);
	return 0;

error:
	LOG (LOG_ERROR, "Error: %d\n",retval);
	return -1;
}


/**
 *
 **/

static int _3dfx_draw_frame (uint8_t *src[]) 
{
	LOG (LOG_DEBUG, "video_out_3dfx: starting display_frame\n");

	// Put packed data onto page 2
	dump_yuv_planar((uint32_t *)src[0],(uint32_t *)src[1],(uint32_t *)src[2],
			vidpage2offset,vidwidth,vidheight);

	LOG (LOG_DEBUG, "video_out_3dfx: done display_frame\n");
	return 0;
}


/**
 *
 **/

static int _3dfx_draw_slice (uint8_t *src[], uint32_t slice_num) 
{
	uint32_t target;

	target = vidpage2offset + (screenwidth*2 * 16*slice_num);
	dump_yuv_planar((uint32_t *)src[0],(uint32_t *)src[1],(uint32_t *)src[2],target,vidwidth,16);
	return 0;
}


/**
 *
 **/

static void _3dfx_flip_page (void) 
{
	//FIXME - update_target() should be called by event handler when window
	//        is resized or moved
	update_target();
	LOG (LOG_DEBUG, "video_out_3dfx: calling blt function\n");
	screen_to_screen_stretch_blt(targetoffset, vidpage2offset, dispwidth, dispheight);
}


/**
 *
 **/

static vo_image_buffer_t* allocate_image_buffer(uint32_t height, uint32_t width, uint32_t format)
{
	vo_image_buffer_t *image;

	if (!(image = malloc (sizeof (vo_image_buffer_t))))
		return NULL;

	image->height   = height;
	image->width    = width;
	image->format   = format;

	//we only know how to do 4:2:0 planar yuv right now.
	if (!(image->base = malloc (width * height * 3 / 2))) {
		free(image);
		return NULL;
	}

	return image;
}


/**
 *
 **/

static void free_image_buffer(vo_image_buffer_t* image)
{
	free (image->base);
	free (image);
}


/**
 * Initialize Plugin.
 **/

int plugin_init (char *whoami)
{
	pluginRegister (whoami,
		PLUGIN_ID_OUTPUT_VIDEO,
		"3dfx",
		NULL,
		NULL,
		&video_3dfx);

	return 0;
}


/**
 * Cleanup Plugin.
 **/

void plugin_exit (void)
{
}

