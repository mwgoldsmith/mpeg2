/*
 *  slice.c
 *
 *  Copyright (C) Aaron Holtzman <aholtzma@ess.engr.uvic.ca> - Nov 1999
 *
 *  Decodes an MPEG-2 video stream.
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
 *  the Free Software Foundation, 
 *
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "config.h"
#include "mpeg2.h"
#include "mpeg2_internal.h"

#include "motion_comp.h"
#include "bitstream.h"
#include "idct.h"

//XXX put these on the stack in slice_process?
static slice_t slice;
static macroblock_t mb ALIGN_16_BYTE;

typedef struct {
	char run, level, len;
} DCTtab;

extern DCTtab DCTtabfirst[],DCTtabnext[],DCTtab0[],DCTtab1[];
extern DCTtab DCTtab2[],DCTtab3[],DCTtab4[],DCTtab5[],DCTtab6[];
extern DCTtab DCTtab0a[],DCTtab1a[];

uint32_t non_linear_quantizer_scale[32] =
{
	 0, 1, 2, 3, 4, 5, 6, 7,
	 8,10,12,14,16,18,20,22,
	24,28,32,36,40,44,48,52,
	56,64,72,80,88,96,104,112
};

void
slice_get_slice_header(const picture_t *picture, slice_t *slice)
{
	uint32_t quantizer_scale_code;

	quantizer_scale_code = bitstream_get(5);

	if (picture->q_scale_type)
		slice->quantizer_scale = non_linear_quantizer_scale[quantizer_scale_code];
	else
		slice->quantizer_scale = quantizer_scale_code << 1 ;

	//Ignore intra_slice and all the extra data
	while(bitstream_get(1))
		bitstream_flush(8);

	//reset intra dc predictor
	slice->dc_dct_pred[0]=slice->dc_dct_pred[1]=slice->dc_dct_pred[2]= 
			1<<(picture->intra_dc_precision + 7) ;
}


//This needs to be rewritten
inline uint32_t
slice_get_block_coeff(uint16_t *run, int16_t *val, uint16_t non_intra_dc, uint16_t intra_vlc_format)
{
	uint32_t code;
	DCTtab *tab = NULL;

	//this routines handles intra AC and non-intra AC/DC coefficients
	code = bitstream_show(16);
 
	//FIXME use a pointer to the right vlc format table based on
	//intra_vlc_format
	if (code>=16384 && !intra_vlc_format) {
		if (non_intra_dc)
			tab = &DCTtabfirst[(code>>12)-4];
		else
			tab = &DCTtabnext[(code>>12)-4];
	} else if (code>=1024) {
		if (intra_vlc_format)
			tab = &DCTtab0a[(code>>8)-4];
		else
			tab = &DCTtab0[(code>>8)-4];
	} else if (code>=512) {
		if (intra_vlc_format)
			tab = &DCTtab1a[(code>>6)-8];
		else
			tab = &DCTtab1[(code>>6)-8];
	} else if (code>=256)
		tab = &DCTtab2[(code>>4)-16];
	else if (code>=128)
		tab = &DCTtab3[(code>>3)-16];
	else if (code>=64)
		tab = &DCTtab4[(code>>2)-16];
	else if (code>=32)
		tab = &DCTtab5[(code>>1)-16];
	else if (code>=16)
		tab = &DCTtab6[code-16];
	else {
		fprintf(stderr,"(vlc) invalid huffman code 0x%x in vlc_get_block_coeff()\n",code);
		exit(1);
		return 0;
	}

	bitstream_flush(tab->len);

	if (tab->run==64) // end_of_block 
		return 0;

	if (tab->run==65) { /* escape */
		*run = bitstream_get(6);

		*val = bitstream_get(12);

		if(*val >= 2048)
			*val =  *val - 4096;
	} else {
		*run = tab->run;
		*val = tab->level;
		 
		if(bitstream_get(1)) //sign bit
			*val = -*val;
	}
	return 1;
}


