TARGET=liburee_drmkey.so
SRC=$(wildcard *.c)
OBJS=${SRC:%.c=%.o}

OUT_DIR := $(OO)/libsks

DEFS = -DPLATFORM_MT8532
CC_FLAGS := -Wall -Wno-unused-parameter -Werror -fPIC -O2
CC_FLAGS += $(DEFS)

.PHONY: all install clean

all: ${TARGET}

${TARGET}: ${OBJS}
	${CC} -shared -o $@ ${OBJS} ${LDFLAGS}

%.o: %.c
	${CC} ${CC_FLAGS} -c $<

clean:
	rm -rf ${TARGET} *.o

install:
	cp -f $(TARGET) $(LIBDIR)/$(TARGET)
	mkdir -p $(INCDIR)/libkeyinstall
	cp -f ${CURDIR}/keyinstall.h $(INCDIR)/libkeyinstall/
