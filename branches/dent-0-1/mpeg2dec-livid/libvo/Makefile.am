##
## Process this file with automake to produce Makefile.in
##

AUTOMAKE_OPTIONS = no-dependencies foreign


DEFS = $(CFLAGS) @DEFS@ @X_CFLAGS@ #How to make this the correct way?
INCLUDES = -I$(top_srcdir)/include/oms -I$(top_srcdir)/include


libdir = $(OMS_PLUGINDIR)


## Libs
lib_LTLIBRARIES = display_null.la $(LIBS_VIDEO_OUTPUT)
EXTRA_LTLIBRARIES = display_x11.la display_3dfx.la display_mga.la display_sdl.la 

EXTRA_display_x11_la_SOURCES = yuv2rgb_mmx.c yuv2rgb_mlib.c
display_x11_la_SOURCES = video_out_x11.c yuv2rgb.c
display_x11_la_LIBADD = @YUV_ACCEL_OBJS@
display_x11_la_DEPENDENCIES = @YUV_ACCEL_OBJS@
display_x11_la_CFLAGS = $(CFLAGS) @DEFS@ @X_CFLAGS@
display_x11_la_LDFLAGS = -rpath $(libdir) -avoid-version -module -Wl,-soname,display_x11 @X_PRE_LIBS@ -X11 -lXext @X_LIBS@ @X_EXTRA_LIBS@ @YUV_ACCEL_LIBS@

display_3dfx_la_SOURCES = video_out_3dfx.c
display_3dfx_la_LDFLAGS = -rpath $(libdir) -avoid-version -module -Wl,-soname,display_3dfx

display_mga_la_SOURCES = video_out_mga.c
display_mga_la_LDFLAGS = -rpath $(libdir) -avoid-version -module -Wl,-soname,display_mga

display_sdl_la_SOURCES = video_out_sdl.c
display_sdl_la_LDFLAGS = -lSDL -rpath $(libdir) -avoid-version -module -Wl,-soname,display_sdl

display_null_la_SOURCES = video_out_null.c
display_null_la_LDFLAGS = -avoid-version -module -Wl,-soname,display_null


noinst_HEADERS = video_out_internal.h mmx.h
