LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	wfa_con.c \
	wfa_sndrcv.c \
	wfa_util.c

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libc \
    liblog

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/../inc

LOCAL_STATIC_LIBRARIES += libwfa_dut_static

LOCAL_CFLAGS := -g -O2 -DWFA_DEBUG -DANDROID_PLATFORM -Wno-error
LOCAL_FORCE_STATIC_EXECUTABLE := false
LOCAL_MODULE_PATH=$(BIN_OUT_PATH)
LOCAL_MODULE:= wfa_con
LOCAL_MODULE_TAGS := eng debug tests
LOCAL_MULTILIB := 32
include $(BUILD_EXECUTABLE)
