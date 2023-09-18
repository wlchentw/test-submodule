LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
	lib/mempool

MODULE_SRCS := \
	$(LOCAL_DIR)/mempool_test.c

include make/module.mk
