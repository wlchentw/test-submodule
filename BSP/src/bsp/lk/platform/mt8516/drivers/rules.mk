LOCAL_DIR := $(GET_LOCAL_DIR)
MODULE := $(LOCAL_DIR)


MODULE_SRCS += \
    $(LOCAL_DIR)/i2c/mtk_i2c.c \
    $(LOCAL_DIR)/uart/uart.c \
    $(LOCAL_DIR)/wdt/mtk_wdt.c \
    $(LOCAL_DIR)/usb/mt_usb.c \
    $(LOCAL_DIR)/key/mtk_key.c \
    $(LOCAL_DIR)/pll/pll.c \
    $(LOCAL_DIR)/spm/spm_mtcmos.c \
    $(LOCAL_DIR)/led/mtk_led.c \
    $(LOCAL_DIR)/gpufreq/gpu_freq.c \
    $(LOCAL_DIR)/ptp/mtk_ptp.c

ifeq ($(WITH_MTK_PMIC_WRAP_AND_PMIC), 1)
MODULE_SRCS += $(LOCAL_DIR)/pwrap/pmic_wrap.c
MODULE_SRCS += $(LOCAL_DIR)/pmic/pmic_6392.c
endif

DDR_TYPE ?= COMMON_DDR3_16BIT

GLOBAL_DEFINES += $(DDR_TYPE)

MODULE_DEPS += \
    lib/bio \
    lib/partition \
    lib/fdt \
    lib/cksum \
    ../dramk_8516/dram \

MODULE_EXTRA_OBJS += \
    $(LOCAL_DIR)/../lib/libdevinfo.o

include make/module.mk

