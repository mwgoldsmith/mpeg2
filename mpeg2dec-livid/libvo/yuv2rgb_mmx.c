/*
 * yuv2rgb_mmx.c, Software YUV to RGB coverter with Intel MMX "technology"
 *
 * Copyright (C) 2000, Silicon Integrated System Corp.
 * All Rights Reserved.
 *
 * Author: Olie Lho <ollie@sis.com.tw>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video decoder
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Make; see the file COPYING. If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#ifdef ARCH_X86

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "attributes.h"
#include "mmx.h"
#include "yuv2rgb.h"
#include "yuv2rgb_mmx.h"

#if 1
#define movntq "movq"	/* for MMX-only processors */
#else
#define movntq "movntq"	/* use this for processors that have SSE or 3Dnow */
#endif

static void yuv420_rgb16_mmx (uint8_t *image,
			      uint8_t *py, uint8_t *pu, uint8_t *pv,
			      int h_size, int v_size,
			      int rgb_stride, int y_stride, int uv_stride)
{
	int even = 1;
	int x = 0, y = 0;

	while (y < v_size) {
		while (x < h_size) {
		    /* this code deals with SINGLE scan line at a time, */
		    /* it converts 8 pixels in each iteration */
			__asm__ (".align 8\n\t"
				MMX_INIT
				: : "r" (py), "r" (pu), "r" (pv), "r" (image));
			__asm__ (MMX_YUV_MUL : :);
			__asm__ (MMX_YUV_ADD : :);
			__asm__ (MMX_UNPACK_16RGB : : "r" (image));

			py += 8;
			pu += 4;
			pv += 4;
			image += 16;
			x += 8;
		};

		if (even) {
			pu -= h_size/2;
			pv -= h_size/2;
		} else {
			pu += (uv_stride - h_size/2);
			pv += (uv_stride - h_size/2);
		}

		py += (y_stride - h_size);
		image += (rgb_stride - 2*h_size);

		x = 0;
		y ++;
		even = (!even);
	};

	/* __asm__ ("emms\n\t"); */
}


static void yuv420_argb32_mmx (uint8_t * image, uint8_t * py,
			       uint8_t * pu, uint8_t * pv,
			       int h_size, int v_size,
			       int rgb_stride, int y_stride, int uv_stride)
{
	int even = 1;
	int x = 0, y = 0;

	while (y < v_size)  {
	    while (x < h_size)  {
		/* this mmx assembly code deals with SINGLE scan line at */
		/* a time, it convert 8 pixels in each iteration */

		/* load data for start of next scan line */
			__asm__ (MMX_INIT
				: : "r" (py), "r" (pu), "r" (pv), "r" (image));

			__asm__ (".align 8 \n\t"
				MMX_YUV_MUL
				MMX_YUV_ADD
				MMX_UNPACK_32RGB
				: : "r" (py), "r" (pu), "r" (pv), "r" (image));

			py += 8;
			pu += 4;
			pv += 4;
			image += 32;
			x += 8;
		}

		if (even) {
			pu -= h_size/2;
			pv -= h_size/2;
		} else {
			pu += (uv_stride - h_size/2);
			pv += (uv_stride - h_size/2);
		}

		py += (y_stride - h_size);
		image += (rgb_stride - 4*h_size);

		x = 0;
		y += 1;
		even = (!even);
	}

	/* __asm__ ("emms\n\t"); */
}


yuv2rgb_fun yuv2rgb_init_mmx (int bpp, int mode)
{
    /* FIXME this code is broken */
	if (bpp == 15 || bpp == 16) {
		if (mode == MODE_RGB)
			return yuv420_rgb16_mmx;
		if (mode == MODE_BGR)
			return yuv420_rgb16_mmx;
		/* return NULL; */
		return yuv420_rgb16_mmx;
	}

	if (bpp == 24) {
		if (mode == MODE_RGB)
			return NULL;
		else if (mode == MODE_BGR)
			return NULL;
	}

	if (bpp == 32) {
		if (mode == MODE_RGB)
			return yuv420_argb32_mmx;
		else if (mode == MODE_BGR)
			return NULL;
	}

	return NULL; /* Fallback to C */
}

#endif
