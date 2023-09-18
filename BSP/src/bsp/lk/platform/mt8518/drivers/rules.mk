LOCAL_DIR := $(GET_LOCAL_DIR)
MODULE := $(LOCAL_DIR)


MODULE_SRCS += \
    $(LOCAL_DIR)/uart/uart.c	\
    $(LOCAL_DIR)/pll/pll.c \
    $(LOCAL_DIR)/spm/spm_mtcmos.c \
    $(LOCAL_DIR)/wdt/mtk_wdt.c \
    $(LOCAL_DIR)/i2c/mt_i2c.c \
    $(LOCAL_DIR)/usb/mt_usb.c \
    $(LOCAL_DIR)/dram/dramc_actiming_api.c \
    $(LOCAL_DIR)/dram/dramc_basic_api.c \
    $(LOCAL_DIR)/dram/dramc_calibration_api.c \
    $(LOCAL_DIR)/dram/dramc_cbt_api.c \
    $(LOCAL_DIR)/dram/dramc_engine_api.c \
    $(LOCAL_DIR)/dram/dramc_gating_api.c \
    $(LOCAL_DIR)/dram/dramc_initial_api.c \
    $(LOCAL_DIR)/dram/dramc_main.c \
    $(LOCAL_DIR)/dram/dramc_mr_api.c \
    $(LOCAL_DIR)/dram/dramc_rxcal_api.c \
    $(LOCAL_DIR)/dram/dramc_txcal_api.c \
    $(LOCAL_DIR)/dram/emi.c \
    $(LOCAL_DIR)/dram/hal_io.c \
    $(LOCAL_DIR)/dram/mt_mem.c \
    $(LOCAL_DIR)/gic/mt_gic_v3.c \
    $(LOCAL_DIR)/key/mtk_key.c \
    $(LOCAL_DIR)/trng/mtk_trng.c \
    $(LOCAL_DIR)/pwm/pwm.c \

ifeq ($(WITH_MTK_PMIC_WRAP_AND_PMIC), 1)
	MODULE_SRCS += $(LOCAL_DIR)/pwrap/pmic_wrap_init.c
	MODULE_SRCS += $(LOCAL_DIR)/pmic/pmic.c
endif

ifeq ($(WITH_PMIC_MT6395), 1)
	MODULE_SRCS += $(LOCAL_DIR)/pmic/pmic_6395.c
endif

ifeq ($(WITH_VCORE_I2C_BUCK), 1)
	MODULE_SRCS += $(LOCAL_DIR)/pmic/rt5748.c
endif

ifeq ($(WITH_VCORE_PWM_BUCK), 1)
	MODULE_SRCS += $(LOCAL_DIR)/pmic/pwm_buck.c
endif

ifeq ($(FPGA_PLATFORM), 1)
	MODULE_SRCS += $(LOCAL_DIR)/usb/md1122.c
endif

MODULE_EXTRA_OBJS += \
    $(LOCAL_DIR)/../lib/libdevinfo.o

include make/module.mk
