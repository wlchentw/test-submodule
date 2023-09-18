#!/bin/bash

##############################################################################################

Libhw_Include_Path=${GVA_Btut_Stack_Dir}/libhardware/include
Core_Include_Path=${GVA_Btut_Stack_Dir}/../../gva_core/include
Btif_Include_Path=${GVA_Btut_Stack_Dir}/btif/include
Stack_Include_Path=${GVA_Btut_Stack_Dir}/stack/include
Utils_Include_Path=${GVA_Btut_Stack_Dir}/utils/include
Mtk_Bt_Include_Path=${GVA_Btut_Stack_Dir}/mediatek/include
if [ ! $GVA_Btut_Lib_Path ]; then
    GVA_Btut_Lib_Path=${Gva_Bluetooth_Prebuilts_Dir}/lib/gva_btut
fi

cd ${Bluetooth_Tool_Dir}
if [ ! -d "gva_core" ]; then
    tar -xf gva_core.tar.gz
fi

cd ${Bluetooth_Tool_Dir}/gva_btut

rm -rf out

gn gen out/Default/ --args="libhw_include_path=\"${Libhw_Include_Path}\" core_include_path=\"${Core_Include_Path}\" btif_include_path=\"${Btif_Include_Path}\" stack_include_path=\"${Stack_Include_Path}\" utils_include_path=\"${Utils_Include_Path}\" mtk_bt_include_path=\"${Mtk_Bt_Include_Path}\" gva_btut_lib_path=\"-L${GVA_Btut_Lib_Path}\" cc=\"${CC}\" cxx=\"${CXX}\""
ninja -C out/Default all

if [ ! -d ${Gva_Bluetooth_Prebuilts_Dir}/bin ]; then
    mkdir -p ${Gva_Bluetooth_Prebuilts_Dir}/bin
fi
if [ -f ${Bluetooth_Tool_Dir}/gva_btut/out/Default/gva_btut ]; then
    cp ${Bluetooth_Tool_Dir}/gva_btut/out/Default/gva_btut ${Gva_Bluetooth_Prebuilts_Dir}/bin/
else
    exit 1
fi