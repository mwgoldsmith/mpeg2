AM_CFLAGS = $(MPEG2DEC_CFLAGS)

noinst_PROGRAMS = sample1 sample2 sample3 sample4 sample5 sample6
sample1_SOURCES = sample1.c
sample1_LDADD = $(top_builddir)/libmpeg2/libmpeg2.la
sample2_SOURCES = sample2.c
sample2_LDADD = $(top_builddir)/libmpeg2/libmpeg2.la \
		$(top_builddir)/libvo/libvo.a $(LIBVO_LIBS)
sample3_SOURCES = sample3.c
sample3_LDADD = $(top_builddir)/libmpeg2/libmpeg2.la
sample4_SOURCES = sample4.c
sample4_LDADD = $(top_builddir)/libmpeg2/libmpeg2.la \
		$(top_builddir)/libvo/libvo.a $(LIBVO_LIBS)
sample5_SOURCES = sample5.c
sample5_LDADD = $(top_builddir)/libmpeg2/libmpeg2.la
sample6_SOURCES = sample6.c
sample6_LDADD = $(top_builddir)/libmpeg2/libmpeg2.la \
		$(top_builddir)/libvo/libvo.a $(LIBVO_LIBS)

EXTRA_DIST = libmpeg2.txt libvo.txt
