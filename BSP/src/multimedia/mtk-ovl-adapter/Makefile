TARGET=libmtk-ovl-adapter.so
SRC=$(wildcard *.c)
OBJS=${SRC:%.c=%.o}

INCLUDES += -I$(PWD)/../gst-plugins-good/sys/v4l2/ext

.PHONY: all clean install

all : ${TARGET}

${TARGET} :  $(OBJS)
	${LD} --hash-style=gnu -shared -fpic -o ${TARGET} $<

%.o: %.c
	${CC} ${CFLAGS} -fPIC -c $< -o $@

clean:
	@rm -fr *.o 

install : ${TARGET}
	install -d ${DESTDIR}${LIBDIR}/
	install -d ${DESTDIR}${LIBDIR}/pkgconfig/
	install -d ${DESTDIR}${INCDIR}/
	install -m 755 ${TARGET} ${DESTDIR}${LIBDIR}/${TARGET}
	install -m 444 mtk_ovl_adapter.h ${DESTDIR}${INCDIR}/
