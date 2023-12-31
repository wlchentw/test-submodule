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
# The following software/firmware and/or related documentation ("MediaTek Software")
# have been modified by MediaTek Inc. All revisions are subject to any receiver's
# applicable license agreements with MediaTek Inc.



ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6735 mt6737t mt6735m mt6737m mt6753 mt6570 mt6580 mt6752 mt6755 mt6757 mt6750 mt6797 mt6799 mt6759 mt6758 mt6763 mt6739 mt8163 mt8167 mt8173 mt6775 mt6771 mt6765 mt6761 mt3967 mt6779))

#
# libdpframework.so
#
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

include $(TOP)/$(MTK_PATH_SOURCE)/hardware/dpframework/mtkcam_inc.mk

#MTK_MDP_VERSION can only be used in mt6799
#Use MDP_VERSION to replace MTK_MDP_VERSION on trunk temporarily due to BRM rule
MDP_VERSION:=2
ifeq ($(strip $(TARGET_BOARD_PLATFORM)), mt6799)
    ifeq ($(MDP_VERSION), 2)
        LOCAL_CFLAGS += -DMDP_VERSION2
        MDP_PLATFORM_VERSION := mt6799p
    endif
endif

#set platform folder name
ifeq ($(MDP_PLATFORM_VERSION), mt6799p)
    MDP_PLATFORM_FOLDER := mt6799p
    MDP_PATH_SWITCH :=
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6735 mt6753 mt6737t))
    MDP_PLATFORM_FOLDER := mt6735
    MDP_PATH_SWITCH := /D1
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6735m mt6737m))
    MDP_PLATFORM_FOLDER := mt6735
    MDP_PATH_SWITCH := /D2
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6755 mt6750))
    MDP_PLATFORM_FOLDER := mt6755
    MDP_PATH_SWITCH :=
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6765 mt6761))
    MDP_PLATFORM_FOLDER := mt6765
    MDP_PATH_SWITCH :=
else
    MDP_PLATFORM_FOLDER := $(TARGET_BOARD_PLATFORM)
    MDP_PATH_SWITCH :=
endif

ifeq ($(BUILD_MTK_LDVT),true)
    LOCAL_CFLAGS += -DUSING_MTK_LDVT
endif


LOCAL_CFLAGS += -DMDP_VERSION_2_0
ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt8163 mt8167))
    LOCAL_CFLAGS += -DMDP_VERSION_USE_TABLET_M4U
endif

ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt8173))
    LOCAL_CFLAGS += -DMDP_VERSION_8173
endif
ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt8167 mt8163 mt8173))
    LOCAL_CFLAGS += -DMDP_VERSION_TABLET
endif

# for HW code generated by R2, SW reset at each tile
ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6755 mt6750 mt6797 mt6739))
    LOCAL_CFLAGS += -DMDP_HW_TILE_SW_RESET
endif

ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6755 mt6757 mt6750 mt6797 mt6799 mt6759 mt6758 mt6763 mt6739 mt6775 mt6771 mt6765 mt6761 mt3967 mt6779))
    LOCAL_CFLAGS += -DLIBPQ_VERSION_1_1
endif

# Platform independent part

ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6735m mt6737m))
    LOCAL_CFLAGS += -DCMDQ_D2_EVENT
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6753))
    LOCAL_CFLAGS += -DCMDQ_D3_EVENT
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6735 mt6737t))
    LOCAL_CFLAGS += -DCMDQ_D1_EVENT
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6797 mt6757))
    LOCAL_CFLAGS += -DCMDQ_6797_EVENT
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6799 mt6759 mt6758 mt6763 mt6775 mt6771 mt3967 mt6779))
    LOCAL_CFLAGS += -DCMDQ_6797_EVENT
    LOCAL_CFLAGS += -DCMDQ_V3
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6765 mt6761))
    LOCAL_CFLAGS += -DCMDQ_6797_EVENT
    LOCAL_CFLAGS += -DCMDQ_6765_EVENT
    LOCAL_CFLAGS += -DCMDQ_V3
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6739))
    LOCAL_CFLAGS += -DCMDQ_6739_EVENT
    LOCAL_CFLAGS += -DCMDQ_V3
