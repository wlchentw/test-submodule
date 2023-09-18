TARGET=liboptee_meta_drmkeyinstall.so
SRC=$(wildcard *.c)
OBJS=${SRC:%.c=%.o}
LDFLAGS += -lteec

OUT_DIR := $(OO)/libsks

CC_FLAGS := -Wall -Wno-unused-parameter -Werror -fPIC -O2

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
	mkdir -p $(INCDIR)/libdrmkey
	cp -f ${CURDIR}/keyblock.h $(INCDIR)/libdrmkey/
