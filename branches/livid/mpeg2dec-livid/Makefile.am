# # Process this file with automake to produce Makefile.in.
AUTOMAKE_OPTIONS = 1.3 foreign

CFLAGS = -Wall -O3 -g 

bin_PROGRAMS = mpeg2dec

mpeg2dec_LIBS= 
mpeg2dec_LDADD= -L/usr/X11R6/lib -lXext -lX11 -lm

mpeg2dec_SOURCES = display.c getvlc.c  mb_buffer.c motion.c \
	parse.c stats.c idct.c motion_comp.c inv_quantize.c decode.c\
	bitstream.c debug.c

noinst_HEADERS = config.h getvlc.h stats.h parse.h slice.h mb_buffer.h\
	motion_comp.h inv_quantize.h mpeg2.h mpeg2_internal.h display.h idct.h\
	old_config.h global.h old_crap.h mpeg2dec.h debug.h bitstream.h

EXTRA_DIST = README Changelog display_mga_vid.c display_x11.c

prof:
	$(MAKE) mpeg2dec CFLAGS="-g $(CFLAGS)"

SUBDIRS=tools drivers .
