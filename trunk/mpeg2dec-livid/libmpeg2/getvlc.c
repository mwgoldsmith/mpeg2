/* getvlc.c, variable length decoding                                       */

/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */

#include <stdio.h>

#include "mpeg2.h"
#include "mpeg2_internal.h"
#include "config.h"
#include "bitstream.h"
#include "getvlc.h"

int Fault_Flag = 0;
int Quiet_Flag = 0;

/* private prototypes */
/* generic picture macroblock type processing functions */
static int Get_I_macroblock_type (void);
static int Get_P_macroblock_type (void);
static int Get_B_macroblock_type (void);
static int Get_D_macroblock_type (void);

int Get_macroblock_type(int picture_coding_type)
{
  int macroblock_type = 0;

	switch (picture_coding_type)
	{
		case I_TYPE:
			macroblock_type =  Get_I_macroblock_type();
			break;
		case P_TYPE:
			macroblock_type = Get_P_macroblock_type();
			break;
		case B_TYPE:
			macroblock_type =  Get_B_macroblock_type();
			break;
		case D_TYPE:
			macroblock_type = Get_D_macroblock_type();
			break;
		default:
			printf("Get_macroblock_type(): unrecognized picture coding type\n");
			break;
	}

  return macroblock_type;
}

static int Get_I_macroblock_type()
{
#ifdef TRACE
  if (Trace_Flag)
    printf("macroblock_type(I) ");
#endif /* TRACE */

  if (bitstream_get(1))
  {
#ifdef TRACE
    if (Trace_Flag)
      printf("(1): Intra (1)\n");
#endif /* TRACE */
    return 1;
  }

  if (!bitstream_get(1))
  {
    if (!Quiet_Flag)
      printf("Invalid macroblock_type code\n");
    Fault_Flag = 1;
  }

#ifdef TRACE
  if (Trace_Flag)
    printf("(01): Intra, Quant (17)\n");
#endif /* TRACE */

  return 17;
}

#ifdef TRACE
static char *MBdescr[]={
  "",                  "Intra",        "No MC, Coded",         "",
  "Bwd, Not Coded",    "",             "Bwd, Coded",           "",
  "Fwd, Not Coded",    "",             "Fwd, Coded",           "",
  "Interp, Not Coded", "",             "Interp, Coded",        "",
  "",                  "Intra, Quant", "No MC, Coded, Quant",  "",
  "",                  "",             "Bwd, Coded, Quant",    "",
  "",                  "",             "Fwd, Coded, Quant",    "",
  "",                  "",             "Interp, Coded, Quant", ""
};
#endif

static int Get_P_macroblock_type()
{
  int code;

#ifdef TRACE
  if (Trace_Flag)
    printf("macroblock_type(P) (");
#endif /* TRACE */

  if ((code = bitstream_show(6))>=8)
  {
    code >>= 3;
    bitstream_flush(PMBtab0[code].len);
#ifdef TRACE
    if (Trace_Flag)
    {
      Print_Bits(code,3,PMBtab0[code].len);
      printf("): %s (%d)\n",MBdescr[(int)PMBtab0[code].val],PMBtab0[code].val);
    }
#endif /* TRACE */
    return PMBtab0[code].val;
  }

  if (code==0)
  {
    if (!Quiet_Flag)
      printf("Invalid macroblock_type code\n");
    Fault_Flag = 1;
    return 0;
  }

  bitstream_flush(PMBtab1[code].len);

#ifdef TRACE
  if (Trace_Flag)
  {
    Print_Bits(code,6,PMBtab1[code].len);
    printf("): %s (%d)\n",MBdescr[(int)PMBtab1[code].val],PMBtab1[code].val);
  }
#endif /* TRACE */

  return PMBtab1[code].val;
}

static int Get_B_macroblock_type()
{
  int code;

#ifdef TRACE
  if (Trace_Flag)
    printf("macroblock_type(B) (");
#endif /* TRACE */

  if ((code = bitstream_show(6))>=8)
  {
    code >>= 2;
    bitstream_flush(BMBtab0[code].len);

#ifdef TRACE
    if (Trace_Flag)
    {
      Print_Bits(code,4,BMBtab0[code].len);
      printf("): %s (%d)\n",MBdescr[(int)BMBtab0[code].val],BMBtab0[code].val);
    }
#endif /* TRACE */

    return BMBtab0[code].val;
  }

  if (code==0)
  {
    if (!Quiet_Flag)
      printf("Invalid macroblock_type code\n");
    Fault_Flag = 1;
    return 0;
  }

  bitstream_flush(BMBtab1[code].len);

#ifdef TRACE
  if (Trace_Flag)
  {
    Print_Bits(code,6,BMBtab1[code].len);
    printf("): %s (%d)\n",MBdescr[(int)BMBtab1[code].val],BMBtab1[code].val);
  }
#endif /* TRACE */

  return BMBtab1[code].val;
}

static int Get_D_macroblock_type()
{
  if (!bitstream_get(1))
  {
    if (!Quiet_Flag)
      printf("Invalid macroblock_type code\n");
    Fault_Flag=1;
  }

  return 1;
}



