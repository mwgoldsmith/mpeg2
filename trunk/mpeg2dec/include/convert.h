/*
 * convert.h
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef CONVERT_H
#define CONVERT_H

mpeg2_convert_t convert_rgb32;
mpeg2_convert_t convert_rgb24;
mpeg2_convert_t convert_rgb16;
mpeg2_convert_t convert_rgb15;
mpeg2_convert_t convert_rgb8;
mpeg2_convert_t convert_bgr32;
mpeg2_convert_t convert_bgr24;
mpeg2_convert_t convert_bgr16;
mpeg2_convert_t convert_bgr15;
mpeg2_convert_t convert_bgr8;

#define CONVERT_RGB 0
#define CONVERT_BGR 1
mpeg2_convert_t * convert_rgb (int order, int bpp);

#endif /* CONVERT_H */