endif

ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6755 mt6757 mt6750 mt6797 mt6799 mt6758 mt6763 mt6759 mt6739 mt6775 mt6771 mt6765 mt6761 mt3967 mt6779))
    LOCAL_CFLAGS += -DMDP_RSZ_DISABLE_DCM_SMALL_TILE
endif


MDP_COMMON_FOLDER := ../common

#Top level common code
LOCAL_SRC_FILES:= \
    $(MDP_COMMON_FOLDER)/buffer/DpBufferPool.cpp \
    $(MDP_COMMON_FOLDER)/composer/DpPathComposer.cpp \
    $(MDP_COMMON_FOLDER)/core/DpPathControl.cpp \
    $(MDP_COMMON_FOLDER)/engine/DpEngine_CAMIN.cpp \
    $(MDP_COMMON_FOLDER)/engine/DpTileUtil.cpp \
    $(MDP_COMMON_FOLDER)/stream/DpAsyncBlitStream.cpp \
    $(MDP_COMMON_FOLDER)/stream/DpBlitStream.cpp \
    $(MDP_COMMON_FOLDER)/stream/DpChannel.cpp \
    $(MDP_COMMON_FOLDER)/stream/DpFragStream.cpp \
    $(MDP_COMMON_FOLDER)/stream/DpIspStream.cpp \
    $(MDP_COMMON_FOLDER)/stream/DpPortAdapt.cpp \
    $(MDP_COMMON_FOLDER)/stream/DpStream.cpp \
    $(MDP_COMMON_FOLDER)/stream/DpVEncStream.cpp \
    $(MDP_COMMON_FOLDER)/stream/DpNotifier.cpp \
    $(MDP_COMMON_FOLDER)/util/DpProfiler.cpp \
    $(MDP_COMMON_FOLDER)/util/DpWriteBMP.cpp \
    $(MDP_COMMON_FOLDER)/util/DpWriteBin.cpp

#MDP2.0 Common code
LOCAL_SRC_FILES += \
    core/DpCommandQueue.cpp \
    core/DpCommandRecorder.cpp \
    core/DpPathConnection.cpp \
    core/DpTilePath.cpp \
    engine/DpEngineBase.cpp \
    engine/DpEngine_IMG2O.cpp \
    engine/DpEngine_IMGI.cpp \
    engine/DpEngine_RDMA.cpp \
    engine/DpEngine_VENC.cpp \
    engine/DpEngine_WDMA.cpp \
    engine/DpEngine_WROT.cpp \
    engine/DpTileEngine.cpp

ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6759 mt6758 mt6763 mt6775 mt6771 mt6765 mt6761 mt3967 mt6779))
LOCAL_SRC_FILES += \
    engine/DpEngine_MDPSOUT.cpp
endif

ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6775 mt6771 mt3967))
LOCAL_SRC_FILES += \
    engine/DpEngine_CCORR.cpp
endif

ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6775 mt6771 mt3967 mt6779))
LOCAL_SRC_FILES += \
    engine/DpEngine_AAL.cpp
endif

ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt3967 mt6779))
LOCAL_SRC_FILES += \
    engine/DpEngine_HDR.cpp
endif

ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6799 mt6775 mt6771 mt3967))
LOCAL_SRC_FILES += \
    engine/DpEngine_WPEO.cpp \
    engine/DpEngine_WPEI.cpp
endif

# OSAL: memory proxy layer
LOCAL_SRC_FILES += \
    $(MDP_COMMON_FOLDER)/osal/android/memory/DpMemory_Android.cpp

# OSAL: Special memory handler
LOCAL_SRC_FILES += \
    $(MDP_COMMON_FOLDER)/osal/android/memory/DpIonHandler.cpp \
    $(MDP_COMMON_FOLDER)/osal/android/memory/DpMmuHandler.cpp \
    $(MDP_COMMON_FOLDER)/osal/android/memory/DpPmemHandler.cpp

# OSAL: Priority manager
LOCAL_SRC_FILES += \
    osal/android/driver/DpDriver_Android.cpp

# OSAL: Get/set property
LOCAL_SRC_FILES += \
    $(MDP_COMMON_FOLDER)/osal/android/property/DpProperty_Android.cpp

# OSAL: Thread abstraction
LOCAL_SRC_FILES += \
    $(MDP_COMMON_FOLDER)/osal/android/thread/DpThread_Android.cpp

# OSAL: Sync abstraction
LOCAL_SRC_FILES += \
    $(MDP_COMMON_FOLDER)/osal/android/sync/DpSync_Android.cpp

