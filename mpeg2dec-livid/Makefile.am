# # Process this file with automake to produce Makefile.in.
AUTOMAKE_OPTIONS = 1.3 foreign

CFLAGS = -Wall -O3 -g @CFLAGS@ @X_CFLAGS@

bin_PROGRAMS = mpeg2dec
noinst_LIBRARIES = libmpeg2.a

libmpeg2_a_LIBADD = @LIB_CONFIG_OBJS@
libmpeg2_a_SOURCES = getvlc.c  mb_buffer.c motion.c parse.c stats.c\
	idct.c motion_comp.c inv_quantize.c decode.c bitstream.c debug.c
libmpeg2_a_DEPENDENCIES = @LIB_CONFIG_OBJS@

mpeg2dec_LDADD= libmpeg2.a @CONFIG_OBJS@ @X_PRE_LIBS@ @X_LIBS@ -lXext -lX11\
	@X_EXTRA_LIBS@ -lm
mpeg2dec_SOURCES = mpeg2dec.c
mpeg2dec_DEPENDENCIES = libmpeg2.a @CONFIG_OBJS@

EXTRA_mpeg2dec_SOURCES = display_gatos.c display_mga_vid.c display_x11.c\
	display_xil.c idct_mmx.c idct_block_mmx.S idct_mlib.c\
	motion_comp_mmx.c yuv2rgb.c yuv2rgb_mlib.c

noinst_HEADERS = config.h getvlc.h stats.h parse.h slice.h mb_buffer.h\
	motion_comp.h inv_quantize.h mpeg2.h mpeg2_internal.h display.h idct.h\
	old_config.h global.h old_crap.h mpeg2dec.h debug.h bitstream.h\
	idct_mmx.h idct_mlib.h motion_comp_mmx.h yuv2rgb.h yuv2rgb_mlib.h

EXTRA_DIST = README Changelog 

prof:
	$(MAKE) mpeg2dec CFLAGS="-pg $(CFLAGS)"

x11:
	$(MAKE) -e mpeg2dec mpeg2dec_LDADD="display_x11.o idct_block_mmx.S motion_comp_mmx.c idct_mmx.c"

SUBDIRS=tools @DRIVERS@ .
