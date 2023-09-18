LOCAL_PATH:= $(call my-dir)

SIGMA_CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -DWFA_STA_TB -Wno-error

######### libwfa_dut
include $(CLEAR_VARS)
LOCAL_SRC_FILES:= \
	wfa_sock.c wfa_tlv.c wfa_cs.c wfa_cmdtbl.c wfa_tg.c wfa_miscs.c wfa_thr.c wfa_wmmps.c



LOCAL_SHARED_LIBRARIES := \
	libcutils

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../inc \


LOCAL_CFLAGS :=  $(MTK_CFLAGS) $(SIGMA_CFLAGS)

LOCAL_MODULE:=libwfa_dut_static
LOCAL_MODULE_TAGS := eng debug tests 

include $(BUILD_STATIC_LIBRARY)
