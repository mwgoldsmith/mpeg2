# # Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.3 foreign 

CFLAGS = -g -O3 -Wall -Werror @LIBVO_CFLAGS@ @GLOBAL_CFLAGS@ @X_CFLAGS@ -idirafter ../compatibility

lib_LTLIBRARIES = libvo.la

include_HEADERS = video_out.h 

libvo_la_LIBADD =  @LIBVO_CONFIG_OBJS@ @LIBVO_CONFIG_LIBS@ @X_PRE_LIBS@ @X_LIBS@ -lXext
libvo_la_DEPENDENCIES = @LIBVO_CONFIG_OBJS@
libvo_la_LDFLAGS = -version-info 1:6:1 

libvo_la_SOURCES =\
	yuv2rgb.c\
	video_out.c\
	video_out_null.c\
	video_out_mga.c\
	video_out_x11.c\
	video_out_3dfx.c\
	video_out_sdl.c \
	video_out_pgm.c

EXTRA_libvo_la_SOURCES = yuv2rgb_mmx.c yuv2rgb_mlib.c

noinst_HEADERS = video_out_internal.h yuv2rgb.h mmx.h
