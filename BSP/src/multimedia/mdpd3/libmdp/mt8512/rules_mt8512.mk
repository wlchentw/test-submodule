LOCAL_DIR := $(GET_LOCAL_DIR)/../dpframework

$(warning DESTDIR = $(DESTDIR))
$(warning LOCAL_DIR = $(LOCAL_DIR))

PQ_SUPPORT := 1
LIB_PQ_DIR := $(LOCAL_DIR)/../../libpq
PQ_DIR := $(LOCAL_DIR)/../../pq

MDP_CFLAGS += -DMDP_VERSION_2_0
MDP_CFLAGS += -DMDP_VERSION_8512
MDP_CFLAGS += -DCMDQ_6797_EVENT
MDP_CFLAGS += -DCMDQ_V3
MDP_CFLAGS += -DMT8512_PQ_SUPPORT
ifeq ($(PQ_SUPPORT), 1)
MDP_CFLAGS += -DLIBPQ_VERSION_1_1
MDP_CFLAGS += -DTDSHP_1_1
MDP_CFLAGS += -DTDSHP_2_0
MDP_CFLAGS += -DTDSHP_3_0
MDP_CFLAGS += -DDYN_SHARP_VERSION=2
MDP_CFLAGS += -DDYN_CONTRAST_VERSION=2
##MDP_CFLAGS += -DSUPPORT_DRE
##SUPPORT_DRE := true
#MDP_CFLAGS += -DMDP_COLOR_ENABLE
#MDP_CFLAGS += -DCOLOR_2_1
#MDP_CFLAGS += -DCOLOR_3_0
#MDP_CFLAGS += -DCOLOR_HW_SHARE
#MDP_CFLAGS += -DDRE_ANDROID_PLATFORM
MDP_CFLAGS += -DNO_PQ_SERVICE
else
MDP_CFLAGS += -DBASIC_PACKAGE
MDP_CFLAGS += -DDYN_SHARP_VERSION=0
MDP_CFLAGS += -DDYN_CONTRAST_VERSION=0
endif

MDP_CFLAGS += -DRSZ_2_0
MDP_CFLAGS += -DRSZ_MT6799
MDP_CFLAGS += -DMDP_RSZ_DISABLE_DCM_SMALL_TILE
MDP_CFLAGS += -DMDP_VSS_ASYNC_ENABLE
MDP_CFLAGS += -DISP_SMART_TILE_ENABLE
MDP_CFLAGS += -DMDP_REDUCE_CONFIG

INCLUDES += \
    -I$(LOCAL_DIR)/../dpframework_prot/include/common \
    -I$(LOCAL_DIR)/../dpframework_prot/include/mt8512 \
    -I$(LOCAL_DIR)/include \
    -I$(LOCAL_DIR)/include/mt8512 \
    -I$(LOCAL_DIR)/MDP2.0/platform/mt8512/hardware \
    -I$(LOCAL_DIR)/MDP2.0/platform/mt8512 \
    -I$(LOCAL_DIR)/MDP2.0/platform/mt8512/engine \
    -I$(LOCAL_DIR)/MDP2.0/hardware \
    -I$(LOCAL_DIR)/MDP2.0/core \
    -I$(LOCAL_DIR)/MDP2.0/osal \
    -I$(LOCAL_DIR)/MDP2.0/osal/android/driver \
    -I$(LOCAL_DIR)/MDP2.0/osal/android/thread \
    -I$(LOCAL_DIR)/MDP2.0/engine \
    -I$(LOCAL_DIR)/MDP2.0/engine \
    -I$(LOCAL_DIR)/MDP2.0/engine/rsz/rsz8512 \
    -I$(LOCAL_DIR)/common/stream \
    -I$(LOCAL_DIR)/common/buffer \
    -I$(LOCAL_DIR)/common/core \
    -I$(LOCAL_DIR)/common/osal \
    -I$(LOCAL_DIR)/common/osal/android/sync \
    -I$(LOCAL_DIR)/common/osal/android/logger \
    -I$(LOCAL_DIR)/common/osal/android/timer \
    -I$(LOCAL_DIR)/common/osal/android/memory \
    -I$(LOCAL_DIR)/common/osal/android/property \
    -I$(LOCAL_DIR)/common/osal/android/thread \
    -I$(LOCAL_DIR)/common/osal \
    -I$(LOCAL_DIR)/common/engine \
    -I$(LOCAL_DIR)/common/composer \
    -I$(LOCAL_DIR)/common/util \
    -I$(LOCAL_DIR)/../../kernel-headers/mt8512 \
    -I$(LOCAL_DIR)/../../v4l2_mdpd/vpu/app/mdp \
    -I$(LOCAL_DIR)/../../v4l2_mdpd/vpu/include \
    -I$(LOCAL_DIR)/../if \
    -I$(LOCAL_DIR)/../../kernel-headers/mt8512
    #-I$(LOCAL_DIR)/MDP2.0/engine/isp/isp8512 \

