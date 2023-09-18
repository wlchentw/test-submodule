TARGET=test_keyinstall
SRC=$(wildcard *.c)
OBJS=${SRC:%.c=%.o}
LDFLAGS += -luree_drmkey -lteec -L ../host

OUT_DIR := $(OO)/libsks

CC_FLAGS := -Wall -Wno-unused-parameter -Werror -fPIC -O2

.PHONY: all install clean

all: ${TARGET}

${TARGET}: ${OBJS}
	${CC} -o $@ ${OBJS} ${LDFLAGS}

%.o: %.c
	${CC} ${CC_FLAGS} -c $<

clean:
	rm -rf ${TARGET} *.o

install:
	cp -f $(TARGET) $(LIBDIR)/$(TARGET)