# IP dependent part: RSZ
ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6580 mt6570 mt8167))
    IP_RSZ_FOLDER:=rsz6582
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6797))
    IP_RSZ_FOLDER:=rsz6797
    LOCAL_CFLAGS += -DRSZ_2_0
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6799 mt6763 mt6758 mt6775 mt6771 mt6765 mt6761 mt3967 mt6779))
    IP_RSZ_FOLDER:=rsz6799
    LOCAL_CFLAGS += -DRSZ_2_0
    LOCAL_CFLAGS += -DRSZ_MT6799
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6759 mt6739))
    IP_RSZ_FOLDER:=rsz6759
    LOCAL_CFLAGS += -DRSZ_2_0
    LOCAL_CFLAGS += -DRSZ_MT6759
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt8173))
    IP_RSZ_FOLDER:=rsz8173
else
    IP_RSZ_FOLDER:=rsz6752
endif

MTK_MDP_RSZ_SRC := engine/rsz/$(IP_RSZ_FOLDER)

LOCAL_SRC_FILES += \
    $(MTK_MDP_RSZ_SRC)/DpEngine_RSZ.cpp

ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6757 mt6799 mt6758 mt6763 mt6739 mt6775 mt6771 mt6765 mt6761 mt3967 mt6779))
    LOCAL_CFLAGS += -DSUPPORT_CLEARZOOM
endif

# IP dependent part: Tile/ISP
ifeq ($(MDP_PLATFORM_VERSION), mt6799p)
    IP_TILE_ISP_FOLDER:=isp6799p
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6735m mt6737m mt6570 mt6580))
    IP_TILE_ISP_FOLDER:=isp6582
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6739))
    IP_TILE_ISP_FOLDER:=isp6739
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6755 mt6750 mt6761))
    IP_TILE_ISP_FOLDER:=isp6755
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6797))
    IP_TILE_ISP_FOLDER:=isp6797
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6799))
    IP_TILE_ISP_FOLDER:=isp6799
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6757))
    IP_TILE_ISP_FOLDER:=isp6757
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6759))
    IP_TILE_ISP_FOLDER:=isp6759
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6758))
    IP_TILE_ISP_FOLDER:=isp6758
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6763))
    IP_TILE_ISP_FOLDER:=isp6763
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6765))
    IP_TILE_ISP_FOLDER:=isp6765
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6775))
    IP_TILE_ISP_FOLDER:=isp6775
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6771))
    IP_TILE_ISP_FOLDER:=isp6771
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt3967))
    IP_TILE_ISP_FOLDER:=isp3967
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6779))
    IP_TILE_ISP_FOLDER:=isp6779
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt8163))
    IP_TILE_ISP_FOLDER:=isp8163
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt8167))
    IP_TILE_ISP_FOLDER:=isp8167
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt8173))
    IP_TILE_ISP_FOLDER:=isp8173
else
    IP_TILE_ISP_FOLDER:=isp6752
endif

MTK_MDP_TILE_ISP_SRC := engine/isp/$(IP_TILE_ISP_FOLDER)

LOCAL_SRC_FILES += \
    $(MTK_MDP_TILE_ISP_SRC)/DpWrapper_ISP.cpp

ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6799 mt6775 mt6771 mt3967))
    LOCAL_SRC_FILES += \
        $(MTK_MDP_TILE_ISP_SRC)/DpWrapper_WPE.cpp
endif

# IP dependent part: hdr
SUPPORT_HDR := false

# IP dependent part: tdshp
ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6739 mt6755 mt6757 mt6750 mt6797 mt6799 mt6759 mt6758 mt6763 mt6775 mt6771 mt6765 mt6761 mt3967 mt6779))
    LOCAL_CFLAGS += -DTDSHP_1_1
endif
ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6797 mt6799 mt6763 mt6758 mt6771 mt6775 mt6765 mt6761 mt3967 mt6779))
    LOCAL_CFLAGS += -DTDSHP_2_0
endif
ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6771 mt6775 mt6765 mt6761 mt3967 mt6779))
    LOCAL_CFLAGS += -DTDSHP_3_0
