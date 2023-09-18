#!/bin/bash

if [ ! $1 ]; then
    echo use default chip id:mt7662
    export MTK_BT_CHIP_ID=mt7662
else
    export MTK_BT_CHIP_ID=$1
fi

if [ ! $2 ]; then
    export FOR_BT_VENDOR="no"
else
    export FOR_BT_VENDOR=$2
fi

export ENABLE_SYS_LOG="yes"
export SUPPORT_AAC="no"
export SUPPORT_SPP="yes"
export SUPPORT_HIDH="yes"
export SUPPORT_HIDD="yes"
export SUPPORT_GATT="yes"
export SUPPORT_AVRCP="yes"
export SUPPORT_A2DP_SRC="yes"
export SUPPORT_A2DP_ADEV="no"
export SUPPORT_A2DP_SINK="yes"
export SUPPORT_HFP_CLIENT="yes"
export SUPPORT_BT_WIFI_RATIO_SETTING="yes"
export SUPPORT_DISPATCH_A2DP_WITH_PLAYBACK="yes"
export SUPPORT_BLE_MESH="no"
export SUPPORT_BLE_MESH_HEARTBEAT="no"
export SUPPORT_STEREO="no"
export SUPPORT_MULTI_POINT="yes"
export SUPPORT_IPDC="no"
export SUPPORT_BLE_MESH="no"
export SUPPORT_BLE_MESH_HEARTBEAT="no"

if [ ! $Script_Dir ]; then
    Script_Dir=$(pwd)
fi
if [ ! $BT_Tool_Dir ]; then
    export BT_Tool_Dir=${Script_Dir}/..
fi

echo $BT_Tool_Dir
export MTK_BT_C4A="no"
export MTK_BT_YOCTOR="no"
export CC=/mtkoss/gnuarm/hard_4.9.2-r116_armv7a-cros/x86_64/armv7a/usr/x86_64-pc-linux-gnu/armv7a-cros-linux-gnueabi/gcc-bin/4.9.x-google/armv7a-cros-linux-gnueabi-gcc
export CXX=/mtkoss/gnuarm/hard_4.9.2-r116_armv7a-cros/x86_64/armv7a/usr/x86_64-pc-linux-gnu/armv7a-cros-linux-gnueabi/gcc-bin/4.9.x-google/armv7a-cros-linux-gnueabi-g++

/bin/bash generate_environment.sh  ${MTK_BT_CHIP_ID} ${FOR_BT_VENDOR}

if [ ! $BT_SET_ENVIRONMENT ]; then
    export BT_SET_ENVIRONMENT="yes"
    source ./set_environment.sh
fi

export Asound_Inc_Path=${Bluetooth_Mw_Dir}/playback/include

/bin/bash clean_all_rpc.sh

cp ${Bluetooth_Tool_Dir}/external_libs/libasound.so ${Bluetooth_Tool_Dir}/external_libs/platform/

echo "start trunk build"
/bin/bash build_all_rpc.sh
if [ $? -ne 0 ]; then
    echo build_all_rpc compile fail!!
    exit 1
fi

echo "trunk build success"
