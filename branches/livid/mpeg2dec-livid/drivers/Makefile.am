EXTRA_DIST = mga_vid.c mga_vid_test.c mga_vid.h

CFLAGS = -Werror -g -O2  -Wall -D__KERNEL__  -DMODULE \
	 -include /usr/src/linux/include/linux/modversions.h

SMP_CFLAGS = -Werror -g -O2  -Wall -D__SMP__ -D__KERNEL__  -DMODULE \
	 -include /usr/src/linux/include/linux/modversions.h


all: mga_vid.o mga_vid_test_g400

CC = gcc

mga_vid.o : mga_vid.c mga_vid.h
	gcc $(CFLAGS) -c  mga_vid.c

mga_vid_test_g400: mga_vid_test.c mga_vid.h
	gcc -g -O -Wall -o mga_vid_test_g400 mga_vid_test.c
	-rm -f mga_vid_test_g200
	ln -s mga_vid_test_g400 mga_vid_test_g200
	
clean: 
	rm -f *.o
	rm -f mga_vid_test_g?00
