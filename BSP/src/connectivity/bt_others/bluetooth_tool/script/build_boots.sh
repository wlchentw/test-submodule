#!/bin/bash

if [ ! $VENDOR_BT_SET_ENVIRONMENT ]; then
    export VENDOR_BT_SET_ENVIRONMENT="yes"
    source ./vendor_set_environment.sh
fi

Boots_Socket_Path=$BT_Misc_Path
Boots_Vcom_Opened=$BT_Vcom_Opened

cd ${Bluetooth_Boots_Dir}

rm -rf out

gn gen out/Default/ --args="boots_socket_path = \"${Boots_Socket_Path}\" boots_vcom_opened = \"${Boots_Vcom_Opened}\" cc=\"${CC}\" cxx=\"${CXX}\""
ninja -C out/Default all

cd ${Script_Dir}


if [ ! -d ${Bluetooth_Vendor_Prebuilts_Dir}/bin ]; then
    mkdir -p ${Bluetooth_Vendor_Prebuilts_Dir}/bin
fi

if [ -f ${Bluetooth_Boots_Dir}/out/Default/boots_srv ]; then
    cp ${Bluetooth_Boots_Dir}/out/Default/boots_srv ${Bluetooth_Vendor_Prebuilts_Dir}/bin/
else
    exit 1
fi

if [ -f ${Bluetooth_Boots_Dir}/out/Default/boots ]; then
    cp ${Bluetooth_Boots_Dir}/out/Default/boots ${Bluetooth_Vendor_Prebuilts_Dir}/bin/
else
    exit 1
fi