static void
slice_get_intra_block(const picture_t *picture,slice_t *slice,int16_t *dest,uint32_t cc)
{
	uint32_t i = 1;
	uint32_t j;
	uint16_t run;
	int16_t val;
	const uint8_t *scan = picture->scan;
	uint8_t *quant_matrix = picture->intra_quantizer_matrix;
	int16_t quantizer_scale = slice->quantizer_scale;

	//Get the intra DC coefficient and inverse quantize it
	if (cc == 0)
		dest[0] = (slice->dc_dct_pred[0] += Get_Luma_DC_dct_diff()) << (3 - picture->intra_dc_precision);
	else 
		dest[0] = (slice->dc_dct_pred[cc]+= Get_Chroma_DC_dct_diff()) << (3 - picture->intra_dc_precision);

	i = 1;
	while((slice_get_block_coeff(&run,&val,0,picture->intra_vlc_format))) {
		i += run;
		j = scan[i++];
		dest[j] = (val * quantizer_scale * quant_matrix[j]) / 16;
		//FIXME put mismatch control back in
	}
}

static void
slice_get_non_intra_block (const picture_t *picture,slice_t *slice, int16_t *dest)
{
	uint32_t i = 0;
	uint32_t j;
	uint16_t run;
	int16_t val;
	const uint8_t *scan = picture->scan;
	uint8_t *quant_matrix = picture->non_intra_quantizer_matrix;
	int16_t quantizer_scale = slice->quantizer_scale;

	while((slice_get_block_coeff(&run,&val,i==0,0))) {
		i += run;
		j = scan[i++];
		dest[j] = (((val<<1) + (val>>15))* quantizer_scale * quant_matrix[j]) >> 5;
	}
}

//This should inline easily into slice_get_motion_vector
static inline int16_t compute_motion_vector(int16_t vec,uint16_t r_size,int16_t motion_code,
		int16_t motion_residual)
{
	int16_t lim;

	lim = 16<<r_size;

	if (motion_code>0) {
		vec+= ((motion_code-1)<<r_size) + motion_residual + 1;
		if (vec>=lim)
			vec-= lim + lim;
	} else if (motion_code<0) {
		vec-= ((-motion_code-1)<<r_size) + motion_residual + 1;
		if (vec<-lim)
			vec+= lim + lim;
	}
	return vec;
}

static void slice_get_motion_vector(int16_t *prev_mv, int16_t *curr_mv,const uint8_t *f_code,
		macroblock_t *mb)
{
	int16_t motion_code, motion_residual;
	int16_t r_size;

	//fprintf(stderr,"motion_vec: h_r_size %d v_r_size %d\n",f_code[0],f_code[1]);

	// horizontal component
	r_size = f_code[0];
	motion_code = Get_motion_code();
	motion_residual =  0;
	if (r_size!=0 && motion_code!=0) 
		motion_residual = bitstream_get(r_size);

	curr_mv[0] = compute_motion_vector(prev_mv[0],r_size,motion_code,motion_residual);
	prev_mv[0] = curr_mv[0];

	//XXX dmvectors are unsed right now...
	if (mb->dmv)
		mb->dmvector[0] = Get_dmvector();

	// vertical component 
	r_size = f_code[1];
	motion_code     = Get_motion_code();
	motion_residual =  0;
	if (r_size!=0 && motion_code!=0) 
		motion_residual =  bitstream_get(r_size);

	if (mb->mvscale)
		prev_mv[1] >>= 1; 

	curr_mv[1] = compute_motion_vector(prev_mv[1],r_size,motion_code,motion_residual);

	if (mb->mvscale)
		curr_mv[1] <<= 1;

	prev_mv[1] = curr_mv[1];

	//XXX dmvectors are unsed right now...
	if (mb->dmv)
		mb->dmvector[1] = Get_dmvector();
}

