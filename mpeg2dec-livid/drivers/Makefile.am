EXTRA_DIST = mga_vid.c mga_vid_test.c mga_vid.h

CFLAGS = -Werror -g -O2  -Wall -D__KERNEL__  -DMODULE @MOD_CFLAGS@\
	 -include /usr/src/linux/include/linux/modversions.h

all: mga_vid.o mga_vid_test

CC = gcc

mga_vid.o : mga_vid.c mga_vid.h
	gcc $(CFLAGS) -c  mga_vid.c

mga_vid_test: mga_vid_test.c mga_vid.h
	gcc -g -O -Wall -o mga_vid_test mga_vid_test.c
	
clean: 
	rm -f *.o
	rm -f mga_vid_test
