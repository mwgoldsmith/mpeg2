/* getvlc.c, variable length decoding */

/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

#include <stdio.h>

#include "mpeg2.h"
#include "mpeg2_internal.h"
#include "config.h"
#include "bitstream.h"
#include "getvlc.h"

int Fault_Flag = 0;
int Quiet_Flag = 0;

static int Get_I_macroblock_type (void)
{
    if (bitstream_get (1))
	return MACROBLOCK_INTRA;

    if (!bitstream_get (1)) {
	if (!Quiet_Flag)
	    printf ("Invalid macroblock_type code\n");
	Fault_Flag = 1;
    }

    return MACROBLOCK_QUANT|MACROBLOCK_INTRA;
}

static int Get_P_macroblock_type (void)
{
    int code;

    code = bitstream_show (6);

    if (code >= 8) {
	code >>= 3;
	bitstream_flush (PMBtab0[code].len);
	return PMBtab0[code].val;
    }

    if (code == 0) {
	if (!Quiet_Flag)
	    printf ("Invalid macroblock_type code\n");
	Fault_Flag = 1;
	return 0;
    }

    bitstream_flush (PMBtab1[code].len);
    return PMBtab1[code].val;
}

static int Get_B_macroblock_type (void)
{
    int code;

    code = bitstream_show (6);

    if (code >= 8) {
	code >>= 2;
	bitstream_flush (BMBtab0[code].len);
	return BMBtab0[code].val;
    }

    if (code == 0) {
	if (!Quiet_Flag)
	    printf ("Invalid macroblock_type code\n");
	Fault_Flag = 1;
	return 0;
    }

    bitstream_flush (BMBtab1[code].len);
    return BMBtab1[code].val;
}

static int Get_D_macroblock_type (void)
{
    if (!bitstream_get (1)) {
	if (!Quiet_Flag)
	    printf ("Invalid macroblock_type code\n");
	Fault_Flag = 1;
    }

    return MACROBLOCK_INTRA;
}

int Get_macroblock_type (int picture_coding_type)
{
    int macroblock_type = 0;

    switch (picture_coding_type) {
    case I_TYPE:
	macroblock_type = Get_I_macroblock_type ();
	break;
    case P_TYPE:
	macroblock_type = Get_P_macroblock_type ();
	break;
    case B_TYPE:
	macroblock_type = Get_B_macroblock_type ();
	break;
    case D_TYPE:
	macroblock_type = Get_D_macroblock_type ();
	break;
    default:
	printf ("Get_macroblock_type (): unrecognized picture coding type\n");
	break;
    }

    return macroblock_type;
}

int Get_motion_code (void)
{
    int code;

    if (bitstream_get (1))
	return 0;

    code = bitstream_show (9);

    if (code >= 64) {
	code >>= 6;
	bitstream_flush (MVtab0[code].len);
	return bitstream_get (1)?-MVtab0[code].val:MVtab0[code].val;
    }

    if (code >= 24) {
	code >>= 3;
	bitstream_flush (MVtab1[code].len);
	return bitstream_get (1)?-MVtab1[code].val:MVtab1[code].val;
    }

    code -= 12;

    if (code < 0) {
	Fault_Flag = 1;
	return 0;
    }

    bitstream_flush (MVtab2[code].len);

    return bitstream_get (1) ? -MVtab2[code].val : MVtab2[code].val;
}

/* get differential motion vector (for dual prime prediction) */
int Get_dmvector (void)
{
    if (bitstream_get (1))
	return bitstream_get (1) ? -1 : 1;
    else
	return 0;
}

int Get_coded_block_pattern (void)
{
    int code;

    code = bitstream_show (9);

    if (code >= 128) {
	code >>= 4;
	bitstream_flush (CBPtab0[code].len);
	return CBPtab0[code].val;
    }

    if (code >= 8) {
	code >>= 1;
	bitstream_flush (CBPtab1[code].len);
	return CBPtab1[code].val;
    }

    if (code == 0) {
	if (!Quiet_Flag)
	    printf ("Invalid coded_block_pattern code\n");
	Fault_Flag = 1;
	return 0;
    }

    bitstream_flush (CBPtab2[code].len);
    return CBPtab2[code].val;
}

int Get_macroblock_address_increment (void)
{
    int code, val;

    val = 0;

    while ((code = bitstream_show (11)) < 24) {
	if (code!=15) {	/* if not macroblock_stuffing */
	    if (code==8) /* if macroblock_escape */
		val += 33;
	    else {
		if (!Quiet_Flag)
		    printf ("Invalid macroblock_address_increment code\n");

		Fault_Flag = 1;
		return 1;
	    }
	}

	bitstream_flush (11);
	needbits ();
    }

    /* macroblock_address_increment == 1 */
    /* ('1' is in the MSB position of the lookahead) */
    if (code >= 1024) {
	bitstream_flush (1);
	return val + 1;
    }

    /* codes 00010 ... 011xx */
    if (code>=128) {
	/* remove leading zeros */
	code >>= 6;
	bitstream_flush (MBAtab1[code].len);
	return val + MBAtab1[code].val;
    }

    /* codes 00000011000 ... 0000111xxxx */
    code -= 24; /* remove common base */
    bitstream_flush (MBAtab2[code].len);
    return val + MBAtab2[code].val;
}

/* combined MPEG-1 and MPEG-2 stage. parse VLC and 
   perform dct_diff arithmetic.

   MPEG-1: ISO/IEC 11172-2 section
   MPEG-2: ISO/IEC 13818-2 section 7.2.1 
 
   Note: the arithmetic here is presented more elegantly than
   the spec, yet the results, dct_diff, are the same.
*/

int Get_Luma_DC_dct_diff (void)
{
    int code, size, dct_diff;

    /* decode length */
    code = bitstream_show (5);

    if (code < 31) {
	size = DClumtab0[code].val;
	bitstream_flush (DClumtab0[code].len);
    } else {
	code = bitstream_show (9) - 0x1f0;
	size = DClumtab1[code].val;
	bitstream_flush (DClumtab1[code].len);
    }

    if (size == 0)
	dct_diff = 0;
    else {
	dct_diff = bitstream_get (size);
	if ((dct_diff & (1<< (size-1))) == 0)
	    dct_diff -= (1<<size) - 1;
    }

    return dct_diff;
}

int Get_Chroma_DC_dct_diff (void)
{
    int code, size, dct_diff;

    /* decode length */
    code = bitstream_show (5);

    if (code < 31) {
	size = DCchromtab0[code].val;
	bitstream_flush (DCchromtab0[code].len);
    } else {
	code = bitstream_show (10) - 0x3e0;
	size = DCchromtab1[code].val;
	bitstream_flush (DCchromtab1[code].len);
    }

    if (size == 0)
	dct_diff = 0;
    else {
	dct_diff = bitstream_get (size);
	if ((dct_diff & (1<< (size-1)))==0)
	    dct_diff-= (1<<size) - 1;
    }

    return dct_diff;
}
