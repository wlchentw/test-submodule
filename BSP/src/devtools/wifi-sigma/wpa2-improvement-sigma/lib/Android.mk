LOCAL_PATH:= $(call my-dir)

SIGMA_CFLAGS = -g -O2 -D_REENTRANT  -DWFA_WMM_PS_EXT -DWFA_WMM_AC -DWFA_VOICE_EXT -DWFA_STA_TB
SIGMA_CFLAGS += -DMTK_11N_SIGMA
SIGMA_CFLAGS += -DMTK_11N_SIGMA_LAST

############ libwfa
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	wfa_sock.c \
	wfa_tg.c \
	wfa_cs.c \
	wfa_ca_resp.c \
	wfa_tlv.c \
	wfa_typestr.c \
	wfa_cmdtbl.c \
	wfa_cmdproc.c \
	wfa_miscs.c \
	wfa_thr.c \
	wfa_wmmps.c 	



LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../inc \

LOCAL_CFLAGS := $(SIGMA_CFLAGS) -DMTK_P2P_SUPPLICANT -DMTK_P2P_SIGMA -DMTK_WFD_SUPPORT -DMTK_WFD_SIGMA


LOCAL_MODULE:=libwfa_static
LOCAL_MODULE_TAGS := eng debug tests 

include $(BUILD_STATIC_LIBRARY)


######### libwfa_ca
include $(CLEAR_VARS)
LOCAL_SRC_FILES:= wfa_sock.c wfa_tlv.c wfa_ca_resp.c wfa_cmdproc.c wfa_miscs.c wfa_typestr.c



LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../inc \

LOCAL_CFLAGS := $(SIGMA_CFLAGS) -DMTK_P2P_SUPPLICANT -DMTK_P2P_SIGMA

LOCAL_MODULE:=libwfa_ca_static
LOCAL_MODULE_TAGS := eng debug tests 

include $(BUILD_STATIC_LIBRARY)

######### libwfa_dut
include $(CLEAR_VARS)
LOCAL_SRC_FILES:= \
	wfa_sock.c wfa_tlv.c wfa_cs.c wfa_cmdtbl.c wfa_tg.c wfa_miscs.c wfa_thr.c wfa_wmmps.c



LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils 

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../inc \


LOCAL_CFLAGS := $(SIGMA_CFLAGS) -DMTK_P2P_SUPPLICANT  -DMTK_P2P_SIGMA -DMTK_WFD_SIGMA -DMTK_WFD_SUPPORT 

LOCAL_MODULE:=libwfa_dut_static
LOCAL_MODULE_TAGS := eng debug tests 
LOCAL_MULTILIB := 32
include $(BUILD_STATIC_LIBRARY)
