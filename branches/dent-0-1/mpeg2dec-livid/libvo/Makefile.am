##
## Process this file with automake to produce Makefile.in
##

AUTOMAKE_OPTIONS = 1.3 foreign no-dependencies


DEFS = $(CFLAGS) @DEFS@ @X_CFLAGS@
INCLUDES = -I$(top_builddir)/include

## Libs
if HAVE_X11
DISPLAY_X11 = display_x11.la
endif

if HAVE_3DFX
DISPLAY_3DFX = display_3dfx.la
endif

if HAVE_MGA
DISPLAY_MGA = display_mga.la
endif

if HAVE_SDL
DISPLAY_SDL = display_sdl.la
endif

lib_LTLIBRARIES = $(DISPLAY_X11) $(DISPLAY_3DFX) $(DISPLAY_MGA) $(DISPLAY_SDL)

display_x11_la_SOURCES = video_out_x11.c yuv2rgb.c
EXTRA_display_x11_la_SOURCES = yuv2rgb_mmx.c
display_x11_la_LIBADD = @X_PRE_LIBS@ @X_LIBS@ -lXext
display_x11_la_LDFLAGS = -avoid-version -module -Wl,-soname,display_x11

display_3dfx_la_SOURCES = video_out_3dfx.c
display_3dfx_la_LDFLAGS = -avoid-version -module -Wl,-soname,display_3dfx

display_mga_la_SOURCES = video_out_mga.c
display_mga_la_LDFLAGS = -avoid-version -module -Wl,-soname,display_mga

display_sdl_la_SOURCES = video_out_sdl.c
display_sdl_la_LDFLAGS = -avoid-version -module -Wl,-soname,display_sdl


noinst_HEADERS = video_out.h video_out_internal.h mmx.h


mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#*


maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in
