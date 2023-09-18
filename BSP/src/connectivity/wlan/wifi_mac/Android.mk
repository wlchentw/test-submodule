LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := wifi_bt_mac_write
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS += -Wall
LOCAL_SRC_FILES := main.c
LOCAL_C_INCLUDES = $(LOCAL_PATH)/../wifi_mac
$(warning $(LOCAL_C_INCLUDES))
include $(BUILD_EXECUTABLE)
