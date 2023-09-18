LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
    $(LOCAL_DIR)/dl_commands.c \
    $(LOCAL_DIR)/fastboot.c \
    $(LOCAL_DIR)/ext_boot.c \

MODULE_COMPILEFLAGS += -Wno-sign-compare

include make/module.mk