INCLUDES += \
    -I$(LOCAL_DIR)/MDP2.0/engine/tdshp/tdshp8512 \
    -I$(LOCAL_DIR)/MDP2.0/engine/pq \
    -I$(LIB_PQ_DIR)/include \
    -I$(PQ_DIR)/v2.0/include

#Top level common code
MDP_LIB_OBJS  += \
    $(LOCAL_DIR)/common/buffer/DpBufferPool.o \
    $(LOCAL_DIR)/common/composer/DpPathComposer.o \
    $(LOCAL_DIR)/common/core/DpPathControl.o \
    $(LOCAL_DIR)/common/engine/DpTileUtil.o \
    $(LOCAL_DIR)/common/stream/DpBlitStream.o \
    $(LOCAL_DIR)/common/stream/DpChannel.o \
    $(LOCAL_DIR)/common/stream/DpPortAdapt.o \
    $(LOCAL_DIR)/common/stream/DpStream.o \
    $(LOCAL_DIR)/common/util/DpWriteBMP.o \
    $(LOCAL_DIR)/common/util/DpWriteBin.o
    #$(LOCAL_DIR)/common/stream/DpIspStream.o \

#MDP2.0 common code
MDP_LIB_OBJS  += \
    $(LOCAL_DIR)/MDP2.0/core/DpCommandRecorder.o \
    $(LOCAL_DIR)/MDP2.0/core/DpCommandQueue.o \
    $(LOCAL_DIR)/MDP2.0/core/DpPathConnection.o \
    $(LOCAL_DIR)/MDP2.0/core/DpTilePath.o \
    $(LOCAL_DIR)/MDP2.0/engine/DpEngineBase.o \
    $(LOCAL_DIR)/MDP2.0/engine/DpEngine_RDMA.o \
    $(LOCAL_DIR)/MDP2.0/engine/DpEngine_WROT.o \
    $(LOCAL_DIR)/MDP2.0/engine/DpTileEngine.o \
    $(LOCAL_DIR)/MDP2.0/engine/DpEngine_GAMMA.o \
    $(LOCAL_DIR)/MDP2.0/engine/DpEngine_DTH.o \
    #$(LOCAL_DIR)/MDP2.0/engine/DpEngine_WDMA.o \
    $(LOCAL_DIR)/MDP2.0/engine/DpEngine_MDPSOUT.o\
    #$(LOCAL_DIR)/MDP2.0/engine/DpEngine_IMG2O.o \
    $(LOCAL_DIR)/MDP2.0/engine/DpEngine_IMGI.o \
    $(LOCAL_DIR)/common/engine/DpEngine_CAMIN.o \
    $(LOCAL_DIR)/MDP2.0/engine/DpEngine_CCORR.o \
    $(LOCAL_DIR)/MDP2.0/engine/DpEngine_AAL.o \
    #$(LOCAL_DIR)/MDP2.0/engine/DpEngine_WPEO.o \
    $(LOCAL_DIR)/MDP2.0/engine/DpEngine_WPEI.o
    #$(LOCAL_DIR)/MDP2.0/engine/isp/isp8512/DpWrapper_ISP.o \
    $(LOCAL_DIR)/MDP2.0/engine/isp/isp8512/DpWrapper_WPE.o \