//These next two functions are very similar except that they
//don't have to switch between forward and backward data structures.
//The jury is still out on whether is was worth it.
static void slice_get_forward_motion_vectors(const picture_t *picture,slice_t *slice, 
		macroblock_t *mb)
{
	if (mb->motion_vector_count==1) {
		if (mb->mv_format==MV_FIELD && !mb->dmv) {
			fprintf(stderr,"field based mv\n");
			mb->f_motion_vertical_field_select[1] = 
				mb->f_motion_vertical_field_select[0] = bitstream_get(1);
		}

		slice_get_motion_vector(slice->f_pmv[0],mb->f_motion_vectors[0],picture->f_code[0],mb);

		/* update other motion vector predictors */
		slice->f_pmv[1][0] = slice->f_pmv[0][0];
		slice->f_pmv[1][1] = slice->f_pmv[0][1];
	} else {
		mb->f_motion_vertical_field_select[0] = bitstream_get(1);
		slice_get_motion_vector(slice->f_pmv[0],mb->f_motion_vectors[0],picture->f_code[0],mb);

		mb->f_motion_vertical_field_select[1] = bitstream_get(1);
		slice_get_motion_vector(slice->f_pmv[1],mb->f_motion_vectors[1],picture->f_code[0],mb);
	}
}

static void slice_get_backward_motion_vectors(const picture_t *picture,slice_t *slice, 
		macroblock_t *mb)
{
	if (mb->motion_vector_count==1) {
		if (mb->mv_format==MV_FIELD && !mb->dmv) {
			fprintf(stderr,"field based mv\n");
			mb->b_motion_vertical_field_select[1] = 
				mb->b_motion_vertical_field_select[0] = bitstream_get(1);
		}

		slice_get_motion_vector(slice->b_pmv[0],mb->b_motion_vectors[0],picture->f_code[1],mb);

		/* update other motion vector predictors */
		slice->b_pmv[1][0] = slice->b_pmv[0][0];
		slice->b_pmv[1][1] = slice->b_pmv[0][1];
	} else {
		mb->b_motion_vertical_field_select[0] = bitstream_get(1);
		slice_get_motion_vector(slice->b_pmv[0],mb->b_motion_vectors[0],picture->f_code[1],mb);

		mb->b_motion_vertical_field_select[1] = bitstream_get(1);
		slice_get_motion_vector(slice->b_pmv[1],mb->b_motion_vectors[1],picture->f_code[1],mb);
	}
}

inline void slice_reset_pmv(slice_t *slice)
{
	memset(slice->b_pmv,0,sizeof(int16_t) * 4);
	memset(slice->f_pmv,0,sizeof(int16_t) * 4);
}

