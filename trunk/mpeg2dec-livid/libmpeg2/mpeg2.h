/*
 * mpeg2.h
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
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

// FIXME try to get rid of that...
#ifdef __OMS__
#include <oms/plugin/output_video.h>
#ifndef vo_functions_t
#define vo_functions_t plugin_output_video_t
#endif
#endif

void mpeg2_init (void);
int mpeg2_decode_data (vo_functions_t *, uint8_t * data_start, uint8_t * data_end);
void mpeg2_close (vo_functions_t *);
void mpeg2_drop (int flag);
void mpeg2_output_init (int flag);
