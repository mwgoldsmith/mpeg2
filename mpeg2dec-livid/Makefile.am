# # Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.3 foreign no-dependencies

CFLAGS = -g -O3 -Wall -Werror @ROOT_CFLAGS@ @GLOBAL_CFLAGS@ @X_CFLAGS@ -Ilibvo

SUBDIRS = libmpeg2 libvo tools drivers .

EXTRA_DIST = Changelog autogen.sh

bin_PROGRAMS = mpeg2dec

mpeg2dec_SOURCES = mpeg2dec.c
mpeg2dec_DEPENDENCIES = libmpeg2/libmpeg2.la libvo/libvo.la 
mpeg2dec_LDADD=  libmpeg2/libmpeg2.la libvo/libvo.la @X_PRE_LIBS@ @X_LIBS@ -lXext\
	-L/usr/X11R6/lib -lX11 @X_EXTRA_LIBS@ -lm

noinst_HEADERS =  config.h 
