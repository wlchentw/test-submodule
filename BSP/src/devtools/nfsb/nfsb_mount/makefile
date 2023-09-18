OBJDIR := ${OUT}/nfsb_mount_out
CC_FLAGS := -Wall -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-unused-variable -Werror -Os -fPIC -Wno-unused-function -Wno-int-to-pointer-cast -Wno-write-strings

INCLUDES := -I./ \
			$(BASE_INC)
			
EXE_LDFLAGS += -g -ldl -lcrypto -lssl

EXECUTABLE = ${OUT}/nfsb_mount

EXEOBJS = nfsb_mount.o rsa.o nfsb.o

EXEOBJS := $(addprefix $(OBJDIR)/,$(EXEOBJS))

all:$(EXECUTABLE)

$(EXECUTABLE):$(EXEOBJS) 
	@echo "----Linking `basename $@`----"
	@if [ ! -d `dirname $@` ] ; then \
		mkdir -p `dirname $@`; \
	fi
	$(CC) -o $@ $(EXEOBJS) $(EXE_LDFLAGS)

${OBJDIR}/%.o : %.c
	echo "----Building `basename $<`----"
	@if [ ! -d `dirname $@` ] ; then \
		mkdir -p `dirname $@`; \
	fi
	$(CC) $(DEFINES) $(INCLUDES) $(CC_FLAGS) -c $< -o $@

${OBJDIR}/%.o : %.cpp
	echo "----Building `basename $<`----"
	@if [ ! -d `dirname $@` ] ; then \
		mkdir -p `dirname $@`; \
	fi
	$(CC) $(DEFINES) $(INCLUDES) $(CC_FLAGS) -c $< -o $@

install:
	install -d ${DESTDIR}${bindir}
	install -m 0755 ${EXECUTABLE} ${DESTDIR}${bindir}

clean:
	rm -rf ${OUT}/nfsb_mount
