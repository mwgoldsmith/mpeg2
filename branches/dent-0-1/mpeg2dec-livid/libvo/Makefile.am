##
## Process this file with automake to produce Makefile.in
##

AUTOMAKE_OPTIONS = no-dependencies foreign


DEFS = $(CFLAGS) @DEFS@
INCLUDES = -I$(top_builddir)/include/oms -I$(top_builddir)/include
libdir = $(OMS_PLUGINDIR)


## Libs
lib_LTLIBRARIES = display_null.la $(LIBS_VIDEO_OUTPUT)
EXTRA_lib_LTLIBRARIES = display_x11.la display_3dfx.la display_mga.la display_sdl.la 
display_x11_la_SOURCES = video_out_x11.c yuv2rgb.c
display_x11_la_DEPENDENCIES = @X11_CONFIG_OBJS@
display_x11_la_CFLAGS = $(CFLAGS) @DEFS@ @X_CFLAGS@
display_x11_la_LIBADD = @X11_CONFIG_OBJS@ @X_PRE_LIBS@ @X_LIBS@ -lXext
display_x11_la_LDFLAGS = -avoid-version -module -Wl,-soname,display_x11
EXTRA_display_x11_la_SOURCES = yuv2rgb_mmx.c yuv2rgb_mlib.c

display_3dfx_la_SOURCES = video_out_3dfx.c
display_3dfx_la_LDFLAGS = -avoid-version -module -Wl,-soname,display_3dfx

display_mga_la_SOURCES = video_out_mga.c
display_mga_la_LDFLAGS = -avoid-version -module -Wl,-soname,display_mga

display_sdl_la_SOURCES = video_out_sdl.c
display_sdl_la_LDFLAGS = -avoid-version -module -Wl,-soname,display_sdl

display_null_la_SOURCES = video_out_null.c
display_null_la_LDFLAGS = -avoid-version -module -Wl,-soname,display_null


noinst_HEADERS = video_out_internal.h mmx.h


mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#*


maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in
