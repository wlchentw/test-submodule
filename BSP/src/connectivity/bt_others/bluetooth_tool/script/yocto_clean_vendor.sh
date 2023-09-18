#!/bin/bash

export BT_GET_CROSS_COMPILE="yes"
if [ $1 ]; then
    export Bluetooth_Tool_Dir=$1/../src/connectivity/bt_others/bluetooth_tool
fi
if [ $2 ]; then
    export MTK_BT_CHIP_ID=$2
fi

if [ ! $VENDOR_BT_SET_ENVIRONMENT ]; then
    export VENDOR_BT_SET_ENVIRONMENT="yes"
    source ${Bluetooth_Tool_Dir}/script/vendor_set_environment.sh
fi

cd ${Bluetooth_Tool_Dir}/script

echo "start clean bluetooth vendor"
/bin/bash clean_bluetooth_vendor.sh

rm -r ${Script_Dir}/vendor_set_environment.sh