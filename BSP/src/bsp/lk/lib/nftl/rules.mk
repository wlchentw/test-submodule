LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
    $(LOCAL_DIR)/nftl_core.c \
    $(LOCAL_DIR)/nftl_part.c \
    $(LOCAL_DIR)/nftl_bdev.c \

include make/module.mk
