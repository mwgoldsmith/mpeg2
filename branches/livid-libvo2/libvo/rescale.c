//TOAST_SPU will define ALL spu entries - no matter the tranparency
//#define TOAST_SPU

/*
 *
 * Copyright (C) 2000  Thomas Mirlacher
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * The author may be reached as <dent@linuxvideo.org>
 *
 *------------------------------------------------------------
 *
 */


#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "log.h"
#include "video_out.h"

static uint32_t *index_tbl_x;
static uint32_t *index_tbl_y;
static uint32_t dst_width;
static uint32_t dst_height;
static uint32_t src_width;

#define BPP_T uint16_t
#define BPP 2

void rescale (uint8_t *img)
{
	BPP_T *dst = (BPP_T *) img;
	BPP_T *src;
	int y = 0;

	uint32_t *y_indx = index_tbl_y;
	uint32_t *y_end = &index_tbl_y[dst_height];
	uint32_t *x_indx;
	uint32_t *x_end = &index_tbl_x[dst_width];

	while (y_indx < y_end) {		// do y rescaling
		src = (BPP_T *) (img + *y_indx*src_width*BPP);
		dst = (BPP_T *) (img + y*src_width*BPP); 
		y++;

		x_indx = index_tbl_x;
		y_indx++;

		while (x_indx < x_end) {	// do x rescaling
			*dst++ = *(src + *x_indx++);
		}
	}
}


static uint32_t *build_tbl (uint32_t *tbl, int dst_size, int src_size)
{
	uint16_t i;

// don't do upscaling right now
	if (dst_size > src_size)
		dst_size = src_size;
// don't do upscaling right now

	if (tbl)
		free (tbl);

	tbl = malloc (dst_size * sizeof (int));

	for (i=0; i<src_size; i++) {
		tbl[(i*dst_size)/src_size] = i;
	}

	return tbl;
}


void rescale_set_factors (int _dst_width, int _dst_height, int _src_width, int _src_height)
{
	dst_width = _dst_width;
	dst_height = _dst_height;
	src_width = _src_width;

	index_tbl_x = build_tbl (index_tbl_x, _dst_width, _src_width);
	index_tbl_y = build_tbl (index_tbl_y, _dst_height, _src_height);
}


void rescale_init (int bpp, int mode)
{
	switch (bpp) {
	case 32:
//		rescale = rescale_32bpp;
		break;
	case 15:
	case 16:
//		rescale = rescale_16bpp;
		break;
	case 8:
//		rescale = rescale_8bpp;
		break;
	default:
		LOG (LOG_ERROR, "%ibpp not supported by rescale", bpp);
	}
}


//---------------------------------------- OVERLAY
#include "tux.h"

#define BLEND_COLOR(dst, src, mask, o) ((((src&mask)*(0xf-o) + ((dst&mask)*o))/0xf) & mask)


static uint16_t blend_rgb16 (uint16_t dst, uint16_t src, uint8_t o)
{
	return  BLEND_COLOR (dst, src, 0xf800, o) |
		BLEND_COLOR (dst, src, 0x07e0, o) |
		BLEND_COLOR (dst, src, 0x001f, o) ;
}


void overlay_tux_rgb (uint8_t *img, int dst_width, int dst_height)
{
	int src_width = bg_width;
	int src_height = bg_height;
	uint8_t *src = (uint8_t *) bg_img_data;
	static int x_off, y_off;
	static int x_dir=1, y_dir=1;
	static int o=5, o_dir=1;

// align right bottom
	x_off += x_dir;
	if (x_off > (dst_width - src_width))
		x_dir = -x_dir;
	if (x_off <= 0)
		x_dir = -x_dir;

	y_off += y_dir;
	if (y_off > (dst_height - src_height))
		y_dir = -y_dir;
	if (y_off <= 0)
		y_dir = -y_dir;

// cycle parameters
	o += o_dir;
	if (o >= 0xf)
		o_dir = -o_dir;
	if (o <= 1)
		o_dir = -o_dir;
//
	{
        BPP_T *dst = (BPP_T *) img;
        int x, y;

	dst += y_off*dst_width;
        for (y=0; y<src_height; y++) {
		dst += x_off;
                for (x=0; x<src_width; x++) {
			if ((*src)-bg_start_index)
				*dst = blend_rgb16 (bg_palette_to_rgb[(*src)-bg_start_index], *dst, o);
			src++;
			dst++;
                }
		dst += dst_width - x - x_off;
        }
	}
}


void overlay_rgb (uint8_t *img, overlay_buf_t *img_overl, int dst_width, int dst_height)
{
	uint8_t *src = (uint8_t *) img_overl->data;
	static int o=5;

u_int myclut[]= {
        0x0000,
        0x20e2,
        0x83ac,
        0x4227,
        0xa381,
        0xad13,
        0xbdf8,
        0xd657,
        0xee67,
        0x6a40,
        0xd4c1,
        0xf602,
        0xf664,
        0xe561,
        0xad13,
        0xffdf,
};

	overlay_tux_rgb (img, dst_width, dst_height);

	{
        BPP_T *dst = (BPP_T *) img;
        int x, y;

	dst += img_overl->y*dst_width;
        for (y=0; y<img_overl->height; y++) {
		dst += img_overl->x;
                for (x=0; x<img_overl->width; x++) {
#ifndef TOAST_SPU
			o = img_overl->trans[*src&0x0f];

			if ((*src&0x0f) != 0)	// if alpha is != 0
				*dst = blend_rgb16 (myclut[img_overl->clut[(*src&0x0f)]], *dst, o);
#else
			o = 15; //img_overl->trans[*src&0x0f];

			*dst = blend_rgb16 (myclut[img_overl->clut[(*src&0x0f)]], *dst, o);
#endif
			src++;
			dst++;
                }
		dst += dst_width - x - img_overl->x;
        }
	}
}

//---------------------------------------- OVERLAY
#if 0
// FIXME

#define BLEND_YUV(dst, src, o) (((src)*o + ((dst)*(0xf-o)))/0xf)


static uint64_t blend_yuv4 (uint64_t dst, uint64_t src, uint8_t o)
{
	return BLEND_YUV (dst, src, o);
}

static uint32_t blend_yuv2 (uint32_t dst, uint32_t src, uint8_t o)
{
	return BLEND_YUV (dst, src, o);
}


void overlay_yuv420 (uint8_t *img, int dst_width, int dst_height)
{
	int src_width = bg_width;
	int src_height = bg_height;
	uint8_t *src = (uint8_t *) bg_img_data;
	static int x_off, y_off;
	static int x_dir=1, y_dir=1;
	static int o=5, o_dir=1;

        BPP_T *dst_y = (BPP_T *) img.y;
        BPP_T *dst_cr = (BPP_T *) img.cr;
        BPP_T *dst_cb = (BPP_T *) img.cb;
        int x, y;

	dst += y_off*dst_width;
        for (y=0; y<src_height; y++) {
		dst += x_off;
                for (x=0; x<src_width; x++) {
			if ((*src)-bg_start_index) {
				*dst_y = blend_yuv4 (bg_palette_to_rgb[(*src)-bg_start_index], *dst, o);
				*dst_cr = blend_yuv2 (bg_palette_to_rgb[(*src)-bg_start_index], *dst, o);
				*dst_cb = blend_yuv2 (bg_palette_to_rgb[(*src)-bg_start_index], *dst, o);
			}

			src++;
			dst_y += 4;
			dst_cr += 2;
			dst_cb += 2;
                }
// FIXME
		dst += dst_width - x - x_off;
        }
}
#endif
