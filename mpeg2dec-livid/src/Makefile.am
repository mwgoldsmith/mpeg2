#FIXME should remove 
INCLUDES 	= 	$(all_includes) \
			-I$(top_srcdir)/libmpeg2 \
			-I$(top_srcdir)/video \
			-I$(top_srcdir)/include


VIDEO_EXTRALIBS         =       ../video/*.lo
lib_LTLIBRARIES		=	libvo.la

libvo_la_SOURCES        =       libvo.c
libvo_la_LDFLAGS        =       $(all_libraries) -module
libvo_la_LIBADD         =       $(VIDEO_EXTRALIBS) \
                                $(SDL_LIBS) $(X_LIBS) \
                                $(GGI_LIBS) $(XV_LIBS) $(DGA_LIBS) \
                                $(MLIB_LIBS) $(X_LIBS)

#
# And now the bin programs
#


bin_PROGRAMS = mpeg2dec extract_mpeg2

mpeg2dec_SOURCES 	= 	mpeg2dec.c cpu_accel.c
mpeg2dec_DEPENDECIES	=	libvo.la
mpeg2dec_LDFLAGS	=	$(all_libraries)
mpeg2dec_LDADD		=	../libmpeg2/libmpeg2.la libvo.la


extract_mpeg2_SOURCES 	= 	extract_mpeg2.c


