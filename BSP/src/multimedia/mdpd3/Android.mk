# Copyright Statement:
#
# This software/firmware and related documentation ("MediaTek Software") are
# protected under relevant copyright laws. The information contained herein
# is confidential and proprietary to MediaTek Inc. and/or its licensors.
# Without the prior written permission of MediaTek inc. and/or its licensors,
# any reproduction, modification, use or disclosure of MediaTek Software,
# and information contained herein, in whole or in part, shall be strictly prohibited.
#
# MediaTek Inc. (C) 2010. All rights reserved.
#
# BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
# THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
# RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
# AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
# NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
# SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
# SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
# THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
# THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
# CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
# SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
# STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
# CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
# AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
# OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
# MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.


#
# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH := $(call my-dir)

GET_LOCAL_DIR = $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))
MDP_PLATFORM := mt2712

#
# libmdpdposix
#
include $(CLEAR_VARS)
OBJS :=
INCLUDES :=
include $(LOCAL_PATH)/v4l2_mdpd/posix/Android.mk
LOCAL_CFLAGS	:= -D__MULTI_THREAD -fPIC
LOCAL_SRC_FILES := $(OBJS)
LOCAL_C_INCLUDES := \
    bionic/libc/include \
    $(INCLUDES)

LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_MODULE    := libmdpdposix
LOCAL_PRELINK_MODULE:=false
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH_32 := $(PRODUCT_OUT)/vendor/lib
LOCAL_MODULE_PATH_64 := $(PRODUCT_OUT)/vendor/lib64
LOCAL_MULTILIB := both
LOCAL_MODULE_OWNER := mtk
include $(BUILD_SHARED_LIBRARY)

#
# libmdp_prot.platform
#
include $(CLEAR_VARS)
OBJS :=
INCLUDES :=
include $(LOCAL_PATH)/libmdp/dpframework_prot/Android_$(MDP_PLATFORM).mk
LOCAL_CPPFLAGS	:= -fPIC -D__MULTI_THREAD
LOCAL_SRC_FILES := $(OBJS)

LOCAL_C_INCLUDES := \
    $(INCLUDES)

LOCAL_SHARED_LIBRARIES := libcutils liblog
LOCAL_MODULE := libmdp_prot.$(MDP_PLATFORM)
LOCAL_PRELINK_MODULE:=false
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH_32 := $(PRODUCT_OUT)/vendor/lib
LOCAL_MODULE_PATH_64 := $(PRODUCT_OUT)/vendor/lib64
LOCAL_MULTILIB := both
LOCAL_MODULE_OWNER := mtk
include $(BUILD_SHARED_LIBRARY)

#
# libmdp.platform
#
include $(CLEAR_VARS)
OBJS :=
INCLUDES :=
include $(LOCAL_PATH)/libmdp/dpframework/Android_$(MDP_PLATFORM).mk
OBJS := $(MDP_LIB_OBJS)
LOCAL_CPPFLAGS	:= -fPIC -D__MULTI_THREAD
LOCAL_CPPFLAGS  += $(MDP_CFLAGS)
LOCAL_SRC_FILES := $(OBJS)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/v4l2_mdpd/vpu/include \
    $(LOCAL_PATH)/v4l2_mdpd/vpu/platform/include \
    $(INCLUDES) \
    $(LOCAL_PATH)/v4l2_mdpd/vpu/include/compiler.h

LOCAL_SHARED_LIBRARIES := libcutils liblog libmdpdposix libmdppq.$(MDP_PLATFORM) libmdp_prot.$(MDP_PLATFORM)
LOCAL_MODULE := libmdp.$(MDP_PLATFORM)
LOCAL_PRELINK_MODULE:=false
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH_32 := $(PRODUCT_OUT)/vendor/lib
LOCAL_MODULE_PATH_64 := $(PRODUCT_OUT)/vendor/lib64
LOCAL_MULTILIB := both
LOCAL_MODULE_OWNER := mtk
include $(BUILD_SHARED_LIBRARY)

#
# libmdpd
#
include $(CLEAR_VARS)
OBJS :=
INCLUDES :=
include $(LOCAL_PATH)/v4l2_mdpd/vpu/platform/Android.mk
LOCAL_CPPFLAGS	:= -fPIC -D__MULTI_THREAD
LOCAL_CPPFLAGS	+= $(MDP_CFLAGS)
LOCAL_SRC_FILES := $(OBJS)
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/v4l2_mdpd/vpu/include \
    $(LOCAL_PATH)/v4l2_mdpd/vpu/platform/include \
    $(INCLUDES) \
    $(LOCAL_PATH)/v4l2_mdpd/vpu/include/compiler.h

#OBJS :=
#INCLUDES :=
#include $(LOCAL_PATH)/v4l2_mdpd/vpu/lib/libc/Android.mk
#LOCAL_SRC_FILES += $(OBJS)
#LOCAL_C_INCLUDES += \
#    $(INCLUDES)

OBJS :=
MDPOBJS :=
INCLUDES :=
include $(LOCAL_PATH)/v4l2_mdpd/vpu/app/mdp/Android.mk
LOCAL_SRC_FILES += $(MDPOBJS)
LOCAL_C_INCLUDES += \
    $(INCLUDES)

# for build libmdp.$(MDP_PLATFORM)
#LOCAL_SHARED_LIBRARIES := libcutils liblog libmdpdposix libmdp.$(MDP_PLATFORM)
LOCAL_SHARED_LIBRARIES := libcutils liblog libmdpdposix
LOCAL_MODULE := libmdpd
LOCAL_PRELINK_MODULE:=false
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH_32 := $(PRODUCT_OUT)/vendor/lib
LOCAL_MODULE_PATH_64 := $(PRODUCT_OUT)/vendor/lib64
LOCAL_MULTILIB := both
LOCAL_MODULE_OWNER := mtk
include $(BUILD_SHARED_LIBRARY)

#
# mdpd
#
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS	:= -D_FILE_OFFSET_BITS=64 -D__MULTI_THREAD
LOCAL_SRC_FILES := \
    v4l2_mdpd/src/mdpd.c

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/v4l2_mdpd/include \
    $(LOCAL_PATH)/v4l2_mdpd/vpu/platform/include \

LOCAL_INIT_RC := v4l2_mdpd/src/mdpd.rc
LOCAL_SHARED_LIBRARIES := libcutils libmdpd libmdpdposix liblog
LOCAL_MODULE := mdpd
LOCAL_PRELINK_MODULE := false
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH_32 := $(PRODUCT_OUT)/vendor/bin32
LOCAL_MODULE_PATH_64 := $(PRODUCT_OUT)/vendor/bin
LOCAL_MULTILIB := both
LOCAL_MODULE_OWNER := mtk
include $(BUILD_EXECUTABLE)
