/* 
 *    display_3dfx.c
 *
 *	Copyright (C) Colin Cross Apr 2000
 *
 *  This file heavily based off of display_mga_vid.c of Aaron Holtzman's
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

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <wchar.h>
#include <signal.h>

#include <X11/Xlib.h>
#include <X11/extensions/xf86dga.h>


//#define VOODOO_DEBUG

//#include "libmpeg2/debug.h"
//#include "libmpeg2/mpeg2.h"
//FIXME
//#include "libmpeg2/mpeg2_internal.h"

#include "drivers/3dfx.h"
//#include "display.h"

uint_32 vidwidth;
uint_32 vidheight;

uint_32 screenwidth;
uint_32 screenheight;
uint_32 screendepth = 2; //Only 16bpp supported right now

uint_32 dispwidth = 1280; // You can change these to whatever you want
uint_32 dispheight = 720; // 16:9 screen ratio??

uint_32 *vidpage0;
uint_32 *vidpage1;
uint_32 *vidpage2;

uint_32 vidpage0offset;
uint_32 vidpage1offset;
uint_32 vidpage2offset;

uint_32 page_space;

voodoo_io_reg *reg_IO;
voodoo_2d_reg *reg_2d;
voodoo_yuv_reg *reg_YUV;
voodoo_yuv_fb *fb_YUV;

uint_32 *memBase0, *memBase1;
uint_32 baseAddr0, baseAddr1;


Display *display;

void restore() 
{
	reg_IO->vidDesktopStartAddr = vidpage0offset;
	XF86DGADirectVideo(display,0,0);
}

void sighup() 
{
	reg_IO->vidDesktopStartAddr = vidpage0offset;
	XF86DGADirectVideo(display,0,0);
	exit(0);
}

uint_32
display_init(uint_32 width, uint_32 height, uint_32 fullscreen, char *title)
{
	int fd;
	pioData data;
	uint_32 retval;

	display = XOpenDisplay(NULL);
	screenwidth = XDisplayWidth(display,0);
	screenheight = XDisplayHeight(display,0);

	page_space = screenwidth*screenheight*2;
	vidpage0offset = 0;
	vidpage1offset = page_space;
	vidpage2offset = page_space << 1;


	signal(SIGALRM,sighup);
	//alarm(120);

	// Open driver device
	if ( (fd = open("/dev/3dfx",O_RDWR) ) == -1) 
	{
		fprintf(stderr,"Couldn't open /dev/3dfx\n");
		exit(1);
	}

	// Store sizes for later
	vidwidth = width;
	vidheight = height;

	// Ask 3dfx driver for base memory address 0
	data.port = 0x10; // PCI_BASE_ADDRESS_0_LINUX;
	data.size = 4;
	data.value = &baseAddr0;
	data.device = 0;
	if ((retval = ioctl(fd,_IOC(_IOC_READ,'3',3,0),&data)) < 0) 
	{
		printf("Error: %ld\n",retval);
		//    return 0;
	}

	// Ask 3dfx driver for base memory address 1
	data.port = 0x14; // PCI_BASE_ADDRESS_1_LINUX;
	data.size = 4;
	data.value = &baseAddr1;
	data.device = 0;
	if ((retval = ioctl(fd,_IOC(_IOC_READ,'3',3,0),&data)) < 0) 
	{
		printf("Error: %ld\n",retval);
		//    return 0;
	}

	// Map all 3dfx memory areas
	memBase0 = mmap(0,0x1000000,PROT_READ | PROT_WRITE,MAP_SHARED,fd,baseAddr0);
	memBase1 = mmap(0,3*page_space,PROT_READ | PROT_WRITE,MAP_SHARED,fd,baseAddr1);
	if (memBase0 == (uint_32 *) 0xFFFFFFFF || memBase1 == (uint_32 *) 0xFFFFFFFF) {
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

#ifndef VOODOO_DEBUG
	// Show page 0 (unblanked)
	reg_IO->vidDesktopStartAddr = vidpage0offset;

	/* Stop X from messing with my video registers!
		 Find a better way to do this?
		 Currently I use DGA to tell XF86 to not screw with registers, but I can't really use it
		 to do FB stuff because I need to know the absolute FB position and offset FB position
		 to feed to BLT command 
	*/
	XF86DGADirectVideo(display,0,XF86DGADirectGraphics & XF86DGADirectMouse & XF86DGADirectKeyb);
