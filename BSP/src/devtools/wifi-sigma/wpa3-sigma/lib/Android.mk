LOCAL_PATH := $(call my-dir)
$(warning "the value of LOCAL_PATH is $(LOCAL_PATH)")

include $(CLEAR_VARS)

LOCAL_MODULE := libwfa
LOCAL_SHARED_LIBRARIES :=
LOCAL_CFLAGS := $(L_CFLAGS) -Wno-unused-variable -Wno-unused-parameter -Wno-format-security -Wno-format-invalid-specifier -Wno-format-extra-args -Wno-format -Wno-undefined-inline -Wno-pointer-sign -Wno-unused-parameter -Wno-incompatible-pointer-types -Wno-dangling-else -Wno-pointer-arith -Wno-parentheses-equality -Wno-array-bounds -Wno-missing-field-initializers -Wno-constant-logical-operand -Wno-incompatible-pointer-types-discards-qualifiers -Wno-sign-compare -Wno-user-defined-warnings -Wno-implicit-function-declaration -Wno-unused-label
LOCAL_SRC_FILES := wfa_sock.c wfa_tlv.c wfa_cs.c wfa_cmdtbl.c wfa_tg.c wfa_miscs.c wfa_thr.c wfa_wmmps.c wfa_ca_resp.c wfa_typestr.c wfa_cmdproc.c mtk_ini.c iniparser.c dictionary.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../inc


#LOCAL_MODULE := libwfa_ca
#LOCAL_SHARED_LIBRARIES :=
#LOCAL_CFLAGS := $(L_CFLAGS)
#LOCAL_SRC_FILES := wfa_sock.c wfa_tlv.c wfa_ca_resp.c wfa_cmdproc.c wfa_miscs.c wfa_typestr.c
#LOCAL_C_INCLUDES := $(LOCAL_PATH)/../inc

include $(BUILD_STATIC_LIBRARY)
