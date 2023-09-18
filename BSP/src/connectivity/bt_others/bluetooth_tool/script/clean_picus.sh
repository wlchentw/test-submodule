#!/bin/bash

if [ ! $VENDOR_BT_SET_ENVIRONMENT ]; then
    export VENDOR_BT_SET_ENVIRONMENT="yes"
    source ./vendor_set_environment.sh
fi

rm -rf ${Bluetooth_Vendor_Prebuilts_Dir}/bin/picus

cd ${Bluetooth_Picus_Dir}/
rm -rf out
cd ${Script_Dir}