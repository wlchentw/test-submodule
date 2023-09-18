NAME := rw_nvram
PREFIX := 
SUFFIX :=

TARGET := $(PREFIX)$(NAME)$(SUFFIX)
SOURCES := rw_nvram.c

all:$(TARGET)

INCLUDES :=	-I../../../nvram/1.0.0-r0/nvram-1.0.0/libnvram \
			-I../../../nvram/1.0.0-r0/nvram-1.0.0/libfile_op \
			-I../../../nvram/1.0.0-r0/nvram-1.0.0/NvRAMUtils \
			-I../../../custom/1.0.0-r0/custom-1.0.0/aud8516-ali-slc/linux-4.4.22/common/cgen/cfgdefault \
			-I../../../custom/1.0.0-r0/custom-1.0.0/aud8516-ali-slc/linux-4.4.22/common/cgen/cfgfileinc \
			-I../../../custom/1.0.0-r0/custom-1.0.0/aud8516-ali-slc/linux-4.4.22/common/cgen/inc \

CFLAGS := -g $(INCLUDES)
LDFLAGS := -L. -lc -lNvRAMUtils -lnvram -lfile_op -lnvram_custom

$(TARGET):$(SOURCES)
	$(info PWD=$(PWD))
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY:clean
clean:
	@rm -rf *.o $(TARGET)