void
slice_get_macroblock(const picture_t *picture,slice_t* slice, macroblock_t *mb)
{
	uint32_t quantizer_scale_code;
	//uint32_t picture_structure = picture->picture_structure;

	// get macroblock_type 
	mb->macroblock_type = Get_macroblock_type(picture->picture_coding_type);

	// get frame/field motion type 
	if (mb->macroblock_type & (MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD)) {
		//if (picture_structure == FRAME_PICTURE) // frame_motion_type 
		mb->motion_type = picture->frame_pred_frame_dct ? MC_FRAME : bitstream_get(2);
		//else // field_motion_type 
		//mb->motion_type = bitstream_get(2);
	} else if ((mb->macroblock_type & MACROBLOCK_INTRA) && picture->concealment_motion_vectors) {
		// concealment motion vectors 
		//mb->motion_type = (picture_structure==FRAME_PICTURE) ? MC_FRAME : MC_FIELD;
		mb->motion_type = MC_FRAME;
	}

	// derive motion_vector_count, mv_format and dmv, (table 6-17, 6-18)
	//if (picture_structure==FRAME_PICTURE)
	{
		mb->motion_vector_count = (mb->motion_type==MC_FIELD) ? 2 : 1;
		mb->mv_format = (mb->motion_type==MC_FRAME) ? MV_FRAME : MV_FIELD;
	}
#if 0
	else
	{
		mb->motion_vector_count = (mb->motion_type==MC_16X8) ? 2 : 1;
		mb->mv_format = MV_FIELD;
	}
#endif

	mb->dmv = (mb->motion_type==MC_DMV); // dual prime

	//Set if we need to scale motion vector prediction by 1/2
	mb->mvscale = ((mb->mv_format==MV_FIELD) /* && (picture_structure==FRAME_PICTURE) */ );

	// get dct_type (frame DCT / field DCT) 
	if( /* (picture_structure==FRAME_PICTURE) && */ (!picture->frame_pred_frame_dct) && 
			(mb->macroblock_type & (MACROBLOCK_PATTERN|MACROBLOCK_INTRA)) )
		mb->dct_type =  bitstream_get(1);
	else
		mb->dct_type =  0;


	if (mb->macroblock_type & MACROBLOCK_QUANT) {
		quantizer_scale_code = bitstream_get(5);

		//The quantizer scale code propogates up to the slice level
		if(picture->q_scale_type)
			slice->quantizer_scale = non_linear_quantizer_scale[quantizer_scale_code];
		else
			slice->quantizer_scale = quantizer_scale_code << 1;
	}
		

	// 6.3.17.2 Motion vectors 

	//decode forward motion vectors 
	if ((mb->macroblock_type & MACROBLOCK_MOTION_FORWARD) || 
			((mb->macroblock_type & MACROBLOCK_INTRA) && picture->concealment_motion_vectors))
			slice_get_forward_motion_vectors(picture,slice,mb);

	//decode backward motion vectors 
	if (mb->macroblock_type & MACROBLOCK_MOTION_BACKWARD)
			slice_get_backward_motion_vectors(picture,slice,mb);

	if ((mb->macroblock_type & MACROBLOCK_INTRA) && picture->concealment_motion_vectors)
		bitstream_flush(1); // remove marker_bit 

	//6.3.17.4 Coded block pattern 
	mb->coded_block_pattern = 0;
	if (mb->macroblock_type & MACROBLOCK_PATTERN)
		mb->coded_block_pattern = Get_coded_block_pattern();

	if (mb->macroblock_type & MACROBLOCK_INTRA) {
		mb->coded_block_pattern = 0x3f;

		// Decode lum blocks
		slice_get_intra_block(picture,slice,&mb->blocks[0*64],0);
		slice_get_intra_block(picture,slice,&mb->blocks[1*64],0);
		slice_get_intra_block(picture,slice,&mb->blocks[2*64],0);
		slice_get_intra_block(picture,slice,&mb->blocks[3*64],0);
		// Decode chroma blocks
		slice_get_intra_block(picture,slice,&mb->blocks[4*64],1);
		slice_get_intra_block(picture,slice,&mb->blocks[5*64],2);
	}
	//coded_block_pattern is set only if there are blocks in bitstream
	else if(mb->coded_block_pattern) {
		// Decode lum blocks 
		if (mb->coded_block_pattern & 0x20)
			slice_get_non_intra_block(picture,slice,&mb->blocks[0*64]);
		if (mb->coded_block_pattern & 0x10)
			slice_get_non_intra_block(picture,slice,&mb->blocks[1*64]);
		if (mb->coded_block_pattern & 0x08)
			slice_get_non_intra_block(picture,slice,&mb->blocks[2*64]);
		if (mb->coded_block_pattern & 0x04)
			slice_get_non_intra_block(picture,slice,&mb->blocks[3*64]);
		
		// Decode chroma blocks 
		if (mb->coded_block_pattern & 0x2)
			slice_get_non_intra_block(picture,slice,&mb->blocks[4*64]);

		if (mb->coded_block_pattern & 0x1)
			slice_get_non_intra_block(picture,slice,&mb->blocks[5*64]);
	}

	// 7.2.1 DC coefficients in intra blocks 
	if (!(mb->macroblock_type & MACROBLOCK_INTRA)) {
		//FIXME this looks suspicious...should be reset to 2^(intra_dc_precision+7)
		//
		//lets see if it works
		slice->dc_dct_pred[0]=slice->dc_dct_pred[1]=slice->dc_dct_pred[2]= 
			1<<(picture->intra_dc_precision + 7) ;
	}

	//7.6.3.4 Resetting motion vector predictors 
	if ((mb->macroblock_type & MACROBLOCK_INTRA) && !picture->concealment_motion_vectors)
		slice_reset_pmv(slice);


	// special "No_MC" macroblock_type case 
	// 7.6.3.5 Prediction in P pictures 
	if ((picture->picture_coding_type==P_TYPE) 
		&& !(mb->macroblock_type & (MACROBLOCK_MOTION_FORWARD|MACROBLOCK_INTRA))) {
		// non-intra mb without forward mv in a P picture 
		// 7.6.3.4 Resetting motion vector predictors 
		slice_reset_pmv(slice);
		memset(mb->f_motion_vectors[0],0,8);
		mb->macroblock_type |= MACROBLOCK_MOTION_FORWARD;

		//6.3.17.1 Macroblock modes, frame_motion_type 
		// if (picture->picture_structure==FRAME_PICTURE)
			mb->motion_type = MC_FRAME;
#if 0
		else
		{
			mb->motion_type = MC_FIELD;
			// predict from field of same parity 
			mb->f_motion_vertical_field_select[0]= (picture->picture_structure==BOTTOM_FIELD);
		}
#endif
	}
}

