LOCAL_PATH := $(call my-dir)
$(warning "the value of LOCAL_PATH is $(LOCAL_PATH)")

include $(CLEAR_VARS)

LOCAL_MODULE := wfa_ca
LOCAL_MODULE_TAGS := debug
LOCAL_STATIC_LIBRARIES := libwfa
LOCAL_CFLAGS := $(L_CFLAGS)
LOCAL_SRC_FILES := wfa_ca.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../inc
include $(BUILD_EXECUTABLE)
