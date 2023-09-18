ifeq ($(strip $(PLATFORM_FLAVOR)),mt8532)
include $(call all-pta-mk-under, $(call my-dir))
endif
