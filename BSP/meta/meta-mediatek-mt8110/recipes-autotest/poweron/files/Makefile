TARGET_GET  := poweron
SOURCES_GET := poweron.c
OBJECTS_GET := $(SOURCES_GET:.c=.o)

ifeq ("$(VA_SUPPORT_GVA_SDK)", "true")
  CFLAGS += -DVA_SUPPORT_GVA_SDK_SUPPORT
endif

ifeq ("$(VA_SUPPORT_ALEXA)", "true")
  CFLAGS += -DVA_SUPPORT_ALEXA
endif

ifeq ("$(AUDIO_SUPPORT_C4A_SDK)", "true")
  CFLAGS += -DAUDIO_SUPPORT_C4A_SDK
endif


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