#!/bin/bash

if [ ! $BT_SET_ENVIRONMENT ]; then
    export BT_SET_ENVIRONMENT="yes"
    source ./set_environment.sh
fi

rm -rf ${Bluetooth_Tool_Dir}/prebuilts/lib/libbt-mw.so

cd ${Bluetooth_Mw_Dir}/sdk
rm -rf out
cd ${Script_Dir}