
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


#ifndef __RESCALE_H__
#define __RESCALE_H__

void rescale_init (int bpp, int mode);
void rescale_set_factors (int dst_width, int dst_height, int src_width, int src_height);
void rescale (uint8_t *img);

void overlay_rgb (uint8_t *img, overlay_buf_t *overlay, int width, int height);
void overlay_yuv (uint8_t *img, int width, int height);
#endif
