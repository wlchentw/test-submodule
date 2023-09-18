LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= \
	wfa_sock.c wfa_tlv.c wfa_cs.c wfa_cmdtbl.c wfa_tg.c wfa_miscs.c wfa_thr.c wfa_wmmps.c

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libandroid_runtime \
	liblog

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../inc \

LOCAL_CFLAGS := -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -DWFA_STA_TB -DANDROID_PLATFORM -Wno-error
LOCAL_LDFLAGS := -hash-style=both
LOCAL_MODULE:=libwfa_dut_static
LOCAL_MODULE_TAGS := eng debug tests 
LOCAL_MULTILIB := 32
include $(BUILD_STATIC_LIBRARY)
