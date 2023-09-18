#!/bin/bash

if [ ! $BT_SET_ENVIRONMENT ]; then
    export BT_SET_ENVIRONMENT="yes"
    source ./set_environment.sh
fi

Libhw_Include_Path=${Bluetooth_Tool_Dir}/libhardware/include
Core_Include_Path=${Bluetooth_Tool_Dir}/core/include
Audio_Include_Path=${Bluetooth_Tool_Dir}/media/audio/include
Zlib_Include_Path=${Bluetooth_Tool_Dir}/zlib-1.2.8

if [ "$SUPPORT_AAC" = "yes" ]; then
    ENABLE_AAC_FLAG="-DENABLE_CODEC_AAC"
    AAC_LINK="-laac"
    AAC_Include_Path=${Bluetooth_Tool_Dir}/external_libs/platform/include
else
    ENABLE_AAC_FLAG=""
    AAC_LINK=""
    AAC_Include_Path=${Bluetooth_Tool_Dir}/external_libs/platform/include
fi

if [ "$SUPPORT_HFP_CLIENT" = "yes" ]; then
    ENABLE_HFP_FLAG="-DENABLE_HFP_CLIENT"

    if [ "$SUPPORT_HFP_PHONEBOOK" = "yes" ]; then
        ENABLE_HFP_PHONEBOOK_FLAG="-DENABLE_HFP_PHONEBOOK"
    else
        ENABLE_HFP_PHONEBOOK_FLAG=""
	fi
else
    ENABLE_HFP_FLAG=""
    ENABLE_HFP_PHONEBOOK_FLAG=""
fi

if [ "$SUPPORT_STEREO" = "yes" ]; then
    ENABLE_STEREO_FLAG="-DENABLE_FEATURE_STEREO"
else
    ENABLE_STEREO_FLAG=""
fi

if [ "$SUPPORT_MULTI_POINT" = "yes" ]; then
    ENABLE_MULTI_POINT_FLAG="-DENABLE_FEATURE_MULTI_POINT"
else
    ENABLE_MULTI_POINT_FLAG=""
fi
Zlib_Path=${External_Libs_Path}

echo ${Bluetooth_Tool_Dir}
cd ${Bluetooth_Tool_Dir}

if [ ! -d "zlib-1.2.8" ]; then
    tar -xf zlib.tar.gz
    if [ ! -f ${External_Libs_Path}/libz.so ]; then
        echo "use local libz.so"
        cd zlib-1.2.8
        export CC=${CC}
        ./configure && make
        cp ${Bluetooth_Tool_Dir}/zlib-1.2.8/libz.so.1 ${External_Libs_Path}/
        cp ${Bluetooth_Tool_Dir}/zlib-1.2.8/libz.so ${External_Libs_Path}/
        cd ..
    else
        echo "use platform libz.so"
    fi
fi

if [ ! -d "core" ]; then
    tar -xf core.tar.gz
fi

if [ ! -d "libhardware" ]; then
    tar -xf libhardware.tar.gz
fi

if [ ! -d "media" ]; then
    tar -xf media.tar.gz
fi

cd ${Bluetooth_Stack_Dir}

if [ ! -d "third_party" ]; then
    tar -xf third_party.tar.gz
fi

rm -rf out

gn gen out/Default/ --args="libhw_include_path=\"${Libhw_Include_Path}\" core_include_path=\"${Core_Include_Path}\" audio_include_path=\"${Audio_Include_Path}\" zlib_include_path=\"${Zlib_Include_Path}\" zlib_path=\"-L${Zlib_Path}\" conf_path=\"${Conf_Path}\" cache_path=\"${Cache_Path}\" bt_tmp_path=\"${BT_Tmp_Path}\" bt_misc_path=\"${BT_Misc_Path}\" bt_etc_path=\"${BT_Etc_Path}\" rpc_dbg_flag=\"${Rpc_Dbg_Flag}\" cc=\"${CC}\" cxx=\"${CXX}\" bt_sys_log_flag=\"${BT_SYS_LOG_FLAG}\" bt_performance_analysis_flag=\"${BT_PERFORMANCE_ANALYSIS_FLAG}\" enable_aac=\"${ENABLE_AAC_FLAG}\" aac_include_path=\"${AAC_Include_Path}\" aac_link=\"${AAC_LINK}\" enable_stereo=\"${ENABLE_STEREO_FLAG}\" enable_multi_point=\"${ENABLE_MULTI_POINT_FLAG}\" enable_hfp=\"${ENABLE_HFP_FLAG}\" enable_hfp_phonebook=\"${ENABLE_HFP_PHONEBOOK_FLAG}\""

ninja -C out/Default all

if [ ! -d ${Bluetooth_Prebuilts_Dir}/lib ]; then
    mkdir -p ${Bluetooth_Prebuilts_Dir}/lib
fi

if [ ! -d ${Bluetooth_Prebuilts_Dir}/conf ]; then
    mkdir -p ${Bluetooth_Prebuilts_Dir}/conf
fi

cd ${Script_Dir}


if [ -f ${Bluetooth_Stack_Dir}/out/Default/libbluetooth.default.so ]; then
    cp ${Bluetooth_Stack_Dir}/out/Default/libbluetooth.default.so ${Bluetooth_Prebuilts_Dir}/lib/
    cp ${Bluetooth_Stack_Dir}/conf/bt_stack.conf.linux ${Bluetooth_Prebuilts_Dir}/conf/bt_stack.conf
    cp ${Bluetooth_Stack_Dir}/conf/bt_did.conf ${Bluetooth_Prebuilts_Dir}/conf/bt_did.conf
else
    exit 1
fi
if [ -f ${Bluetooth_Stack_Dir}/out/Default/libaudio.a2dp.default.so ]; then
    cp ${Bluetooth_Stack_Dir}/out/Default/libaudio.a2dp.default.so ${Bluetooth_Prebuilts_Dir}/lib/
else
    exit 1
fi
