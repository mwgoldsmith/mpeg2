#FIXME should remove
INCLUDES = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/video \
	-I$(top_srcdir)/src/plugin/output/video

CFLAGS = @CFLAGS@ @LIBMPEG2_CFLAGS@

noinst_LTLIBRARIES = libmpeg2.la
libmpeg2_la_SOURCES = decode.c header.c stats.c slice.c \
		      motion_comp.c motion_comp_mlib.c motion_comp_mmx.c \
		      idct.c idct_mlib.c idct_mmx.c
# the source ordering seems to impact the speed.
# On my setup the following ordering is about 2% faster.
# I'll have to investigate...
# libmpeg2_la_SOURCES = slice.c header.c stats.c idct.c motion_comp.c \
# decode.c idct_mmx.c motion_comp_mmx.c
libmpeg2_la_LIBADD = @LIBMPEG2_LIBS@

#EXTRA_DIST = configure.incl vlc.h mpeg2_internal.h
EXTRA_DIST = configure.incl vlc.h mpeg2_internal.h mpeg2.h
