LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)
MODULE_BUILDDIR := $(call TOBUILDDIR,$(MODULE))
MODULE_INCLUDES += $(MODULE_BUILDDIR)/../../../include \
				   lib

MODULE_SRCS += \
	$(LOCAL_DIR)/avb_ab_flow.c

MODULE_DEFINES += AVB_COMPILATION

include make/module.mk