int Get_motion_code()
{
  int code;

#ifdef TRACE
  if (Trace_Flag)
    printf("motion_code (");
#endif /* TRACE */

  if (bitstream_get(1))
  {
#ifdef TRACE
    if (Trace_Flag)
      printf("0): 0\n");
#endif /* TRACE */
    return 0;
  }

  if ((code = bitstream_show(9))>=64)
  {
    code >>= 6;
    bitstream_flush(MVtab0[code].len);

#ifdef TRACE
    if (Trace_Flag)
    {
      Print_Bits(code,3,MVtab0[code].len);
      printf("%d): %d\n",
        bitstream_show(1),bitstream_show(1)?-MVtab0[code].val:MVtab0[code].val);
    }
#endif /* TRACE */

    return bitstream_get(1)?-MVtab0[code].val:MVtab0[code].val;
  }

  if (code>=24)
  {
    code >>= 3;
    bitstream_flush(MVtab1[code].len);

#ifdef TRACE
    if (Trace_Flag)
    {
      Print_Bits(code,6,MVtab1[code].len);
      printf("%d): %d\n",
        bitstream_show(1),bitstream_show(1)?-MVtab1[code].val:MVtab1[code].val);
    }
#endif /* TRACE */

    return bitstream_get(1)?-MVtab1[code].val:MVtab1[code].val;
  }

  if ((code-=12)<0)
  {
    Fault_Flag=1;
    return 0;
  }

  bitstream_flush(MVtab2[code].len);

#ifdef TRACE
  if (Trace_Flag)
  {
    Print_Bits(code+12,9,MVtab2[code].len);
    printf("%d): %d\n",
      bitstream_show(1),bitstream_show(1)?-MVtab2[code].val:MVtab2[code].val);
  }
#endif /* TRACE */

  return bitstream_get(1) ? -MVtab2[code].val : MVtab2[code].val;
}

/* get differential motion vector (for dual prime prediction) */
int Get_dmvector()
{
#ifdef TRACE
  if (Trace_Flag)
    printf("dmvector (");
#endif /* TRACE */

  if (bitstream_get(1))
  {
#ifdef TRACE
    if (Trace_Flag)
      printf(bitstream_show(1) ? "11): -1\n" : "10): 1\n");
#endif /* TRACE */
    return bitstream_get(1) ? -1 : 1;
  }
  else
  {
#ifdef TRACE
    if (Trace_Flag)
      printf("0): 0\n");
#endif /* TRACE */
    return 0;
  }
}

int Get_coded_block_pattern()
{
  int code;

#ifdef TRACE
  if (Trace_Flag)
    printf("coded_block_pattern_420 (");
#endif /* TRACE */

  if ((code = bitstream_show(9))>=128)
  {
    code >>= 4;
    bitstream_flush(CBPtab0[code].len);

#ifdef TRACE
    if (Trace_Flag)
    {
      Print_Bits(code,5,CBPtab0[code].len);
      printf("): ");
      Print_Bits(CBPtab0[code].val,6,6);
      printf(" (%d)\n",CBPtab0[code].val);
    }
#endif /* TRACE */

    return CBPtab0[code].val;
  }

  if (code>=8)
  {
    code >>= 1;
    bitstream_flush(CBPtab1[code].len);

#ifdef TRACE
    if (Trace_Flag)
    {
      Print_Bits(code,8,CBPtab1[code].len);
      printf("): ");
      Print_Bits(CBPtab1[code].val,6,6);
      printf(" (%d)\n",CBPtab1[code].val);
    }
#endif /* TRACE */

    return CBPtab1[code].val;
  }

  if (code<1)
  {
    if (!Quiet_Flag)
      printf("Invalid coded_block_pattern code\n");
    Fault_Flag = 1;
    return 0;
  }

  bitstream_flush(CBPtab2[code].len);

#ifdef TRACE
  if (Trace_Flag)
  {
    Print_Bits(code,9,CBPtab2[code].len);
    printf("): ");
    Print_Bits(CBPtab2[code].val,6,6);
    printf(" (%d)\n",CBPtab2[code].val);
  }
#endif /* TRACE */

  return CBPtab2[code].val;
}

