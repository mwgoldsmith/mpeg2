SUBDIRS = convert

AM_CFLAGS = $(OPT_CFLAGS) $(LIBMPEG2_CFLAGS)

lib_LTLIBRARIES = libmpeg2.la
libmpeg2_la_SOURCES = alloc.c header.c decode.c slice.c motion_comp.c idct.c
libmpeg2_la_LIBADD = libmpeg2arch.la
libmpeg2_la_LDFLAGS = -no-undefined -version-info 1:0:1

noinst_LTLIBRARIES = libmpeg2arch.la

if ARCH_ARM
libmpeg2arch_la_SOURCES += arm/motion_comp_arm.c arm/motion_comp_arm_s.S
else
if ARCH_X86
libmpeg2arch_la_SOURCES += x86/motion_comp_mmx.c x86/idct_mmx.c x86/cpu_state_x86.c x86/cpu_accel_x86.c
else
if ARCH_ALPHA
libmpeg2arch_la_SOURCES += alpha/motion_comp_alpha.c alpha/idct_alpha.c alpha/cpu_accel_alpha.c
else
if ARCH_SPARC
libmpeg2arch_la_SOURCES += sparc/motion_comp_vis.c sparc/cpu_accel_sparc.c
else
if ARCH_PPC
libmpeg2arch_la_SOURCES += ppc/motion_comp_altivec.c ppc/idct_altivec.c ppc/cpu_state_ppc.c ppc/cpu_accel_ppc.c
endif
endif
endif
endif
endif

libmpeg2arch_la_CFLAGS = $(OPT_CFLAGS) $(ARCH_OPT_CFLAGS) $(LIBMPEG2_CFLAGS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmpeg2.pc

EXTRA_DIST = vlc.h mpeg2_internal.h
