LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MULTILIB := 32

SIGMA_CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -DWFA_STA_TB -Wno-error

LOCAL_SRC_FILES:= \
	wfa_dut.c \
	wfa_dut_init.c \

LOCAL_SHARED_LIBRARIES := \
	libcutils

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/../inc \

LOCAL_CFLAGS := $(SIGMA_CFLAGS) $(MTK_CFLAGS)

#libwfa_static
LOCAL_STATIC_LIBRARIES += libwfa_dut_static
#LOCAL_STATIC_LIBRARIES += libminzip libunz libmtdutils libmincrypt 
#LOCAL_STATIC_LIBRARIES += libminui libpixelflinger_static libpng libcutils


LOCAL_FORCE_STATIC_EXECUTABLE := false

LOCAL_MODULE_PATH=$(BIN_PATH)

LOCAL_MODULE:= wfa_dut
LOCAL_MODULE_TAGS := eng debug tests 

include $(BUILD_EXECUTABLE)
