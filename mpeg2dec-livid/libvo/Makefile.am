# # Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.3 foreign 

CFLAGS = -g -O3 -Wall -Werror @LIBVO_CFLAGS@ @GLOBAL_CFLAGS@ @X_CFLAGS@

#SUBDIRS = kernel_modules

include_HEADER = video_out.h 
lib_LTLIBRARIES = libvo.la

libvo_la_SOURCES = video_out_x11.c yuv2rgb.c video_out_3dfx.c\
	video_out_mga.c  video_out_sdl.c
libvo_la_LIBADD =  @LIBVO_CONFIG_LIBS@ @X_PRE_LIBS@ @X_LIBS@ -lXext

noinst_HEADERS =  video_out_internal.h  yuv2rgb.h 