int Get_macroblock_address_increment()
{
  int code, val;

#ifdef TRACE
  if (Trace_Flag)
    printf("macroblock_address_increment (");
#endif /* TRACE */

  val = 0;

  while ((code = bitstream_show(11))<24)
  {
    if (code!=15) /* if not macroblock_stuffing */
    {
      if (code==8) /* if macroblock_escape */
      {
#ifdef TRACE
        if (Trace_Flag)
          printf("00000001000 ");
#endif /* TRACE */

        val+= 33;
      }
      else
      {
        if (!Quiet_Flag)
          printf("Invalid macroblock_address_increment code\n");

        Fault_Flag = 1;
        return 1;
      }
    }
    else /* macroblock suffing */
    {
#ifdef TRACE
      if (Trace_Flag)
        printf("00000001111 ");
#endif /* TRACE */
    }

    bitstream_flush(11);
  }

  /* macroblock_address_increment == 1 */
  /* ('1' is in the MSB position of the lookahead) */
  if (code>=1024)
  {
    bitstream_flush(1);
#ifdef TRACE
    if (Trace_Flag)
      printf("1): %d\n",val+1);
#endif /* TRACE */
    return val + 1;
  }

  /* codes 00010 ... 011xx */
  if (code>=128)
  {
    /* remove leading zeros */
    code >>= 6;
    bitstream_flush(MBAtab1[code].len);

#ifdef TRACE
    if (Trace_Flag)
    {
      Print_Bits(code,5,MBAtab1[code].len);
      printf("): %d\n",val+MBAtab1[code].val);
    }
#endif /* TRACE */

    
    return val + MBAtab1[code].val;
  }
  
  /* codes 00000011000 ... 0000111xxxx */
  code-= 24; /* remove common base */
  bitstream_flush(MBAtab2[code].len);

#ifdef TRACE
  if (Trace_Flag)
  {
    Print_Bits(code+24,11,MBAtab2[code].len);
    printf("): %d\n",val+MBAtab2[code].val);
  }
#endif /* TRACE */

  return val + MBAtab2[code].val;
}

/* combined MPEG-1 and MPEG-2 stage. parse VLC and 
   perform dct_diff arithmetic.

   MPEG-1:  ISO/IEC 11172-2 section
   MPEG-2:  ISO/IEC 13818-2 section 7.2.1 
   
   Note: the arithmetic here is presented more elegantly than
   the spec, yet the results, dct_diff, are the same.
*/

int Get_Luma_DC_dct_diff()
{
  int code, size, dct_diff;

#ifdef TRACE
/*
  if (Trace_Flag)
    printf("dct_dc_size_luminance: (");
*/
#endif /* TRACE */

  /* decode length */
  code = bitstream_show(5);

  if (code<31)
  {
    size = DClumtab0[code].val;
    bitstream_flush(DClumtab0[code].len);
#ifdef TRACE
/*
    if (Trace_Flag)
    {
      Print_Bits(code,5,DClumtab0[code].len);
      printf("): %d",size);
    }
*/
#endif /* TRACE */
  }
  else
  {
    code = bitstream_show(9) - 0x1f0;
    size = DClumtab1[code].val;
    bitstream_flush(DClumtab1[code].len);

#ifdef TRACE
/*
    if (Trace_Flag)
    {
      Print_Bits(code+0x1f0,9,DClumtab1[code].len);
      printf("): %d",size);
    }
*/
#endif /* TRACE */
  }

#ifdef TRACE
/*
  if (Trace_Flag)
    printf(", dct_dc_differential (");
*/
#endif /* TRACE */

  if (size==0)
    dct_diff = 0;
  else
  {
    dct_diff = bitstream_get(size);
#ifdef TRACE
/*
    if (Trace_Flag)
      Print_Bits(dct_diff,size,size);
*/
#endif /* TRACE */
    if ((dct_diff & (1<<(size-1)))==0)
      dct_diff-= (1<<size) - 1;
  }

#ifdef TRACE
/*
  if (Trace_Flag)
    printf("): %d\n",dct_diff);
*/
#endif /* TRACE */

  return dct_diff;
}


int Get_Chroma_DC_dct_diff()
{
  int code, size, dct_diff;

#ifdef TRACE
/*
  if (Trace_Flag)
    printf("dct_dc_size_chrominance: (");
*/
#endif /* TRACE */

  /* decode length */
  code = bitstream_show(5);

  if (code<31)
  {
    size = DCchromtab0[code].val;
    bitstream_flush(DCchromtab0[code].len);

#ifdef TRACE
/*
    if (Trace_Flag)
    {
      Print_Bits(code,5,DCchromtab0[code].len);
      printf("): %d",size);
    }
*/
#endif /* TRACE */
  }
  else
  {
    code = bitstream_show(10) - 0x3e0;
    size = DCchromtab1[code].val;
    bitstream_flush(DCchromtab1[code].len);

#ifdef TRACE
/*
    if (Trace_Flag)
    {
      Print_Bits(code+0x3e0,10,DCchromtab1[code].len);
      printf("): %d",size);
    }
*/
#endif /* TRACE */
  }

#ifdef TRACE
/* 
  if (Trace_Flag)
    printf(", dct_dc_differential (");
*/
#endif /* TRACE */

  if (size==0)
    dct_diff = 0;
  else
  {
    dct_diff = bitstream_get(size);
#ifdef TRACE
/*
    if (Trace_Flag)
      Print_Bits(dct_diff,size,size);
*/
#endif /* TRACE */
    if ((dct_diff & (1<<(size-1)))==0)
      dct_diff-= (1<<size) - 1;
  }

#ifdef TRACE
/*
  if (Trace_Flag)
    printf("): %d\n",dct_diff);
*/
#endif /* TRACE */

  return dct_diff;
}
