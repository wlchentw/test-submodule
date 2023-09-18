#!/bin/bash

if [ ! $BT_SET_ENVIRONMENT ]; then
    export BT_SET_ENVIRONMENT="yes"
    source ./set_environment.sh
fi

Mw_Inc_Path=${Bluetooth_Mw_Dir}/sdk/inc
Stack_Include_Path=${Bluetooth_Stack_Dir}/mediatek/include
Stack_Root_Path=${Bluetooth_Stack_Dir}
Libasound_Path=${External_Libs_Path}

echo "TARGET_PRODUCT = $TARGET_PRODUCT"
echo "MTK_BT_PROJECT = $MTK_BT_PROJECT"

if [ "$SUPPORT_STEREO" = "yes" ]; then
    ENABLE_STEREO_FLAG="YES"
    STEREO_LINK="-lstereo"
    STEREO_Include_Path=${Bluetooth_Tool_Dir}/external_libs/platform/include
else
    ENABLE_STEREO_FLAG="NO"
    STEREO_LINK=""
    STEREO_Include_Path=$Mw_Inc_Path
fi

if [ "$SUPPORT_IPDC" = "yes" ]; then
    IPCD_LINK="-lipcd"
    ENABLE_IPCD_FLAG="-DENABLE_IPCD"
else
    IPCD_LINK=""
    ENABLE_IPCD_FLAG=""
fi

cd ${Bluetooth_Mw_Dir}/playback

rm -rf out

gn gen out/Default/ --args="stack_include_path=\"${Stack_Include_Path}\" stack_root_path=\"${Stack_Root_Path}\" asound_inc_path=\"${Asound_Inc_Path}\" mw_inc_path=\"${Mw_Inc_Path}\" libasound_path=\"-L${Libasound_Path}\" bluedroid_libs_path=\"-L${Bluedroid_Libs_Path}\" bt_sys_log_flag=\"${BT_SYS_LOG_FLAG}\" bt_tmp_path=\"${BT_Tmp_Path}\" enable_ipcd=\"${ENABLE_IPCD_FLAG}\" ipcd_link=\"${IPCD_LINK}\" bt_misc_path=\"${BT_Misc_Path}\" bt_etc_path=\"${BT_Etc_Path}\" cc=\"${CC}\" cxx=\"${CXX}\" enable_stereo=\"${ENABLE_STEREO_FLAG}\" libbt_stereo_link=\"${STEREO_LINK}\" stereo_include_path=\"${STEREO_Include_Path}\""
ninja -C out/Default all

cd ${Script_Dir}

if [ ! -d ${Bluetooth_Prebuilts_Dir}/lib ]; then
    mkdir -p ${Bluetooth_Prebuilts_Dir}/lib
fi

if [ -f ${Bluetooth_Mw_Dir}/playback/out/Default/libbt-alsa-playback.so ]; then
    cp ${Bluetooth_Mw_Dir}/playback/out/Default/libbt-alsa-playback.so ${Bluetooth_Prebuilts_Dir}/lib/
else
    exit 1
fi
if [ -f ${Bluetooth_Mw_Dir}/playback/out/Default/libbt-stereo-playback.so ]; then
    cp ${Bluetooth_Mw_Dir}/playback/out/Default/libbt-stereo-playback.so ${Bluetooth_Prebuilts_Dir}/lib/
else
    exit 1
fi
