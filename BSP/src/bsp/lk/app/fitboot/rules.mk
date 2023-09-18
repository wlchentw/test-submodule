LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)
MODULE_BUILDDIR := $(call TOBUILDDIR,$(MODULE))
MODULE_INCLUDES += $(MODULE_BUILDDIR)/../../../include
MODULE_INCLUDES += $(LOCAL_DIR)/../../platform/$(PLATFORM)/include/platform
LOCAL_DIR_EMI := $(LOCAL_DIR)/../../platform/$(PLATFORM)/drivers/dram
MODULE_INCLUDES += $(LOCAL_DIR)/../../lib/fastboot/include

ifeq ($(MT8518),1)
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

endif

ifeq ($(MT8512),1)
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


MODULE_SRCS += \
	$(LOCAL_DIR)/fitboot.c \

include make/module.mk
