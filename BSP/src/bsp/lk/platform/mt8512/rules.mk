LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH ?= arm64
ARM_CPU ?= cortex-a53
WITH_SMP ?= 0
WITH_KERNEL_VM ?= 0
WITH_EL3_EXCEPTIONS ?= 1

LK_HEAP_IMPLEMENTATION ?= miniheap

GLOBAL_INCLUDES += -I$(LK_TOP_DIR)/include \

MT8512 :=1

GLOBAL_DEFINES += \
       MT8512=1

ifeq ($(ARCH),arm)
GLOBAL_DEFINES += \
       ARCH_ARM=1
endif

MODULE_SRCS += \
    $(LOCAL_DIR)/platform.c \
    $(LOCAL_DIR)/debug.c \
    $(LOCAL_DIR)/interrupts.c \
    $(LOCAL_DIR)/timer.c \

ifdef MT8512
ifeq ($(ARCH),arm)
# not link to libefuse.o
else
MODULE_EXTRA_OBJS += \
    $(LOCAL_DIR)/lib/libefuse.o
endif
else
MODULE_EXTRA_OBJS += \
    $(LOCAL_DIR)/lib/libefuse.o
endif

ifeq ($(WITH_KERNEL_VM),1)
BOOT_ARGUMENT_LOCATION	:= 0xffff000011600100UL
else
BOOT_ARGUMENT_LOCATION	:= 0x40000100
endif

GLOBAL_DEFINES += \
	   BOOT_ARGUMENT_LOCATION=$(BOOT_ARGUMENT_LOCATION)

ifeq ($(WITH_KERNEL_VM),1)
KERNEL_ASPACE_BASE ?= 0xffff000000110000
KERNEL_ASPACE_SIZE ?= 0x00000000f0000000
MMU_IDENT_SIZE_SHIFT ?= 32
endif
MEMBASE ?= 0x110000
KERNEL_LOAD_OFFSET ?= 0x1000
MEMSIZE ?= 0x00050000   # 256K
MACH_TYPE := 8512

MODULE_DEPS += \
    dev/timer/arm_generic \
    lib/cksum \
    lib/mempool \

ifeq ($(WITH_KERNEL_VM),1)
GLOBAL_DEFINES += MMU_IDENT_SIZE_SHIFT=$(MMU_IDENT_SIZE_SHIFT)
endif

ifeq ($(WITH_MTK_PMIC_WRAP_AND_PMIC),1)
GLOBAL_DEFINES += WITH_MTK_PMIC_WRAP_AND_PMIC=$(WITH_MTK_PMIC_WRAP_AND_PMIC)
endif

ifeq ($(WITH_PMIC_MT6398),1)
	GLOBAL_DEFINES += WITH_PMIC_MT6398=$(WITH_PMIC_MT6398)
endif

ifeq ($(WITH_PMIC_BD71828),1)
	GLOBAL_DEFINES += WITH_PMIC_BD71828=$(WITH_PMIC_BD71828)
endif

ifeq ($(WITH_MAX20342),1)
	GLOBAL_DEFINES += WITH_MAX20342=$(WITH_MAX20342)
endif

ifeq ($(WITH_PMIC_M2296),1)
	GLOBAL_DEFINES += WITH_PMIC_M2296=$(WITH_PMIC_M2296)
endif

ifeq ($(WITH_EXT_32K),1)
	GLOBAL_DEFINES += WITH_EXT_32K=$(WITH_EXT_32K)
endif

ifeq ($(MTK_TINYSYS_SCP_SUPPORT), yes)
MODULE_DEPS += $(LOCAL_DIR)/drivers/scp
GLOBAL_DEFINES += MTK_TINYSYS_SCP_SUPPORT
endif
ifeq ($(WITH_VCORE_I2C_BUCK),1)
	GLOBAL_DEFINES += WITH_VCORE_I2C_BUCK=$(WITH_VCORE_I2C_BUCK)
endif

ifeq ($(WITH_VCORE_PWM_BUCK),1)
	GLOBAL_DEFINES += WITH_VCORE_PWM_BUCK=$(WITH_VCORE_PWM_BUCK)
endif

ifeq ($(WITH_32K_OSC),1)
	GLOBAL_DEFINES += WITH_32K_OSC=$(WITH_32K_OSC)
endif

ifeq ($(WITH_UBOOT),1)
GLOBAL_DEFINES += WITH_UBOOT=$(WITH_UBOOT)
endif

ifeq ($(SUPPORT_SLC_GPT_BACKUP), 1)
	GLOBAL_DEFINES += SUPPORT_SLC_GPT_BACKUP=1
endif

GLOBAL_DEFINES += \
    MEMBASE=$(MEMBASE) \
    MEMSIZE=$(MEMSIZE) \
    RAMBASE=$(RAMBASE) \
    MACH_TYPE=$(MACH_TYPE) \
    PLATFORM_SUPPORTS_PANIC_SHELL=1 \
    WITH_NO_FP=1 \

LINKER_SCRIPT += \
    $(BUILDDIR)/system-onesegment.ld

include make/module.mk $(LOCAL_DIR)/drivers/rules.mk