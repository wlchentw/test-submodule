#!/bin/bash

if [ ! $VENDOR_BT_SET_ENVIRONMENT ]; then
    export VENDOR_BT_SET_ENVIRONMENT="yes"
    source ./vendor_set_environment.sh
fi

rm -rf ${Bluetooth_Vendor_Prebuilts_Dir}/bin/boots_srv
rm -rf ${Bluetooth_Vendor_Prebuilts_Dir}/bin/boots


cd ${Bluetooth_Boots_Dir}

rm -rf out

cd ${Script_Dir}