endif

ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6797))
    LOCAL_CFLAGS += -DDYN_SHARP_VERSION=1
    LOCAL_CFLAGS += -DDYN_CONTRAST_VERSION=1
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6799 mt6763 mt6758))
    LOCAL_CFLAGS += -DDYN_SHARP_VERSION=1
    LOCAL_CFLAGS += -DDYN_CONTRAST_VERSION=2
else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6771 mt6775 mt6765 mt6761 mt3967 mt6779))
    LOCAL_CFLAGS += -DDYN_SHARP_VERSION=2
    LOCAL_CFLAGS += -DDYN_CONTRAST_VERSION=2
else
    LOCAL_CFLAGS += -DDYN_SHARP_VERSION=0
    LOCAL_CFLAGS += -DDYN_CONTRAST_VERSION=0
endif

# VSS async mode enable list
ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6771 mt6775 mt3967 mt6779))
    LOCAL_CFLAGS += -DMDP_VSS_ASYNC_ENABLE
    LOCAL_CFLAGS += -DISP_SMART_TILE_ENABLE
    LOCAL_CFLAGS += -DMDP_REDUCE_CONFIG
endif

ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6765))
    LOCAL_CFLAGS += -DMDP_VSS_ASYNC_ENABLE
endif

ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6797 mt6799))
    LOCAL_CFLAGS += -DSUPPORT_VIDEO_UR
endif

ifeq ($(MTK_BASIC_PACKAGE), yes)
    MDP_OPEN_PQ_CONFIG := no
else
    MDP_OPEN_PQ_CONFIG := yes
endif

ifeq ($(MDP_OPEN_PQ_CONFIG), no)
    LOCAL_CFLAGS += -DBASIC_PACKAGE
endif

IP_TDSHP_FOLDER:=tdshp6752
MTK_MDP_TDSHP_SRC := engine/tdshp/$(IP_TDSHP_FOLDER)

#PQ support
ifneq ($(MDP_OPEN_PQ_CONFIG), no)

MTK_MDP_PQ_SRC := engine/pq

# IP dependent part: hdr
ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6799))
    ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6799))
        LOCAL_CFLAGS += -DHDR_IN_RDMA
    endif

    LOCAL_CFLAGS += -DSUPPORT_HDR
    SUPPORT_HDR := true
endif

# IP dependent part: AAL
ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6771 mt6775))
    LOCAL_CFLAGS += -DSUPPORT_DRE
    SUPPORT_DRE := true
endif

LOCAL_SRC_FILES += \
    $(MTK_MDP_PQ_SRC)/PQSession.cpp \
    $(MTK_MDP_PQ_SRC)/PQSessionManager.cpp \
    $(MTK_MDP_PQ_SRC)/PQConfig.cpp \
    $(MTK_MDP_PQ_SRC)/PQDCConfig.cpp \
    $(MTK_MDP_PQ_SRC)/PQDSConfig.cpp \
    $(MTK_MDP_PQ_SRC)/PQDSAdaptor.cpp \
    $(MTK_MDP_PQ_SRC)/PQDCAdaptor.cpp \
    $(MTK_MDP_PQ_SRC)/PQAlgorithmAdaptor.cpp \
    $(MTK_MDP_PQ_SRC)/PQAlgorithmFactory.cpp \
    $(MTK_MDP_PQ_SRC)/PQTuningBuffer.cpp \
    $(MTK_MDP_PQ_SRC)/PQAshmem.cpp \
    $(MTK_MDP_PQ_SRC)/PQReadBackFactory.cpp \
    $(MTK_MDP_PQ_SRC)/PQIspTuning.cpp

# add rsz for some platforms
ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6797 mt6799 mt6759 mt6758 mt6763 mt6739 mt6775 mt6771 mt6765 mt6761 mt3967 mt6779))
LOCAL_SRC_FILES += \
    $(MTK_MDP_PQ_SRC)/PQRSZAdaptor.cpp \
    $(MTK_MDP_PQ_SRC)/PQRSZConfig.cpp
endif

# add color for some platforms
ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6755 mt6757 mt6750 mt6797 mt6799 mt6759 mt6758 mt6763 mt6775 mt6771 mt6765 mt6761 mt3967 mt6779))
    LOCAL_SRC_FILES += \
        $(MTK_MDP_PQ_SRC)/PQColorConfig.cpp \
        $(MTK_MDP_PQ_SRC)/PQColorAdaptor.cpp
endif # end of color

