LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
    lib/hmac \
    lib/sha256_neon \

MODULE_SRCS := \
	$(LOCAL_DIR)/rpmb_mac.c

include make/module.mk