#MDP2.0 PQ code
ifeq ($(PQ_SUPPORT), 1)
MDP_LIB_OBJS  += \
    $(LOCAL_DIR)/MDP2.0/engine/pq/PQSession.o \
    $(LOCAL_DIR)/MDP2.0/engine/pq/PQSessionManager.o \
    $(LOCAL_DIR)/MDP2.0/engine/pq/PQConfig.o \
    $(LOCAL_DIR)/MDP2.0/engine/pq/PQDCConfig.o \
    $(LOCAL_DIR)/MDP2.0/engine/pq/PQDSConfig.o \
    $(LOCAL_DIR)/MDP2.0/engine/pq/PQDSAdaptor.o \
    $(LOCAL_DIR)/MDP2.0/engine/pq/PQDCAdaptor.o \
    $(LOCAL_DIR)/MDP2.0/engine/pq/PQAlgorithmAdaptor.o \
    $(LOCAL_DIR)/MDP2.0/engine/pq/PQAlgorithmFactory.o \
    $(LOCAL_DIR)/MDP2.0/engine/pq/PQTuningBuffer.o \
    $(LOCAL_DIR)/MDP2.0/engine/pq/PQAshmem.o \
    $(LOCAL_DIR)/MDP2.0/engine/pq/PQReadBackFactory.o \
    $(LOCAL_DIR)/MDP2.0/engine/pq/PQIspTuning.o
MDP_LIB_OBJS  += \
    $(LOCAL_DIR)/MDP2.0/engine/pq/PQRSZAdaptor.o \
    $(LOCAL_DIR)/MDP2.0/engine/pq/PQRSZConfig.o

ifeq ($(SUPPORT_HDR),true)
MDP_LIB_OBJS += \
    $(LOCAL_DIR)/MDP2.0/engine/pq/PQHDRAdaptor.o \
    $(LOCAL_DIR)/MDP2.0/engine/pq/PQHDRConfig.o
endif
ifeq ($(SUPPORT_DRE),true)
MDP_LIB_OBJS += \
    $(LOCAL_DIR)/MDP2.0/engine/pq/PQDREAdaptor.o \
    $(LOCAL_DIR)/MDP2.0/engine/pq/PQDREConfig.o \
    $(LOCAL_DIR)/MDP2.0/engine/pq/PQDREHistogramAdaptor.o
endif
ifeq ($(MDP_COLOR_ENABLE),true)
MDP_LIB_OBJS += \
    $(LOCAL_DIR)/MDP2.0/engine/pq/PQColorConfig.o \
    $(LOCAL_DIR)/MDP2.0/engine/pq/PQColorAdaptor.o \
    $(LOCAL_DIR)/MDP2.0/engine/color/color6755/DpEngine_COLOR.o
endif
endif

# OSAL: memory proxy layer
MDP_LIB_OBJS  += \
    $(LOCAL_DIR)/common/osal/android/memory/DpMemory_Android.o

# OSAL: Special memory handler
MDP_LIB_OBJS  += \
    $(LOCAL_DIR)/common/osal/android/memory/DpIonHandler.o \
    $(LOCAL_DIR)/common/osal/android/memory/DpMmuHandler.o \
    $(LOCAL_DIR)/common/osal/android/memory/DpPmemHandler.o

# OSAL: Priority manager
MDP_LIB_OBJS  += \
    $(LOCAL_DIR)/MDP2.0/osal/android/driver/DpDriver_Android.o

# OSAL: Get/set property
MDP_LIB_OBJS  += \
    $(LOCAL_DIR)/common/osal/android/property/DpProperty_Android.o

# OSAL: Thread abstraction
MDP_LIB_OBJS  += \
    $(LOCAL_DIR)/common/osal/android/thread/DpThread_Android.o

# OSAL: Sync abstraction
MDP_LIB_OBJS  += \
    $(LOCAL_DIR)/common/osal/android/sync/DpSync_Android.o

# IP dependent part: RSZ
MDP_LIB_OBJS  += \
    $(LOCAL_DIR)/MDP2.0/engine/rsz/rsz6799/DpEngine_RSZ.o

# IP dependent part: tdshp
ifeq ($(PQ_SUPPORT), 1)
MDP_LIB_OBJS  += \
    $(LOCAL_DIR)/MDP2.0/engine/tdshp/tdshp8512/DpEngine_TDSHP.o \
    $(LOCAL_DIR)/MDP2.0/engine/tdshp/tdshp8512/cust_tdshp.o
endif

# Platform dependent part
MDP_LIB_OBJS  += \
    $(LOCAL_DIR)/MDP2.0/platform/mt8512/composer/DpPathTopology.o \
    $(LOCAL_DIR)/MDP2.0/platform/mt8512/engine/DpEngineMutex.o \
    $(LOCAL_DIR)/MDP2.0/platform/mt8512/engine/DpPathConnectionPlatform.o \
#    $(LOCAL_DIR)/MDP2.0/platform/mt8512/engine/DpESLControl.o

# mdp library interface for mdpd
MDP_LIB_OBJS  += \
    $(LOCAL_DIR)/../if/mt8512/mdp_lib_if.o
