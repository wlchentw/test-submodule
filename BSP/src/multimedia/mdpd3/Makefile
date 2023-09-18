CC_OPTION := -Wformat -Wformat-security -Werror=format-security -Werror -w

TARGET  := mdpd
SOURCES := v4l2_mdpd/src/mdpd.c
OBJECTS := $(SOURCES:.c=.o)
$(OBJECTS): CPPFLAGS := $(CC_OPTION) -Iv4l2_mdpd/include -Iv4l2_mdpd/vpu/platform/include

include has_platform.mk

GET_LOCAL_DIR = $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

include libmdp.mk

OBJS :=
INCLUDES :=
include v4l2_mdpd/vpu/platform/rules.mk
$(eval $$(OBJS): CPPFLAGS := $(CC_OPTION) -fPIC -Iv4l2_mdpd/vpu/include -Iv4l2_mdpd/vpu/platform/include $$(INCLUDES) -include v4l2_mdpd/vpu/include/compiler.h)
OBJECTS-libmdpd.so := $(OBJS)

OBJS :=
INCLUDES :=
include v4l2_mdpd/vpu/lib/libc/rules.mk
$(eval $$(OBJS): CPPFLAGS := $(CC_OPTION) -fPIC -Iv4l2_mdpd/vpu/include -Iv4l2_mdpd/vpu/platform/include $$(INCLUDES) -include v4l2_mdpd/vpu/include/compiler.h)
OBJECTS-libmdpd.so += $(OBJS)

OBJS :=
INCLUDES :=
MDPOBJS :=
include v4l2_mdpd/vpu/app/mdp/rules.mk
OBJS := $(MDPOBJS)
$(eval $$(OBJS): CPPFLAGS := $(CC_OPTION) -fPIC -ldl -Iv4l2_mdpd/vpu/include -Iv4l2_mdpd/vpu/platform/include $$(INCLUDES) -include v4l2_mdpd/vpu/include/compiler.h)
OBJECTS-libmdpd.so += $(OBJS)

OBJS :=
INCLUDES :=
include v4l2_mdpd/posix/rules.mk
$(eval $$(OBJS): CPPFLAGS := $(CC_OPTION) $$(INCLUDES))
OBJECTS += $(OBJS)

.PHONY: all clean install

all: $(TARGET)

include libmdp.target.mk

libmdpd.so: $(OBJECTS-libmdpd.so)
	$(CC) -shared $(CFLAGS) $(CPPFLAGS) -pthread $(LDFLAGS) $^ -o $@

$(TARGET): $(OBJECTS) libmdpd.so
	$(CC) -pthread -ldl $(LDFLAGS) $^ -o $@

clean:
	$(RM) $(OBJECTS) $(TARGET)

install: $(TARGET) libmdpd.so $(LIB_MDP_TARGET)
	install -d $(DESTDIR)$(bindir)
	install -d $(DESTDIR)$(libdir)
	install -m 755 $(TARGET) $(DESTDIR)$(bindir)
	install -m 644 libmdpd.so $(DESTDIR)$(libdir)
	install -m 644 libmdp.*.so $(DESTDIR)$(libdir)
	-install -m 644 libmdp_tile.*.so $(DESTDIR)$(libdir)
	-install -m 644 libcmdq.*.so $(DESTDIR)$(libdir)