void
slice_init(void)
{
}


uint32_t
slice_process (picture_t *picture, uint8_t code, uint8_t *buffer)
{
	uint32_t mba;      
	uint32_t mba_inc;
	uint32_t prev_macroblock_type = 0; 
	uint32_t mb_width = picture->coded_picture_width >> 4;
	uint32_t i;

	mba = (code - 1) * mb_width - 1;

	bitstream_init (buffer);

	slice_reset_pmv(&slice);
	slice_get_slice_header(picture,&slice);
	do {
		mba_inc = Get_macroblock_address_increment();

		if(mba_inc > 1) {
			//FIXME: this should be a function in slice.c instead
			//reset intra dc predictor on skipped block
			slice.dc_dct_pred[0]=slice.dc_dct_pred[1]=slice.dc_dct_pred[2]=
				1<<(picture->intra_dc_precision + 7);

			mb.coded_block_pattern = 0;
			mb.skipped = 1;

			//handling of skipped mb's differs between P_TYPE and B_TYPE
			//pictures
			if(picture->picture_coding_type == P_TYPE) {
				slice_reset_pmv(&slice);
				memset(mb.f_motion_vectors[0],0,8);
				mb.macroblock_type = MACROBLOCK_MOTION_FORWARD;

				for(i=0; i< mba_inc - 1; i++) {
					mb.mba = ++mba;
					motion_comp(picture,&mb);
				}
			} else {
				memcpy(mb.f_motion_vectors[0],slice.f_pmv,8);
				memcpy(mb.b_motion_vectors[0],slice.b_pmv,8);
				mb.macroblock_type = prev_macroblock_type;

				for(i=0; i< mba_inc - 1; i++) {
					mb.mba = ++mba;
					motion_comp(picture,&mb);
				}
			}
			mb.skipped = 0;
		}
		
		mb.mba = ++mba; 

		slice_get_macroblock(picture,&slice,&mb);

		//we store the last macroblock mv flags, as skipped b-frame blocks
		//inherit them
		prev_macroblock_type = mb.macroblock_type & (MACROBLOCK_MOTION_FORWARD | MACROBLOCK_MOTION_BACKWARD);

		idct(&mb);
		motion_comp(picture,&mb);
	} while (bitstream_show(23));

	return (mba >= picture->last_mba);
}
