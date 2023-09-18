TARGET=ewriter
SRC=$(wildcard *.c)
OBJS=${SRC:%.c=%.o}


.PHONY: all install clean

all: ${TARGET}

${TARGET}: ${OBJS}
	${CC} -o $@ ${OBJS} ${LDFLAGS}

%.o: %.c
	${CC} ${CFLAGS} -c $<

clean:
	rm -rf ${TARGET} *.o
