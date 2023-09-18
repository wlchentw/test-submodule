LOCAL_PATH := $(call my-dir)
$(warning "the value of LOCAL_PATH is $(LOCAL_PATH)")

include $(CLEAR_VARS)

LOCAL_MODULE := wfa_dut
LOCAL_MODULE_TAGS := debug
LOCAL_STATIC_LIBRARIES := libwfa
LOCAL_CFLAGS := $(L_CFLAGS) -Wno-unused-variable -Wno-unused-parameter -Wno-format-security -Wno-format-invalid-specifier -Wno-format-extra-args -Wno-format -Wno-undefined-inline -Wno-pointer-sign -Wno-unused-parameter -Wno-incompatible-pointer-types -Wno-dangling-else -Wno-pointer-arith -Wno-parentheses-equality -Wno-array-bounds -Wno-missing-field-initializers -Wno-constant-logical-operand -Wno-incompatible-pointer-types-discards-qualifiers -Wno-sign-compare -Wno-user-defined-warnings -Wno-implicit-function-declaration -Wno-unused-label -Wno-unused-function
LOCAL_SRC_FILES := wfa_dut_init.c wfa_dut.c wfa_wpa.c wfa_mtk_wfd.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../inc
include $(BUILD_EXECUTABLE)
