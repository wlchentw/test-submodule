LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	wfa_ca.c \

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libandroid_runtime

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../inc \

LOCAL_CFLAGS := -D_REENTRANT $(MTK_CFLAGS)

LOCAL_STATIC_LIBRARIES += libwfa_ca_static
#LOCAL_STATIC_LIBRARIES += libminzip libunz libmtdutils libmincrypt 
#LOCAL_STATIC_LIBRARIES += libminui libpixelflinger_static libpng libcutils
LOCAL_STATIC_LIBRARIES += libstdc++ libc 

LOCAL_FORCE_STATIC_EXECUTABLE := true

LOCAL_MODULE:= wfa_ca

include $(BUILD_EXECUTABLE)
