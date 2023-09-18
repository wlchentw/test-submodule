LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
    lib/sha256_neon

MODULE_SRCS := \
	$(LOCAL_DIR)/hmac.c

include make/module.mk
