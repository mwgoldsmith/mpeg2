CFLAGS = -g -O3 -Wall  -Werror @LIBMPEG2_CFLAGS@ @GLOBAL_CFLAGS@ -idirafter ../compatibility

lib_LTLIBRARIES = libmpeg2.la

include_HEADERS = mpeg2.h 

libmpeg2_la_LIBADD = @LIBMPEG2_CONFIG_OBJS@ 
libmpeg2_la_DEPENDENCIES = @LIBMPEG2_CONFIG_OBJS@
libmpeg2_la_LDFLAGS = -version-info 1:6:1 -export-symbols libmpeg2.sym @LIBMPEG2_LDFLAGS@

libmpeg2_la_SOURCES = slice.c header.c stats.c idct.c motion_comp.c\
	decode.c debug.c

EXTRA_libmpeg2_la_SOURCES = idct_mmx.c idct_mlib.c\
	motion_comp_mmx.c motion_comp_mlib.c libmpeg2.sym 

noinst_HEADERS = mpeg2.h stats.h header.h slice.h motion_comp.h\
	mpeg2_internal.h idct.h debug.h vlc.h idct_mmx.h idct_mlib.h mmx.h
