#!/bin/bash

if [ ! $Bluetooth_Tool_Dir ]; then
    export Bluetooth_Tool_Dir=$1/../src/connectivity/bt_others/bluetooth_tool
fi
if [ ! $Gva_Bluetooth_Prebuilts_Dir ]; then
    export Gva_Bluetooth_Prebuilts_Dir=${Bluetooth_Tool_Dir}/gva_prebuilts
fi
if [ ! $Gva_Bluetooth_Stack_Dir ]; then
    export Gva_Bluetooth_Stack_Dir=${Bluetooth_Tool_Dir}/../../../support/assistant-ctrl/gva/bt
fi
cd ${Bluetooth_Tool_Dir}/script

echo "start clean gva stack"
/bin/bash clean_gva_stack.sh

#echo "start clean gva_btut"
#/bin/bash clean_gva_btut.sh

if [ -d ${Gva_Bluetooth_Prebuilts_Dir} ]; then
    rm -rf ${Gva_Bluetooth_Prebuilts_Dir}
fi
