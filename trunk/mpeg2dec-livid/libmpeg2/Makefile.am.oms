##
## Process this file with automake to produce Makefile.in
##

INCLUDES = -I$(top_srcdir)/include
CFLAGS = @CFLAGS@ @LIBMPEG2_CFLAGS@

libdir = $(PLUGIN_DIR)


## Libs
lib_LTLIBRARIES = dec_mpeg2.la

include_HEADERS = mpeg2.h 

dec_mpeg2_la_LIBADD = @LIBMPEG2_LIBS@
dec_mpeg2_la_LDFLAGS = -avoid-version -module Wl,-soname,dec_mpeg2 -L$(top_builddir)/src/oms/.libs -loms

dec_mpeg2_la_SOURCES = dec_mpeg2.c decode.c header.c stats.c slice.c \
		      motion_comp.c motion_comp_mlib.c motion_comp_mmx.c \
		      idct.c idct_mlib.c idct_mmx.c
