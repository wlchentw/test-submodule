ifeq ($(strip $(PLATFORM_FLAVOR)),mt8532)
libdir := $(call my-dir)
libname := mtee_fwk_src

include $(BUILD_OPTEE_OS_LIB)
endif