# add hdr for some platforms
ifeq ($(SUPPORT_HDR),true)
LOCAL_SRC_FILES += \
    $(MTK_MDP_PQ_SRC)/PQHDRAdaptor.cpp \
    $(MTK_MDP_PQ_SRC)/PQHDRConfig.cpp
endif

# add dre for some platforms
ifeq ($(SUPPORT_DRE),true)
LOCAL_SRC_FILES += \
    $(MTK_MDP_PQ_SRC)/PQDREAdaptor.cpp \
    $(MTK_MDP_PQ_SRC)/PQDREConfig.cpp \
    $(MTK_MDP_PQ_SRC)/PQDREHistogramAdaptor.cpp
endif
LOCAL_CFLAGS += -DDRE_ANDROID_PLATFORM

# add pq white list for some platforms
ifeq ($(SUPPORT_PQ_WHITE_LIST),true)
LOCAL_SRC_FILES += \
    $(MTK_MDP_PQ_SRC)/PQWhiteList.cpp
endif

LOCAL_SRC_FILES += \
    $(MTK_MDP_TDSHP_SRC)/DpEngine_TDSHP.cpp

# add color for some platforms
ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6755 mt6757 mt6750 mt6797 mt6799 mt6759 mt6758 mt6763 mt6775 mt6771 mt6765 mt6761 mt3967 mt6779))
    # IP dependent part: color
    IP_COLOR_FOLDER:=color6755

    LOCAL_CFLAGS += -DMDP_COLOR_ENABLE

    ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6797 mt6757 mt6799 mt6763 mt6758 mt6775 mt6771 mt3967 mt6779))
        LOCAL_CFLAGS += -DCOLOR_2_1
    endif

    ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6757 mt6799 mt6763 mt6758 mt6775 mt6771 mt3967 mt6779))
        LOCAL_CFLAGS += -DCOLOR_3_0
    endif

    ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6759 mt6758 mt6763 mt6775 mt6771 mt6765 mt6761 mt3967 mt6779))
        LOCAL_CFLAGS += -DCOLOR_HW_SHARE
    endif

    MTK_MDP_COLOR_SRC := engine/color/$(IP_COLOR_FOLDER)

    LOCAL_SRC_FILES += \
        $(MTK_MDP_COLOR_SRC)/DpEngine_COLOR.cpp
endif # end of color

ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6771))
    LOCAL_CFLAGS += -DSUPPORT_NVRAM_TUNING
endif

endif
#end of PQ support

# Platform dependent part
MTK_MDP_PLATFORM_SRC := platform/$(MDP_PLATFORM_FOLDER)$(MDP_PATH_SWITCH)

LOCAL_SRC_FILES += \
    $(MTK_MDP_PLATFORM_SRC)/composer/DpPathTopology.cpp \
    $(MTK_MDP_PLATFORM_SRC)/engine/DpEngineMutex.cpp \
    $(MTK_MDP_PLATFORM_SRC)/engine/DpPathConnectionPlatform.cpp

ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6759 mt6758 mt6763 mt6775 mt6771 mt6765 mt6761 mt3967 mt6779))
    LOCAL_SRC_FILES += $(MTK_MDP_PLATFORM_SRC)/engine/DpESLControl.cpp
endif

ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt8173))
    LOCAL_SRC_FILES += $(MTK_MDP_PLATFORM_SRC)/engine/DpEngine_MDPMOUT.cpp
endif

LOCAL_C_INCLUDES:= \
    $(MTK_PATH_SOURCE)/hardware/dpframework/MDP2.0/core \
    $(MTK_PATH_SOURCE)/hardware/dpframework/MDP2.0/engine \
    $(MTK_PATH_SOURCE)/hardware/dpframework/MDP2.0/hardware \
    $(MTK_PATH_SOURCE)/hardware/dpframework/MDP2.0/osal \
    $(MTK_PATH_SOURCE)/hardware/dpframework/common/buffer \
    $(MTK_PATH_SOURCE)/hardware/dpframework/common/composer \
    $(MTK_PATH_SOURCE)/hardware/dpframework/common/core \
    $(MTK_PATH_SOURCE)/hardware/dpframework/common/engine \
    $(MTK_PATH_SOURCE)/hardware/dpframework/common/osal \
    $(MTK_PATH_SOURCE)/hardware/dpframework/common/stream \
    $(MTK_PATH_SOURCE)/hardware/dpframework/common/util \
    $(MTK_PATH_SOURCE)/hardware/libomx/video/MtkWLParser/include \
    $(MTK_PATH_SOURCE)/hardware/libomx/video/MtkWLParser/service/parserservice/include \
    $(MTK_PATH_SOURCE)/external/perfservicenative

