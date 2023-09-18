#!/bin/bash

if [ ! $BT_SET_ENVIRONMENT ]; then
    export BT_SET_ENVIRONMENT="yes"
    source ./set_environment.sh
fi

rm -rf ${Bluetooth_Tool_Dir}/prebuilts/bin/btut

cd ${Bluetooth_Tool_Dir}/btut
rm -rf out
cd ${Script_Dir}
