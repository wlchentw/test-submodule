LOCAL_DIR := $(GET_LOCAL_DIR)
MODULE := $(LOCAL_DIR)


MODULE_SRCS += \
    $(LOCAL_DIR)/uart/uart.c	\
    $(LOCAL_DIR)/pll/pll.c \
    $(LOCAL_DIR)/spm/spm_mtcmos.c \
    $(LOCAL_DIR)/wdt/mtk_wdt.c \
    $(LOCAL_DIR)/i2c/mt_i2c.c \
    $(LOCAL_DIR)/usb/mtu3.c \
    $(LOCAL_DIR)/usb/mtu3_qmu.c \
    $(LOCAL_DIR)/gic/mt_gic_v3.c \
    $(LOCAL_DIR)/gpio/gpio.c \
    $(LOCAL_DIR)/key/mtk_key.c \
    $(LOCAL_DIR)/trng/mtk_trng.c \
    $(LOCAL_DIR)/pwm/pwm.c \
	$(LOCAL_DIR)/ntxhw/ntx_hw.c \

ifeq ($(SPI_NAND_PROJECT), y)
	MODULE_DEPS += \
		lib/bio \
		lib/partition \
		lib/nftl

	include $(LOCAL_DIR)/nandx/Nandx.mk
endif

ifeq ($(WITH_PMIC_MT6398), 1)
	MODULE_SRCS += $(LOCAL_DIR)/pmic/pmic_6398.c
endif

ifeq ($(WITH_PMIC_BD71828), 1)
	MODULE_SRCS += $(LOCAL_DIR)/pmic/bd71828.c
endif

ifeq ($(WITH_PMIC_M2296), 1)
	MODULE_SRCS += $(LOCAL_DIR)/pmic/m2296.c
endif

ifeq ($(WITH_CLKSQ_MONCK_OFF),1)
	GLOBAL_DEFINES += WITH_CLKSQ_MONCK_OFF=$(WITH_CLKSQ_MONCK_OFF)
endif

ifeq ($(WITH_VCORE_I2C_BUCK), 1)
	MODULE_SRCS += $(LOCAL_DIR)/pmic/rt5748.c
endif

ifeq ($(WITH_VCORE_PWM_BUCK), 1)
	MODULE_SRCS += $(LOCAL_DIR)/pmic/pwm_buck.c
endif

ifeq ($(WITH_USB_MD1122), 1)
	MODULE_SRCS += $(LOCAL_DIR)/usb/u3phy-i2c.c
	MODULE_SRCS += $(LOCAL_DIR)/usb/md1122.c
else
	MODULE_SRCS += $(LOCAL_DIR)/usb/usbphy.c
endif

ifeq ($(WITH_MAX20342), 1)
	MODULE_SRCS += $(LOCAL_DIR)/usb/max20342.c
endif

MODULE_DEPS += \
	$(LOCAL_DIR)/../../../../dramk_8512/dram

include make/module.mk