#endif
	/* fd is deliberately not closed - if it were, mmaps might be released??? */

	atexit(restore);

	printf("(display) 3dfx initialized %p\n",memBase1);
	return 0;
}

void 
dump_yuv_planar_frame(uint_32 *y, uint_32 *u, uint_32 *v) 
{
	uint_32 j;
	uint_32 y_imax, uv_imax, jmax;

	jmax = vidheight>>1; // vidheight/2, height of U and V planes
	uv_imax = vidwidth>>3;  // vidwidth/2/4, width of U and V planes in 32-bit words
	y_imax = uv_imax << 1; // Y plane is twice as wide as U and V planes

	for (j=0;j<jmax;j++) 
	{
		wmemcpy(fb_YUV->U + (unsigned long int)VOODOO_YUV_STRIDE*  j       ,u+(unsigned long int)uv_imax*  j       ,uv_imax);
		wmemcpy(fb_YUV->V + (unsigned long int)VOODOO_YUV_STRIDE*  j       ,v+(unsigned long int)uv_imax*  j       ,uv_imax);
		wmemcpy(fb_YUV->Y + (unsigned long int)VOODOO_YUV_STRIDE* (j<<1)   ,y+(unsigned long int)y_imax * (j<<1)   ,y_imax);
		wmemcpy(fb_YUV->Y + (unsigned long int)VOODOO_YUV_STRIDE*((j<<1)+1),y+(unsigned long int)y_imax *((j<<1)+1),y_imax);
	}
}

uint_32
display_frame(uint_8 *src[]) 
{
	uint_32 *y, *u, *v;
	voodoo_2d_reg saved_regs;

	// Get nice 32 bit pointers to Y, U, and V planes
	y = (uint_32 *)src[0];
	u = (uint_32 *)src[1];
	v = (uint_32 *)src[2];

	// Save VGA regs (so X kinda works when we're done)
	saved_regs = *reg_2d;


	/* YUV conversion works like this:
		 We write the Y, U, and V planes separately into 3dfx YUV Planar memory
		 region.  The nice chip then takes these and packs them into the YUYV
		 format in the regular frame buffer, starting at yuvBaseAddr, page 2 here.
		 Then we tell the 3dfx to do a Screen to Screen Stretch BLT to copy all 
		 of the data on page 2 onto page 1, converting it to 16 bpp RGB as it goes.
		 The result is a nice image on page 1 ready for display. 
	*/

	// Put packed data onto page 2
	reg_YUV->yuvBaseAddr = vidpage2offset;
	reg_YUV->yuvStride = screenwidth*2;


	// Dump Y, U and V planes into planar buffers (must use 32 bit writes)
	// Any way to speed this up??  Yes - see next loop
	/*for (i=0;i<vidheight;i++) {
		for (j=0;j<vidwidth/4;j++) {
			if (!(j & 1 || i & 1)) {
	fb_YUV->U[(j>>1)+1024/4*(i>>1)] = u[(j>>1)+vidwidth/4/2*(i>>1)];
	fb_YUV->V[(j>>1)+1024/4*(i>>1)] = v[(j>>1)+vidwidth/4/2*(i>>1)];
			}
			fb_YUV->Y[j+1024/4*i] = y[j+vidwidth/4*i];
		}
		}*/

	dump_yuv_planar_frame(y,u,v);

#ifndef VOODOO_DEBUG
	/* The following lines set up the screen to screen stretch blt from page2 to
		 page 1
	*/
	reg_2d->commandExtra = 4; //disable colorkeying, enable wait for v-refresh
	reg_2d->clip0Min = 0;
	reg_2d->clip0Max = 0xFFFFFFFF; //no clipping

	reg_2d->srcBaseAddr = vidpage2offset;
	reg_2d->srcXY = 0;
	reg_2d->srcFormat = screenwidth*2 | VOODOO_BLT_FORMAT_YUYV; // | 1<<21;
	reg_2d->srcSize = vidwidth | (vidheight << 16);

	reg_2d->dstBaseAddr = vidpage0offset;
	reg_2d->dstXY = 0;
	reg_2d->dstFormat = screenwidth*2 | VOODOO_BLT_FORMAT_16;

	reg_2d->dstSize = dispwidth | (dispheight << 16);

	// Executes screen to screen stretch blt
	reg_2d->command = 2 | 1<<8 | 0xCC<<24;

	// Restore registers (get rid of this!)
	*reg_2d = saved_regs;
#endif
	return 0;
}






























