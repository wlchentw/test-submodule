LOCAL_DIR := $(GET_LOCAL_DIR)
MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
        lib/libc \
        lib/debug \

GLOBAL_INCLUDES += \
    $(LKROOT)/../lk_ext_mod/platform/mt2712/include \

MODULES += \
    $(LKROOT)/../lk_ext_mod/platform/mt2712/drivers \

MODULE_SRCS += \
    $(LOCAL_DIR)/mtk_wrapper.c \

include make/module.mk
