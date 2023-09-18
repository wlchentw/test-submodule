srcs-y = memcfg.c

global-incdirs-y = include

cflags-y = -DMEMSIZE=$(MEMSIZE) -DFBSIZE=$(FBSIZE) \
