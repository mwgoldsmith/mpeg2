# # Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.3 foreign no-dependencies

SUBDIRS = tools drivers

EXTRA_DIST = Changelog autogen.sh soft_video.c

CFLAGS = -Wall -O3 -g @CFLAGS@ @X_CFLAGS@ -I/usr/src/linux

if HAVE_X
bin_PROGRAMS = mpeg2dec
endif

mpeg2dec_SOURCES = mpeg2dec.c
mpeg2dec_DEPENDENCIES = libmpeg2.la @CONFIG_OBJS@
mpeg2dec_LDADD= libmpeg2.la @CONFIG_OBJS@ @X_PRE_LIBS@ @X_LIBS@ -lXext\
	-L/usr/X11R6/lib -lX11 @X_EXTRA_LIBS@ -lm

EXTRA_mpeg2dec_SOURCES = display_gatos.c display_mga_vid.c display_x11.c\
	display_xil.c idct_mmx.c idct_block_mmx.S idct_mlib.c\
	motion_comp_mmx.c motion_comp_mlib.c yuv2rgb.c yuv2rgb_mlib.c

if HAVE_X
lib_LTLIBRARIES = libmpeg2.la
endif

libmpeg2_la_LIBADD = @LIB_CONFIG_OBJS@
libmpeg2_la_SOURCES = getvlc.c  mb_buffer.c motion.c parse.c stats.c\
	idct.c motion_comp.c inv_quantize.c decode.c bitstream.c debug.c
libmpeg2_la_DEPENDENCIES = @LIB_CONFIG_OBJS@
libmpeg2_la_LDFLAGS = -version-info 1:6:1


noinst_HEADERS = config.h getvlc.h stats.h parse.h slice.h mb_buffer.h\
	motion_comp.h inv_quantize.h mpeg2.h mpeg2_internal.h display.h idct.h\
	old_config.h global.h old_crap.h mpeg2dec.h debug.h bitstream.h\
	idct_mmx.h idct_mlib.h motion_comp_mmx.h motion_comp_mlib.h\
	yuv2rgb.h yuv2rgb_mlib.h


prof:
	$(MAKE) mpeg2dec CFLAGS="-pg $(CFLAGS)"


x11:
	$(MAKE) -e mpeg2dec mpeg2dec_LDADD="display_x11.o idct_block_mmx.S motion_comp_mmx.c idct_mmx.c"


mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#* $(distdir).tar.gz
	-rm -rf $(distdir)


maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in configure aclocal.m4 acinclude.m4
	-rm -f config.h.in stamp-h.in
