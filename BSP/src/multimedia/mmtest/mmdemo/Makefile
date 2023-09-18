TARGET=demoap
SRC=$(wildcard *.c)
OBJS=${SRC:%.c=%.o}

	
.PHONY: all install clean

all: ${TARGET}	

${TARGET}: ${OBJS}	
	${CC} -o $@ ${OBJS} ${LDFLAGS} -lpthread
		
%.o: %.c
	${CC} ${CFLAGS} -c $<
		
install:	
	install -d $(DESTDIR)$(PREFIX)/bin/	
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/	
	
clean:	
	@rm -fr ${TARGET} *.o core