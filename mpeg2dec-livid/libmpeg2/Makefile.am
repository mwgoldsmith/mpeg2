
CFLAGS = -g -O3 -Wall -Werror @LIBMPEG2_CFLAGS@ @GLOBAL_CFLAGS@

lib_LTLIBRARIES = libmpeg2.la
lib_LIBRARIES = libmpeg2.a

noinst_HEADERS = config.h getvlc.h stats.h header.h slice.h motion_comp.h \
	mpeg2.h mpeg2_internal.h idct.h\
	mpeg2dec.h debug.h bitstream.h idct_mmx.h idct_mlib.h motion_comp_mmx.h\
	motion_comp_mlib.h mmx.h

EXTRA_libmpeg2_a_SOURCES = idct_mmx.c idct_block_mmx.S idct_mlib.c\
	motion_comp_mmx.c motion_comp_mlib.c 

libmpeg2_a_LIBADD = @LIB_CONFIG_OBJS@
libmpeg2_a_SOURCES = getvlc.c slice.c header.c stats.c idct.c motion_comp.c\
	motion_comp_c.c decode.c bitstream.c debug.c
libmpeg2_a_DEPENDENCIES = @LIB_CONFIG_OBJS@

libmpeg2_la_LIBADD = @LIB_CONFIG_OBJS@
libmpeg2_la_SOURCES = libmpeg2_a_SOURCES
libmpeg2_la_LDFLAGS = -version-info 1:6:1
libmpeg2_la_DEPENDENCIES = @LIB_CONFIG_OBJS@
