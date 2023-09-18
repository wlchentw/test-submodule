#!/bin/bash

if [ $1 ]; then
    BT_Tool_Dir=$1/../src/connectivity/bt_others/bluetooth_tool
else
    echo "should input TOPDIR"
    exit 1
fi
if [ $2 ]; then
    typeset -l MTK_BT_CHIP_ID
    export MTK_BT_CHIP_ID=$2
else
    echo "should input COMBO_CHIP_ID"
    exit 1
fi
if [ $3 ]; then
    export MTK_BT_PROJECT=$3
else
    echo "should input MTK_PROJECT"
    exit 1
fi
if [ $4 ]; then
    echo "RECIPE_SYSROOT:$4"
else
    echo "should input RECIPE_SYSROOT"
    exit 1
fi

if [ $5 ]; then
    echo "MTK_BT_C4A:$5"
    export MTK_BT_C4A=$5
else
    export MTK_BT_C4A="no"
    echo "if not exist, MTK_BT_C4A equal to no"
fi

#export MTK_BT_C4A="no"
export CC=$CC
export CXX=$CXX
export BT_GET_CROSS_COMPILE="yes"
if [ -d $1/tmp/sysroots/${MTK_BT_PROJECT}/usr/include/alsa ]; then
    export Asound_Inc_Path=$1/tmp/sysroots/${MTK_BT_PROJECT}/usr/include/alsa
else
    export Asound_Inc_Path=$4/${includedir}/alsa
fi

echo $PWD
echo hello yocto bluetooth
echo ${BT_Tool_Dir}

cd ${BT_Tool_Dir}/script
echo $PWD

if [ "$ENABLE_SYS_LOG" = "yes" ]; then
    export BT_SYS_LOG_FLAG=-DMTK_BT_SYS_LOG
    echo "enable sys log:$ENABLE_SYS_LOG"
else
    echo "not disable sys log:$ENABLE_SYS_LOG"
fi

if [ "${MTK_BT_CHIP_ID:0:4}" = "mt76" ]; then
export BT_PERFORMANCE_ANALYSIS_FLAG=-DMTK_BT_PERFORMANCE_ANALYSIS
fi

export BT_PLAYBACK_ALSA_flag=""

echo ${MTK_BT_PROJECT}
if [ ! $BT_SET_ENVIRONMENT ]; then
    export BT_SET_ENVIRONMENT="yes"
    source ${BT_Tool_Dir}/script/set_environment.sh
fi

/bin/bash clean_all_rpc.sh

if [ "$SUPPORT_AAC" = "yes" ]; then
    cp $1/../src/multimedia/audio-misc/libaac/libAACdec/include/aacdecoder_lib.h ${BT_Tool_Dir}/external_libs/platform/include/
    cp $1/../src/multimedia/audio-misc/libaac/libAACenc/include/aacenc_lib.h ${BT_Tool_Dir}/external_libs/platform/include/
    cp -r $1/../src/multimedia/audio-misc/libaac/libSYS/include/*.h ${BT_Tool_Dir}/external_libs/platform/include/
fi

if [ "$SUPPORT_STEREO" = "yes" ]; then
    cp $1/../src/multimedia/audio-misc/multi-room/stereo_player/stereo_app.h ${BT_Tool_Dir}/external_libs/platform/include/
fi

if [ -d $1/tmp/sysroots/${MTK_BT_PROJECT}/usr/include/alsa ]; then
    cp $1/tmp/sysroots/${MTK_BT_PROJECT}/${libdir}/libasound.so ${BT_Tool_Dir}/external_libs/platform/
    cp $1/tmp/sysroots/${MTK_BT_PROJECT}/${libdir}/libz.so ${BT_Tool_Dir}/external_libs/platform/
else
    cp $4/${libdir}/libasound.so ${BT_Tool_Dir}/external_libs/platform/
    cp $4/${libdir}/libz.so ${BT_Tool_Dir}/external_libs/platform/
fi

/bin/bash build_all_rpc.sh
