##
## Process this file with automake to produce Makefile.in
##

libdir = $(PLUGIN_DIR)


## Libs
lib_LTLIBRARIES = codec_mpeg2dec.la

include_HEADERS = mpeg2.h 


################# FIX THE CONDITIONAL BUILD !!!
LIBMPEG2_OBJS=

if HAVE_MLIB
	LIBMPEG2_OBJS="idct_mlib.lo motion_comp_mlib.lo"
endif

if HAVE_MMX
	LIBMPEG2_OBJS="idct_mmx.lo idct_block_mmx.lo motion_comp_mmx.lo"
endif

#codec_mpeg2dec_la_LIBADD = $(LIBMPEG2_OBJS)
#codec_mpeg2dec_la_DEPENDENCIES = $(LIBMPEG2_OBJS)
codec_mpeg2dec_la_LIBADD = idct_mmx.lo idct_block_mmx.lo motion_comp_mmx.lo
codec_mpeg2dec_la_DEPENDENCIES = idct_mmx.lo idct_block_mmx.lo motion_comp_mmx.lo
codec_mpeg2dec_la_CFLAGS = @MM_CFLAGS@
codec_mpeg2dec_la_LDFLAGS = -avoid-version -module Wl,-soname,codec_mpeg2dec @MM_LIBS@

codec_mpeg2dec_la_SOURCES = getvlc.c slice.c header.c stats.c idct.c motion_comp.c motion_comp_c.c codec.c bitstream.c debug.c

EXTRA_codec_mpeg2dec_la_SOURCES = idct_mmx.c idct_block_mmx.S idct_mlib.c motion_comp_mmx.c motion_comp_mlib.c 

noinst_HEADERS = mpeg2.h getvlc.h stats.h header.h slice.h motion_comp.h mpeg2_internal.h idct.h debug.h bitstream.h idct_mmx.h idct_mlib.h mmx.h
