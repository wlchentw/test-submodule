#!/bin/bash

Gva_Core_Include_Path=${Bluetooth_Tool_Dir}/gva_core/include
Gva_Glibc_Bridge_Include_Path=${Bluetooth_Tool_Dir}/gva_core/glibc_bridge/include

if [ "$SUPPORT_GVA_AAC" = "yes" ]; then
    ENABLE_AAC_FLAG="-DENABLE_CODEC_AAC"
else
    ENABLE_AAC_FLAG=""
fi

cd ${Bluetooth_Tool_Dir}
if [ ! -d "gva_core" ]; then
    tar -xf gva_core.tar.gz
fi

cd ${Gva_Bluetooth_Stack_Dir}
rm -rf out

gn gen out/Default/ --args="gva_core_include_path=\"${Gva_Core_Include_Path}\" gva_glibc_bridge_include_path=\"${Gva_Glibc_Bridge_Include_Path}\" bt_performance_analysis_flag=\"${BT_PERFORMANCE_ANALYSIS_FLAG}\" enable_aac=\"${ENABLE_AAC_FLAG}\" cc=\"${CC}\" cxx=\"${CXX}\""
ninja -C out/Default all

if [ ! -d ${Gva_Bluetooth_Prebuilts_Dir}/bin ]; then
    mkdir -p ${Gva_Bluetooth_Prebuilts_Dir}/bin
fi
cp ${Gva_Bluetooth_Stack_Dir}/out/Default/bluetoothtbd ${Gva_Bluetooth_Prebuilts_Dir}/bin/

if [ ! -d ${Gva_Bluetooth_Prebuilts_Dir}/conf ]; then
    mkdir -p ${Gva_Bluetooth_Prebuilts_Dir}/conf
fi
cp ${Gva_Bluetooth_Stack_Dir}/conf/bt_did.conf ${Gva_Bluetooth_Prebuilts_Dir}/conf/
cp ${Gva_Bluetooth_Stack_Dir}/conf/bt_stack.conf ${Gva_Bluetooth_Prebuilts_Dir}/conf/

if [ ! -d ${Gva_Bluetooth_Prebuilts_Dir}/lib/hw ]; then
    mkdir -p ${Gva_Bluetooth_Prebuilts_Dir}/lib/hw
fi
cp ${Gva_Bluetooth_Stack_Dir}/out/Default/bluetooth.default.so ${Gva_Bluetooth_Prebuilts_Dir}/lib/hw/

if [ ! -d ${Gva_Bluetooth_Prebuilts_Dir}/lib/gva_btut ]; then
    mkdir -p ${Gva_Bluetooth_Prebuilts_Dir}/lib/gva_btut
fi
cp ${Gva_Bluetooth_Stack_Dir}/out/Default/bluetooth.default.so ${Gva_Bluetooth_Prebuilts_Dir}/lib/gva_btut/libbluetooth.default.so