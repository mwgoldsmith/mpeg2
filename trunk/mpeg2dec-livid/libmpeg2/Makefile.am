CFLAGS = -g -O3 -Wall  -Werror @LIBMPEG2_CFLAGS@ @GLOBAL_CFLAGS@

lib_LTLIBRARIES = libmpeg2.la

include_HEADER = mpeg2.h 

libmpeg2_la_LIBADD = @LIBMPEG2_CONFIG_OBJS@ 
libmpeg2_la_DEPENDENCIES = @LIBMPEG2_CONFIG_OBJS@
libmpeg2_la_LDFLAGS = -version-info 1:6:1 -export-symbols libmpeg2.sym

libmpeg2_la_SOURCES = getvlc.c slice.c header.c stats.c idct.c motion_comp.c\
	motion_comp_c.c decode.c bitstream.c debug.c

EXTRA_libmpeg2_la_SOURCES = idct_mmx.c idct_block_mmx.S idct_mlib.c\
	motion_comp_mmx.c motion_comp_mlib.c 

noinst_HEADERS = mpeg2.h getvlc.h stats.h header.h slice.h motion_comp.h\
	mpeg2_internal.h idct.h debug.h bitstream.h idct_mmx.h idct_mlib.h mmx.h
