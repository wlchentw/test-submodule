#!/bin/bash

export MTK_BT_CHIP_ID=$2
export CC=$CC
export CXX=$CXX

echo "start build gva bluetooth"
echo "CXX=${CXX}"

if [ ! $Bluetooth_Tool_Dir ]; then
    export Bluetooth_Tool_Dir=$1/../src/connectivity/bt_others/bluetooth_tool
fi
echo "Bluetooth_Tool_Dir = ${Bluetooth_Tool_Dir}"
if [ ! $Gva_Bluetooth_Stack_Dir ]; then
    export Gva_Bluetooth_Stack_Dir=${Bluetooth_Tool_Dir}/../../../support/assistant-ctrl/gva/bt
fi
if [ ! $GVA_Btut_Stack_Dir ]; then
    export GVA_Btut_Stack_Dir=${Bluetooth_Tool_Dir}/gva_btut/gva_stack
fi
if [ ! $Gva_Bluetooth_Prebuilts_Dir ]; then
    export Gva_Bluetooth_Prebuilts_Dir=${Bluetooth_Tool_Dir}/gva_prebuilts
fi
if [ ! -d ${Gva_Bluetooth_Prebuilts_Dir}/lib/hw ]; then
    mkdir -p ${Gva_Bluetooth_Prebuilts_Dir}/lib/hw
fi
if [ ! -d ${Gva_Bluetooth_Prebuilts_Dir}/lib/gva_btut ]; then
    mkdir -p ${Gva_Bluetooth_Prebuilts_Dir}/lib/gva_btut
fi
if [ ! -d ${Gva_Bluetooth_Prebuilts_Dir}/bin ]; then
    mkdir -p ${Gva_Bluetooth_Prebuilts_Dir}/bin
fi
if [ ! -d ${Gva_Bluetooth_Prebuilts_Dir}/conf ]; then
    mkdir -p ${Gva_Bluetooth_Prebuilts_Dir}/conf
fi

if [ "$MTK_BT_CHIP_ID" = "mt7668" ]; then
   export BT_PERFORMANCE_ANALYSIS_FLAG=-DMTK_BT_PERFORMANCE_ANALYSIS
fi

cd ${Bluetooth_Tool_Dir}/script

echo "start clean gva bluetooth"
/bin/bash clean_gva_bluetooth.sh

echo "start build gva stack"
/bin/bash build_gva_stack.sh

#echo "start build gva_btut"
#/bin/bash build_gva_btut.sh

