#!/bin/bash

if [ ! $1 ]; then
    echo use default chip id:mt8167
    export MTK_BT_CHIP_ID=mt8167
else
    export MTK_BT_CHIP_ID=$1
fi

if [ ! $Script_Dir ]; then
    Script_Dir=$(pwd)
fi
if [ ! $BT_Tool_Dir ]; then
    export BT_Tool_Dir=${Script_Dir}/..
fi

echo "PWD:$(pwd)"
echo $BT_Tool_Dir
export ENABLE_SYS_LOG="no"
export MTK_BT_C4A="no"
export MTK_BT_YOCTOR="no"
export CC=/mtkoss/gnuarm/hard_4.9.2-r116_armv7a-cros/x86_64/armv7a/usr/x86_64-pc-linux-gnu/armv7a-cros-linux-gnueabi/gcc-bin/4.9.x-google/armv7a-cros-linux-gnueabi-gcc
export CXX=/mtkoss/gnuarm/hard_4.9.2-r116_armv7a-cros/x86_64/armv7a/usr/x86_64-pc-linux-gnu/armv7a-cros-linux-gnueabi/gcc-bin/4.9.x-google/armv7a-cros-linux-gnueabi-g++

if [ "$ENABLE_SYS_LOG" = "yes" ]; then
    export BT_SYS_LOG_FLAG=-DMTK_BT_SYS_LOG
    echo "enable sys log:$ENABLE_SYS_LOG"
else
    echo "not disable sys log:$ENABLE_SYS_LOG"
fi
if [ "$MTK_BT_CHIP_ID" = "mt7668" ]; then
    export BT_PERFORMANCE_ANALYSIS_FLAG=-DMTK_BT_PERFORMANCE_ANALYSIS
fi

export SUPPORT_AAC="no"
export SUPPORT_SPP="no"
export SUPPORT_HIDH="no"
export SUPPORT_HIDD="no"
export SUPPORT_GATT="yes"
export SUPPORT_AVRCP="yes"
export SUPPORT_A2DP_SRC="yes"
export SUPPORT_A2DP_ADEV="yes"
export SUPPORT_A2DP_SINK="yes"
export SUPPORT_HFP_CLIENT="yes"
export SUPPORT_BT_WIFI_RATIO_SETTING="yes"
export SUPPORT_DISPATCH_A2DP_WITH_PLAYBACK="yes"
export SUPPORT_BLE_MESH="no"
export SUPPORT_BLE_MESH_HEARTBEAT="no"

#1st phase:build bluetooth vendor lib and boots, picus common tools(vendor lib, tools)
FOR_BT_VENDOR="yes"
/bin/bash generate_environment.sh ${MTK_BT_CHIP_ID} ${FOR_BT_VENDOR}

if [ ! $VENDOR_BT_SET_ENVIRONMENT ]; then
    export VENDOR_BT_SET_ENVIRONMENT="yes"
    source ./vendor_set_environment.sh
fi

echo "start clean bluetooth vendor_lib"
/bin/bash clean_bluetooth_vendor.sh

echo "start build bluetooth vendor"
if [ "$MTK_BT_CHIP_ID" = "mt8167"  -o "$MTK_BT_CHIP_ID" = "mt6631" ]; then
    cp ${BT_Tool_Dir}/vendor_libs/mtk/bluedroid/external/consys_CFG_BT_Default.h ${BT_Tool_Dir}/vendor_libs/mtk/bluedroid/external/platform/CFG_BT_Default.h
elif [ "$MTK_BT_CHIP_ID" = "mt6630" ]; then
    cp ${BT_Tool_Dir}/vendor_libs/mtk/bluedroid/external/mt6630_CFG_BT_Default.h ${BT_Tool_Dir}/vendor_libs/mtk/bluedroid/external/platform/CFG_BT_Default.h
fi

if [ "$MTK_BT_CHIP_ID" = "mt8167" -o "$MTK_BT_CHIP_ID" = "mt6630" -o "$MTK_BT_CHIP_ID" = "mt6631" ]; then
    cp ${BT_Tool_Dir}/vendor_libs/mtk/bluedroid/external/CFG_BT_File.h ${BT_Tool_Dir}/vendor_libs/mtk/bluedroid/external/platform/
    cp ${BT_Tool_Dir}/vendor_libs/mtk/bluedroid/external/CFG_file_lid.h ${BT_Tool_Dir}/vendor_libs/mtk/bluedroid/external/platform/
fi

/bin/bash build_bluetooth_vendor.sh
if [ $? -ne 0 ]; then
    echo bluetooth vendor compile fail!!
    exit 1
fi

#2nd phase:build all other bluetooth related library and binary(mw, stack)
FOR_BT_VENDOR="no"
/bin/bash generate_environment.sh ${MTK_BT_CHIP_ID} ${FOR_BT_VENDOR}

if [ ! $BT_SET_ENVIRONMENT ]; then
    export BT_SET_ENVIRONMENT="yes"
    source ./set_environment.sh
fi

export Asound_Inc_Path=${Bluetooth_Mw_Dir}/playback/include

/bin/bash clean_all_rpc.sh

cp ${BT_Tool_Dir}/external_libs/libasound.so ${BT_Tool_Dir}/external_libs/platform/
#cp ${BT_Tool_Dir}/external_libs/libz.so ${BT_Tool_Dir}/external_libs/platform/

/bin/bash build_all_rpc.sh