#!/bin/bash

if [ ! $VENDOR_BT_SET_ENVIRONMENT ]; then
    export VENDOR_BT_SET_ENVIRONMENT="yes"
    source ./vendor_set_environment.sh
fi

cd ${Bluetooth_Picus_Dir}

rm -rf out

gn gen out/Default/ --args="cc=\"${CC}\" cxx=\"${CXX}\""
ninja -C out/Default all

cd ${Script_Dir}

if [ ! -d ${Bluetooth_Vendor_Prebuilts_Dir}/bin ]; then
    mkdir -p ${Bluetooth_Vendor_Prebuilts_Dir}/bin
fi

if [ -f ${Bluetooth_Picus_Dir}/out/Default/picus ]; then
    cp ${Bluetooth_Picus_Dir}/out/Default/picus ${Bluetooth_Vendor_Prebuilts_Dir}/bin/
else
    exit 1
fi
