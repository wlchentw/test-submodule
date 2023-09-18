CC := ${CROSS_COMPILE}gcc
AR := ${CROSS_COMPILE}ar
LD := ${CROSS_COMPILE}ld
OBJCOPY := ${CROSS_COMPILE}objcopy

CUST_TEE := ./custom/$(TZ_PROJECT)/cust_tee.mak
CUST_TEE_EXIST := $(if $(wildcard $(CUST_TEE)),TRUE,FALSE)

include ./default.mak
ifeq ("$(CUST_TEE_EXIST)","TRUE")
include ./custom/$(TZ_PROJECT)/cust_tee.mak
endif
include ./feature.mak

LDS = tllink.lds

DIR_INC = ./include
DIR_SRC = ./src
DIR_PREBUILT = ./prebuilt
DIR_OBJ = ${TL_RAW_OUT}/obj
DIR_BIN = ${TL_RAW_OUT}/bin

ASRCS = $(wildcard $(DIR_SRC)/*.s)
CSRCS = $(wildcard $(DIR_SRC)/*.c)
CSRCS += \
	$(DIR_SRC)/drivers/device_apc.c \
	$(DIR_SRC)/security/tz_init.c \
	$(DIR_SRC)/security/tz_emi_mpu.c \
	$(DIR_SRC)/security/tz_sec_cfg.c \
	$(DIR_SRC)/security/seclib_dummy.c \
	$(DIR_SRC)/drivers/tz_apc.c

ifeq ($(CFG_TRUSTONIC_TEE_SUPPORT),1)
CSRCS += \
	$(DIR_SRC)/security/tz_tbase.c
endif
ifeq ($(CFG_TRUSTKERNEL_TEE_SUPPORT),1)
CSRCS += \
	$(DIR_SRC)/security/tz_tkcore.c
endif

AOBJS = $(patsubst %.s, $(DIR_OBJ)/%.o, $(notdir $(ASRCS)))
COBJS = $(patsubst %.c, $(DIR_OBJ)/%.o, $(notdir $(CSRCS)))
SOBJS = $(wildcard $(DIR_PREBUILT)/*.a)
OBJS = $(AOBJS) $(COBJS) $(SOBJS)

CFLAGS += -fno-builtin -fno-stack-protector ${C_OPTION}

TARGET = teeloader
BIN_TARGET = $(DIR_BIN)/$(TARGET)

all: $(OBJS)
	@if [ ! -d `dirname $(BIN_TARGET).elf` ] ; then \
		mkdir -p `dirname $(BIN_TARGET).elf`; \
	fi
	sed "s/%BASE_ADDR%/${BASE_ADDR}/g" $(LDS) > $(DIR_OBJ)/$(LDS)
	$(LD) --start-group $^ --end-group -T$(DIR_OBJ)/$(LDS) -o $(BIN_TARGET).elf
	-echo "teeloader binary created"
	$(OBJCOPY) -O binary $(BIN_TARGET).elf $(BIN_TARGET).bin
	./zero_padding.sh $(BIN_TARGET).bin ${TL_ALIGN_SIZE}

$(COBJS): $(CSRCS)
	@if [ ! -d `dirname $@` ] ; then \
		mkdir -p `dirname $@`; \
	fi
	$(CC) -I$(DIR_INC) $(CFLAGS) -c $(filter %$(patsubst %.o,%.c,$(notdir $@)),$(CSRCS)) -o $@

$(AOBJS): $(ASRCS)
	@if [ ! -d `dirname $@` ] ; then \
		mkdir -p `dirname $@`; \
	fi
	$(CC) -c $(filter %$(patsubst %.o,%.s,$(notdir $@)),$(ASRCS)) -o $@

.PHONY: clean
clean:
	-@rm -rf $(DIR_OBJ)/* $(DIR_BIN)/*
