LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

SIGMA_CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -DWFA_STA_TB -Wall 
SIGMA_CFLAGS += -DMTK_11N_SIGMA

LOCAL_SRC_FILES:= \
	wfa_dut.c \
	wfa_dut_init.c \

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libwpa_client


LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/../inc \

LOCAL_CFLAGS := $(SIGMA_CFLAGS) -DMTK_P2P_SUPPLICANT -DMTK_P2P_SIGMA -DMTK_WFD_SIGMA -DMTK_WFD_SUPPORT

#libwfa_static
LOCAL_STATIC_LIBRARIES += libwfa_dut_static
#LOCAL_STATIC_LIBRARIES += libminzip libunz libmtdutils libmincrypt 
#LOCAL_STATIC_LIBRARIES += libminui libpixelflinger_static libpng libcutils
#LOCAL_STATIC_LIBRARIES += libstdc++ libc 
LOCAL_SHARED_LIBRARIES += libstdc++ libc

LOCAL_FORCE_STATIC_EXECUTABLE := false

LOCAL_MODULE_PATH=$(LOCAL_PATH)

LOCAL_MODULE:= wfa_dut
LOCAL_MODULE_TAGS := eng debug tests 
LOCAL_MULTILIB := 32
include $(BUILD_EXECUTABLE)