# Venc Modify +

ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6797 mt6799))
    LOCAL_CFLAGS += -DMTK_SLOW_MOTION_HEVC_SUPPORT

    LOCAL_C_INCLUDES += \
        $(MTK_PATH_SOURCE)/hardware/libvcodec/common/hardware/hevc_enc
    LOCAL_C_INCLUDES += \
        $(MTK_PATH_SOURCE)/hardware/libvcodec/include
else
    #LOCAL_CFLAGS += -DMTK_SLOW_MOTION_H264_SUPPORT

    LOCAL_C_INCLUDES += \
        $(MTK_PATH_SOURCE)/hardware/libvcodec/common/hardware/h264_enc
endif

# Venc Modify -

ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6735 mt6737t mt6735m mt6737m mt6753 mt6570 mt6580 mt6752 mt6755 mt6757 mt6750 mt6797 mt6799 mt6759 mt6758 mt6763 mt8163 mt8167 mt8173 mt6775 mt6771 mt6765 mt6761 mt3967 mt6779))
    ifeq ($(IS_LEGACY), 0)
        ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6771 mt3967 mt6775))
            LOCAL_C_INCLUDES += \
                $(MTK_PATH_SOURCE)/hardware/mtkcam/drv/include/dip/isp_50/
        else ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6779))
            LOCAL_C_INCLUDES += \
                $(MTK_PATH_SOURCE)/hardware/mtkcam/drv/include/dip/$(TARGET_BOARD_PLATFORM)/
        else ifeq ($(MDP_PLATFORM_VERSION), mt6799p)
            LOCAL_C_INCLUDES += \
                $(MTK_PATH_SOURCE)/hardware/mtkcam/drv/include/mt6799p/drv
        else
            LOCAL_C_INCLUDES += \
                $(MTK_PATH_SOURCE)/hardware/mtkcam/drv/include/$(TARGET_BOARD_PLATFORM)/drv
        endif

    else
        LOCAL_C_INCLUDES += $(MTK_MTKCAM_PLATFORM)/include
        LOCAL_C_INCLUDES += $(MTK_MTKCAM_PLATFORM)/include/mtkcam/drv
    endif
endif

LOCAL_C_INCLUDES += \
    $(MTK_PATH_SOURCE)/hardware/dpframework/MDP2.0/$(MTK_MDP_PLATFORM_SRC)/engine \
    $(MTK_PATH_SOURCE)/hardware/dpframework/MDP2.0/$(MTK_MDP_PLATFORM_SRC)/hardware \
    $(MTK_PATH_SOURCE)/hardware/dpframework/MDP2.0/$(MTK_MDP_PLATFORM_SRC)

LOCAL_C_INCLUDES += \
    $(MTK_PATH_SOURCE)/hardware/dpframework/MDP2.0/$(MTK_MDP_RSZ_SRC) \
    $(MTK_PATH_SOURCE)/hardware/dpframework/MDP2.0/$(MTK_MDP_TILE_ISP_SRC) \
    $(MTK_PATH_SOURCE)/hardware/dpframework/MDP2.0/$(MTK_MDP_TDSHP_SRC) \
    $(MTK_PATH_SOURCE)/hardware/dpframework/MDP2.0/$(MTK_MDP_PQ_SRC)

LOCAL_C_INCLUDES += \
    $(MTK_PATH_SOURCE)/hardware/jpeg/$(MTK_PLATFORM_DIR)/hal

LOCAL_C_INCLUDES += \
    $(MTK_PATH_SOURCE)/hardware/dpframework/include

LOCAL_C_INCLUDES += \
    $(MTK_PATH_SOURCE)/hardware/pq/v2.0/include \
    $(MTK_PATH_SOURCE)/hardware/m4u/$(MTK_PLATFORM_DIR) \
    $(MTK_PATH_SOURCE)/external/libion_mtk/include \
    $(MTK_PATH_SOURCE)/external/include

