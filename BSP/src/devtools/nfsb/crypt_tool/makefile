OBJDIR := ${OUT}/crypttool
CC_FLAGS := -Wall -Wno-unused-parameter -Wno-unused-variable -Wno-format -Werror -fPIC -Wno-unused-function -Wno-int-to-pointer-cast

INCLUDES := -I./ \
			$(BASE_INC)
			
EXE_LDFLAGS += -g -ldl -ltz_uree -lsecure_api

EXECUTABLE = ${OUT}/crypt_target_tool

EXEOBJS = crypt_target_tool.o dm_protect_key.o

EXEOBJS := $(addprefix $(OBJDIR)/,$(EXEOBJS))

all:$(EXECUTABLE)

$(EXECUTABLE):$(EXEOBJS) 
	@echo "----Linking `basename $@`----"
	@if [ ! -d `dirname $@` ] ; then \
		mkdir -p `dirname $@`; \
	fi
	$(CC) -o $@ $(EXEOBJS) $(EXE_LDFLAGS)

${OBJDIR}/%.o : %.cpp
	echo "----Building `basename $<`----"
	@if [ ! -d `dirname $@` ] ; then \
		mkdir -p `dirname $@`; \
	fi
	$(CC) $(DEFINES) -DDM_PROTECT_KEY=\"`cat ${DM_PROTECT_KEY}`\" $(INCLUDES) $(CC_FLAGS) -c $< -o $@

install:
	install -d ${DESTDIR}${bindir}
	install -m 0755 ${EXECUTABLE} ${DESTDIR}${bindir}
	install -m 0755 prebuilts/${ARCH}/dmsetup ${DESTDIR}${bindir}
	install -d ${DESTDIR}${libdir}
	install -m 0755 prebuilts/${ARCH}/libdevmapper.so.1.02 ${DESTDIR}${libdir}

clean:
	rm -rf ${OUT}/crypttool