# SPDX-License-Identifier: MediaTekProprietary

pq_package_arch=$(subst -,_,${PACKAGE_ARCH})
PQ_PLATFORM := $(TARGET_PLATFORM)

TARGET  := mmdemoap v4l2mdp display imghal
SOURCES := 
OBJECTS := $(SOURCES:.c=.o)
$(OBJECTS): 

GET_LOCAL_DIR = $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

OBJS :=
INCLUDES :=
include mmdemo/rules.mk
$(eval $$(OBJS): CFLAGS := -fPIC $$(INCLUDES))
OBJECTS-mmdemo := $(OBJS)

OBJS :=
INCLUDES :=
include mdp/rules.mk
$(eval $$(OBJS): CFLAGS := -fPIC $$(INCLUDES))
OBJECTS-v4l2mdp := $(OBJS)

OBJS :=
INCLUDES :=
include hwtcon_test/rules.mk
$(eval $$(OBJS): CFLAGS := -fPIC $$(INCLUDES))
OBJECTS-hwtcon := $(OBJS)

OBJS :=
INCLUDES :=
include img_hal/rules.mk
$(eval $$(OBJS): CFLAGS := -fPIC $$(INCLUDES))
OBJECTS-imghal := $(OBJS)

all: $(TARGET)
imghal: $(OBJECTS-imghal)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

v4l2mdp: $(OBJECTS-v4l2mdp)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

display: $(OBJECTS-hwtcon)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

mmdemoap: $(OBJECTS-mmdemo)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@
clean:
	$(RM) $(OBJECTS) $(TARGET)


install: $(TARGET)
	install -d $(DESTDIR)$(bindir)
	install -d ${DESTDIR}/data
	install -d ${DESTDIR}/data/init_bin
	install -m 755 mmdemoap $(DESTDIR)$(bindir)
	install -m 755 imghal $(DESTDIR)$(bindir)
	install -m 755 v4l2mdp $(DESTDIR)$(bindir)
	install -m 755 display $(DESTDIR)$(bindir)
	install -m 0666 ${GET_LOCAL_DIR}/../mmdemo/pic/windows.png ${DESTDIR}/data/windows.png
	install -m 0666 ${GET_LOCAL_DIR}/../mmdemo/pic/button.png ${DESTDIR}/data/button.png
	install -m 0666 ${GET_LOCAL_DIR}/../mmdemo/pic/book.jpg ${DESTDIR}/data/book.jpg
	install -m 0666 ${GET_LOCAL_DIR}/../mmdemo/pic/book_content.jpg ${DESTDIR}/data/book_content.jpg
	install -m 0666 ${GET_LOCAL_DIR}/../img_hal/pic/00.jpg ${DESTDIR}/data/00.jpg
	install -m 0444 ${GET_LOCAL_DIR}/../hwtcon_test/init_bin/wf_lut.gz ${DESTDIR}/data/init_bin/wf_lut.gz
