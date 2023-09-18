LOCAL_DIR := $(GET_LOCAL_DIR)
MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/dramc_pi_basic_api.c \
	$(LOCAL_DIR)/dramc_pi_calibration_api.c \
	$(LOCAL_DIR)/dramc_pi_ddr_reserve.c \
	$(LOCAL_DIR)/dramc_pi_main.c \
	$(LOCAL_DIR)/psramc_pi_basic_api.c \
	$(LOCAL_DIR)/psramc_pi_calibration_api.c \
	$(LOCAL_DIR)/psramc_pi_main.c \
	$(LOCAL_DIR)/emi.c \
	$(LOCAL_DIR)/Hal_io.c \
	$(LOCAL_DIR)/pemi.c \

DDR_TYPE ?= COMMON_DDR3_16BIT

GLOBAL_DEFINES += $(DDR_TYPE)

include make/module.mk
