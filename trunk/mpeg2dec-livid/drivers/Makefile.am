# # Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.3 foreign no-dependencies


EXTRA_DIST = $(EXTRA_mga_vid_SOURCES) $(EXTRA_mga_vid_test_SOURCES)

EXTRA_mga_vid_SOURCES = mga_vid.c
EXTRA_mga_vid_test_SOURCES = mga_vid_test.c

INCLUDES = $(KERNEL_INCLUDES)
CFLAGS = -g -O2  -Wall -D__KERNEL__  -DMODULE @MOD_CFLAGS@\
	 -include /usr/src/linux/include/linux/modversions.h


noinst_HEADERS = mga_vid.h

if BUILD_MGA_DRIVER
all: mga_vid.o mga_vid_test
endif

mga_vid.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $(basename $@).c


mga_vid_test:
	$(CC) -g -O -Wall $(INCLUDES) -o $@ $@.c



clean: 
	rm -f *.o
	rm -f mga_vid_test mga_vid_test


mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#*


maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in
