LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
    lib/wrapper \
    lib/unittest \

MODULE_SRCS := \
    $(LOCAL_DIR)/wrapper_test.c

include make/module.mk
