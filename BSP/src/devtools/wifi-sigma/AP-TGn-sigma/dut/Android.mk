LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	wfa_dut.c \
	wfa_dut_init.c \

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libc \
    liblog

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/../inc

LOCAL_CFLAGS := -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -DWFA_STA_TB -DANDROID_PLATFORM -Wno-error
LOCAL_STATIC_LIBRARIES += libwfa_dut_static
LOCAL_FORCE_STATIC_EXECUTABLE := false
LOCAL_MODULE_PATH=$(BIN_OUT_PATH)
LOCAL_MODULE:= wfa_dut
LOCAL_MODULE_TAGS := eng debug tests 
LOCAL_MULTILIB := 32
include $(BUILD_EXECUTABLE)