LOCAL_C_INCLUDES += \
    $(TOP)/system/core/include/utils \
    $(TOP)/system/core/libsync/include/sync \
    $(TOP)/system/core/libsync \
    $(TOP)/system/core/include \
    $(TOP)/frameworks/native/libs/nativewindow/include

LOCAL_C_INCLUDES += \
    $(MTK_PATH_SOURCE)/hardware/gralloc_extra/include

ifeq ($(BUILD_MTK_LDVT),true)
LOCAL_C_INCLUDES += \
    $(MTK_PATH_PLATFORM)/external/ldvt/include \
    $(MTK_PATH_PLATFORM)/hardware/camera/inc/drv
endif

LOCAL_HEADER_LIBRARIES := libbinder_headers libhardware_headers

ifeq ($(BUILD_MTK_LDVT),true)
LOCAL_WHOLE_STATIC_LIBRARIES += libuvvf
endif

ifeq ($(strip $(MTK_MIRAVISION_SUPPORT)), yes)
    LOCAL_CFLAGS += -DCONFIG_FOR_SOURCE_PQ
endif

#Log Level
ifeq ($(TARGET_BUILD_VARIANT), eng)
    CONFIG_LOG_LEVEL := 1
else
    CONFIG_LOG_LEVEL := 0
endif

#Debug Dump Config
ifeq ($(TARGET_BUILD_VARIANT), eng)
    LOCAL_CFLAGS += -DDEBUG_DUMP_REG
else ifeq ($(TARGET_BUILD_VARIANT), userdebug)
    LOCAL_CFLAGS += -DDEBUG_DUMP_REG
endif

#malloc_debug
ifneq ($(TARGET_BUILD_VARIANT), user)
    LOCAL_CFLAGS += -fno-omit-frame-pointer
endif

LOCAL_CFLAGS += -DCONFIG_LOG_LEVEL=$(CONFIG_LOG_LEVEL)

ifneq (,$(filter $(strip $(TARGET_BUILD_VARIANT)), eng userdebug))
    LOCAL_CFLAGS += -DENABLE_PQ_DEBUG_LOG
endif

LOCAL_SHARED_LIBRARIES := \
    libutils \
    libion \
    libcutils \
    liblog \
    libsync \
    libdl \
    libhardware

#PQ support
ifneq ($(MDP_OPEN_PQ_CONFIG), no)
LOCAL_SHARED_LIBRARIES += \
    libhidlbase \
    libhidlmemory
endif

ifneq (,$(filter $(strip $(TARGET_BOARD_PLATFORM)), mt6797 mt6799))
    LOCAL_SHARED_LIBRARIES += \
        libvcodecdrv
endif

#Vendor, system partition use different so
ifeq ($(FOR_SYSTEM_PARTITION), 0)
    ifeq ($(MDP_PLATFORM_VERSION), mt6799p)
        LOCAL_WHOLE_STATIC_LIBRARIES := \
            libdpframework_prot2
    else
        LOCAL_WHOLE_STATIC_LIBRARIES := \
            libdpframework_prot
    endif

    LOCAL_SHARED_LIBRARIES += \
        libion_mtk \
        libgralloc_extra

    ifeq ($(MTK_M4U_SUPPORT), yes)
        LOCAL_SHARED_LIBRARIES += \
            libm4u
    endif

    ifneq ($(MDP_OPEN_PQ_CONFIG), no)
        LOCAL_SHARED_LIBRARIES += \
            vendor.mediatek.hardware.pq@2.0 \
            libpq_prot
    endif
else
    ifeq ($(MDP_PLATFORM_VERSION), mt6799p)
        LOCAL_WHOLE_STATIC_LIBRARIES := \
            libdpframework_prot2_mtk
    else
        LOCAL_WHOLE_STATIC_LIBRARIES := \
            libdpframework_prot_mtk
    endif

    LOCAL_SHARED_LIBRARIES += \
        libion_mtk_sys \
        libgralloc_extra_sys

    ifeq ($(MTK_M4U_SUPPORT), yes)
        LOCAL_SHARED_LIBRARIES += \
            libm4u_sys
    endif

    ifneq ($(MDP_OPEN_PQ_CONFIG), no)
        LOCAL_SHARED_LIBRARIES += \
            vendor.mediatek.hardware.pq@2.0 \
            libpq_prot_mtk
    endif
endif

MTK_MTKCAM_PLATFORM :=

endif
