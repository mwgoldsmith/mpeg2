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

//
// We should remove the typdefs in oms for vo_image_buffer_t 
// plugin_output_video_attr_t and let OMS include
// them from the video.
// INCLUDES ="${INCLUDES} $video_lib_includes" and let video lib
// define this :-)

#ifdef __OMS__
#include "../../output/video/output_video.h"
#else
#include "../video/output_video.h"
#endif

//
// Structure for the mpeg2dec decoder.
// 

//
// Backward compatible old interface [START]
//

void mpeg2_init (void);
int mpeg2_decode_data (vo_output_video_t *, 
		       uint8_t * data_start, uint8_t * data_end);
void mpeg2_close (vo_output_video_t *);
void mpeg2_drop (int flag);
void mpeg2_output_init (int flag);


//
// Backward compatible old interface [END]
//


//
// Proposed new interface [START]
//


// forward declaration (hidden in mpeg2_internal.h)
#ifndef __MPEG2_INTERNAL
typedef void* picture_t;
#endif


typedef struct mpeg2dec_s {
    vo_output_video_t* output;
    vo_output_video_attr_t attr;
    void* user_data;

    // here we store the allocated images, which we get from
    // the output interface
    img_buf_t * frame[3];
 
    // this is where we keep the state of the decoder
    picture_t * picture;
    
    uint32_t shift;
    int is_display_initialized;
    int is_sequence_needed;
    int drop_flag;
    int drop_frame;
    int in_slice;

    // the maximum chunk size is determined by vbv_buffer_size 
    // which is 224K for
    // MP@ML streams. 
    // (we make no pretenses ofdecoding anything more than that)
    // allocated in init gcc has problems allocating
    // such big structures
    uint8_t* chunk_buffer;
    // pointer to current position in chunk_buffer
    uint8_t* chunk_ptr;
    // last start code ?
    uint8_t code;

} mpeg2dec_t ;





// initialize mpegdec with a opaque user pointer
// if not needed in the output use NULL here
void mpeg2_init_ng (mpeg2dec_t* mpeg2dec,
		    vo_output_video_t* output,void* user_data);

// destroy everything which was allocated, shutdown the output
void mpeg2_close_ng (mpeg2dec_t* mpeg2dec);


int mpeg2_decode_data_ng (mpeg2dec_t* mpeg2dec,
			  uint8_t * data_start, uint8_t * data_end);

void mpeg2_drop_ng (mpeg2dec_t* mpeg2dec, int flag);
void mpeg2_output_init_ng (mpeg2dec_t* mpeg2dec, int flag);


//
// Proposed new interface [END]
//
