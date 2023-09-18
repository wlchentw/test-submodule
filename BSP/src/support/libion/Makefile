TARGET=libion.so
SRC=ion.c
OBJS=${SRC:%.c=%.o}

.PHONY: all clean install

all : ${TARGET}

${TARGET} :  $(OBJS)
	${CC} -shared -fpic -o ${TARGET} $<

%.o: %.c
	${CC} -fPIC -I./include -I./kernel-headers -c $< -o $@

clean:
	${RM} -r *.o

install:
	install -d ${DESTDIR}${LIBDIR}
	install -d ${DESTDIR}${INCDIR}/
	install -d ${DESTDIR}${INCDIR}/ion/
	install -m 644 ${SRCDIR}/libion/libion.so ${DESTDIR}${LIBDIR}
	install -m 444 ${SRCDIR}/libion/include/ion/ion.h ${DESTDIR}${INCDIR}/ion/ion.h
	install -d ${DESTDIR}${INCDIR}/linux
	install -m 444 ${SRCDIR}/libion/kernel-headers/linux/ion.h ${DESTDIR}${INCDIR}/linux/ion.h
	install -m 444 ${SRCDIR}/libion/kernel-headers/linux/ion_mtk.h ${DESTDIR}${INCDIR}/linux/ion_mtk.h
