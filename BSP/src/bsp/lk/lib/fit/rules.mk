LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)
MODULE_BUILDDIR := $(call TOBUILDDIR,$(MODULE))
MODULE_INCLUDES += $(MODULE_BUILDDIR)/../../../include

MODULE_SRCS += \
    $(LOCAL_DIR)/fit.c \
    $(LOCAL_DIR)/image_verify.c

MODULE_COMPILEFLAGS += -Wno-sign-compare

include make/module.mk
