# Copyright 2006 The Android Open Source Project
###############################################################################
LOCAL_PATH:= $(call my-dir)
###############################################################################

## include variants like TA_DEV_KIT_DIR
## and OPTEE_BIN
INCLUDE_FOR_BUILD_TA := false
include $(BUILD_OPTEE_MK)
INCLUDE_FOR_BUILD_TA :=

VERSION = $(shell git describe --always --dirty=-dev 2>/dev/null || echo Unknown)

# TA_DEV_KIT_DIR must be set to non-empty value to
# avoid the Android build scripts complaining about
# includes pointing outside the Android source tree.
# This var is expected to be set when OPTEE OS built.
# We set the default value to an invalid path.
TA_DEV_KIT_DIR ?= ../invalid_include_path

-include $(TA_DEV_KIT_DIR)/host_include/conf.mk

include $(CLEAR_VARS)

LOCAL_SRC_FILES := src/pkcs11_api.c \
				src/ck_debug.c \
				src/ck_helpers.c \
				src/invoke_ta.c \
				src/pkcs11_token.c \
				src/serializer.c \
				src/serialize_ck.c \
				src/pkcs11_processing.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include

LOCAL_CFLAGS += -D_GNU_SOURCE -fPIC

LOCAL_SHARED_LIBRARIES := libteec

LOCAL_ADDITIONAL_DEPENDENCIES := $(OPTEE_BIN)
LOCAL_ADDITIONAL_DEPENDENCIES += fd02c9da-306c-48c7-a49c-bbd827ae86ee.ta

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := mtk

LOCAL_MODULE := libsks

include $(BUILD_SHARED_LIBRARY)

include $(LOCAL_PATH)/../ta/ta.mk


