/*
 *  video_out.h
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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef AARONS_TYPES
#define AARONS_TYPES
//typedef to appropriate type for your architecture
typedef unsigned char uint_8;
typedef unsigned short uint_16;
typedef unsigned int uint_32;
typedef signed int sint_32;
typedef signed short sint_16;
typedef signed char sint_8;
#endif

typedef struct vo_info_s
{
	/* driver name ("Matrox Millennium G200/G400" */
	const char *name;
	/* short name (for config strings) ("mga") */
	const char *short_name;
	/* author ("Aaron Holtzman <aholtzma@ess.engr.uvic.ca>") */
	const char *author;
	/* any additional comments */
	const char *comment;
} vo_info_t;

typedef struct vo_image_buffer_s
{
	uint_32 height;
	uint_32 width;
	uint_32 format;
	uint_8 *base;
	void *private;
} vo_image_buffer_t;

typedef struct vo_functions_s
{
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
	 *   returns : zero on successful initialization, non-zero on error.
	 *              The program will probably respond to an error condition
	 *              by terminating.
	 */

	uint_32 (*init)(uint_32 width, uint_32 height, uint_32 fullscreen, char *title);

	/*
	 * Return driver information.
	 *
	 *    params : none.
	 *   returns : read-only pointer to a vo_info_t structure.
	 *             Fields are non-NULL.
	 *             Should not return NULL.
	 */

	const vo_info_t* (*get_info)(void);

	/*
	 * Display a new frame of the video to the screen. This may get called very
	 *  rapidly, so the more efficient you can make your implementation of this
	 *  function, the better.
	 *
	 *    params : *src[] == An array with three elements. This is a YUV
	 *                       stream, with the Y plane in src[0], U in src[1],
	 *                       and V in src[2]. There is enough data for an image
	 *                       that is (WxH) pixels, where W and H are the width
	 *                       and height parameters that were previously passed
	 *                       to display_init().
	 *                         Information on the YUV format can be found at:
	 *                           http://www.webartz.com/fourcc/fccyuv.htm#IYUV
	 *
	 *   returns : zero on successful rendering, non-zero on error.
	 *              The program will probably respond to an error condition
	 *              by terminating.
	 */

	uint_32 (*draw_frame)(uint_8 *src[]);

	/*
	 * Update a section of the offscreen buffer. A "slice" is an area of the
	 *  video image that is 16 rows of pixels at the width of the video image.
	 *  Position (0, 0) is the upper left corner of slice #0 (the first slice),
	 *  and position (0, 15) is the lower right. The next slice, #1, is bounded
	 *  by (0, 16) and (0, 31), and so on.
	 *
	 * Note that slices are not drawn directly to the screen, and should be
	 *  buffered until your implementation of display_flip_page() (see below)
	 *  is called.
	 *
	 * This may get called very rapidly, so the more efficient you can make your
	 *  implementation of this function, the better.
	 *
	 *     params : *src[] == see display_frame(), above. The data passed in this
	 *                         array is just what enough data to contain the
	 *                         new slice, and NOT the entire frame.
	 *              slice_num == The index of the slice. Starts at 0, not 1.
	 *
	 *   returns : zero on successful rendering, non-zero on error.
	 *              The program will probably respond to an error condition
	 *              by terminating.
	 */

	uint_32 (*draw_slice)(uint_8 *src[], uint_32 slice_num);

	/*
	 * Draw the current image buffer to the screen. There may be several
	 *  display_slice() calls before display_flip_page() is used. Note that
	 *  display_frame does an implicit page flip, so you might or might not
	 *  want to call this internally from your display_frame() implementation.
	 *
	 * This may get called very rapidly, so the more efficient you can make
	 *  your implementation of this function, the better.
	 *
	 *     params : void.
	 *    returns : void.
	 */

	void (*flip_page)(void);

	/*
	 * Allocate an image buffer. This may allow, for some drivers, some
	 * bonus acceleration (like AGP-based transfers, etc...). If nothing
	 * else, implementing this as a simple wrapper over malloc() is acceptable.
	 * This memory must be system memory as it will be read often.
	 *
	 *    params : height  == heigth of image in pixels
	 *           : width   == width of image in pixels
	 *           : format  == fourCC code corresponding to the image format
	 *   returns : NULL if unable to allocate, ptr to new surface
	 */

	vo_image_buffer_t* (*allocate_image_buffer)(uint_32 height, uint_32 width, uint_32 format);

	/*
	 * Free an image buffer allocated with allocate_image_buffer.
	 *
	 *    params : image == image buffer to be freed
	 *   returns : none
	 */

	void	(*free_image_buffer)(vo_image_buffer_t* image);
} vo_functions_t;

// NULL terminated array of all drivers 
extern vo_functions_t* video_out_drivers[];

#ifdef __cplusplus
}
#endif
