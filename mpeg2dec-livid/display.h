/* 
 *    display.h
 *
 *	Copyright (C) Aaron Holtzman - Aug 1999
 *
 *  This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
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

/*
 * Initialize the display driver.
 *
 *    params : width  == width of video to display.
 *             height == height of video to display.
 *             fullscreen == non-zero if driver should attempt to
 *                           render in fullscreen mode. Zero if
 *                           a windowed mode is requested. This is
 *                           merely a request; if the driver can only do
 *                           fullscreen (like fbcon) or windowed (like X11),
 *                           than this param may be disregarded.
 *             title == string for titlebar of window. May be disregarded
 *                      if there is no such thing as a window to your
 *                      driver. Make a copy of this string, if you need it.
 *   returns : non-zero on successful initialization, zero on error.
 *              The program will probably respond to an error condition
 *              by terminating.
 */
uint_32 display_init(uint_32 width, uint_32 height, uint_32 fullscreen, char *title);

/*
 * Display a new frame of the video. This gets called very rapidly, so
 *  the more efficient you can make your implementation of this function,
 *  the better.
 *
 *    params : *src[] == A array with three elements. This is a YUV
 *                       stream, with the Y plane in src[0], U in src[1],
 *                       and V in src[2]. There is enough data for an image
 *                       that is (WxH) pixels, where W and H are the width
 *                       and height parameters that were previously passed
 *                       to display_init().
 *                         Information on the YUV format can be found at:
 *                           http://www.webartz.com/fourcc/fccyuv.htm#IYUV
 *
 *   returns : non-zero on successful rendering, zero on error.
 *              The program will probably respond to an error condition
 *              by terminating.
 */
uint_32 display_frame(uint_8 *src[]);


//FIXME document this when things settle down
uint_32 display_slice(uint_8 *src[],uint_32 slice_num);
void display_flip_page(void);
void* display_allocate_buffer(uint_32 num_bytes);
