TARGET_GET  := getprop
SOURCES_GET := getprop.c
OBJECTS_GET := $(SOURCES_GET:.c=.o)

.PHONY: all clean install

all: $(TARGET_GET)

$(TARGET_GET): $(OBJECTS_GET)
	${CC}  -o $@ ${OBJECTS_GET} ${LDFLAGS}

%.o: %.c
	${CC} ${CFLAGS} -c $<

clean:
	@rm -rf ${TARGET_GET} *.o core

install: $(TARGET_GET)
	install -d $(DESTDIR)/bin/
	install -m 0755 ${TARGET_GET} $(DESTDIR)/bin/