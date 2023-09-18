LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)
MODULE_BUILDDIR := $(call TOBUILDDIR,$(MODULE))
MODULE_INCLUDES += $(MODULE_BUILDDIR)/../../../include lib


SCRATCH_SIZE        ?= 0x04000000 # 64MB
MAX_TEE_DRAM_SIZE   ?= 0x04000000 # 64MB
MAX_KERNEL_SIZE     ?= 0x02000000 # 32MB
MAX_DTB_SIZE        ?= 0x00200000 # 2MB
MAX_DTBO_SIZE       ?= 0x00200000 # 2MB
MAX_LZ4_BUF_SIZE    ?= 0x00100000 # 1MB

GLOBAL_DEFINES += SCRATCH_SIZE=$(SCRATCH_SIZE) \
                  MAX_TEE_DRAM_SIZE=$(MAX_TEE_DRAM_SIZE) \
                  MAX_KERNEL_SIZE=$(MAX_KERNEL_SIZE) \
                  MAX_DTB_SIZE=$(MAX_DTB_SIZE) \
                  MAX_DTBO_SIZE=$(MAX_DTBO_SIZE) \
                  MAX_LZ4_BUF_SIZE=$(MAX_LZ4_BUF_SIZE)

ifeq ($(strip $(AVB_ENABLE_ANTIROLLBACK)),yes)
GLOBAL_COMPILEFLAGS += -DAVB_ENABLE_ANTIROLLBACK
endif

ifeq ($(strip $(AB_OTA_UPDATER)),yes)
GLOBAL_COMPILEFLAGS += -DAB_OTA_UPDATER
endif

MODULE_DEPS += \
    lib/bio \
    lib/lz4 \
    lib/mempool \
    lib/fdt \
    lib/sha256_neon \
    lib/rsa2048 \
    lib/fit \
    lib/fastboot \
    lib/libavb \
    lib/libavb_ab \

MODULE_SRCS += \
	$(LOCAL_DIR)/avbboot.c \
	$(LOCAL_DIR)/avb.c \

MODULE_COMPILEFLAGS += -Wno-sign-compare -Wno-unused-but-set-variable

GLOBAL_COMPILEFLAGS += -Os -Wno-maybe-uninitialized

include make/module.mk
