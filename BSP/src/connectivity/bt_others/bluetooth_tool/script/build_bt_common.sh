#!/bin/bash

if [ ! $BT_SET_ENVIRONMENT ]; then
    export BT_SET_ENVIRONMENT="yes"
    source ./set_environment.sh
fi

Mw_Include_Path=${Bluetooth_Mw_Dir}/inc

cd ${Bluetooth_Mw_Dir}/common

rm -rf out

gn gen out/Default/ --args="mw_inc_path=\"${Mw_Include_Path}\" cc=\"${CC}\" cxx=\"${CXX}\""
ninja -C out/Default all

if [ ! -d ${Bluetooth_Prebuilts_Dir}/lib ]; then
    mkdir -p ${Bluetooth_Prebuilts_Dir}/lib
fi

cd ${Script_Dir}

if [ -f ${Bluetooth_Mw_Dir}/common/out/Default/libbt-common.so ]; then
    cp ${Bluetooth_Mw_Dir}/common/out/Default/libbt-common.so ${Bluetooth_Prebuilts_Dir}/lib/
else
    exit 1
fi