AM_CFLAGS = $(OPT_CFLAGS) $(LIBMPEG2_CFLAGS)

lib_LTLIBRARIES = libmpeg2.la
libmpeg2_la_SOURCES = alloc.c header.c decode.c slice.c motion_comp.c idct.c \
		      motion_comp_mlib.c idct_mlib.c
libmpeg2_la_LIBADD = libmpeg2arch.la $(LIBMPEG2_LIBS)
libmpeg2_la_LDFLAGS = -no-undefined

noinst_LTLIBRARIES = libmpeg2arch.la
libmpeg2arch_la_SOURCES = motion_comp_mmx.c idct_mmx.c \
			  motion_comp_altivec.c idct_altivec.c \
			  motion_comp_alpha.c idct_alpha.c \
			  cpu_accel.c cpu_state.c
libmpeg2arch_la_CFLAGS = $(OPT_CFLAGS) $(ARCH_OPT_CFLAGS) $(LIBMPEG2_CFLAGS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmpeg2.pc

EXTRA_DIST = configure.incl vlc.h mpeg2_internal.h
