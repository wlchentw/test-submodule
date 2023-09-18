#!/bin/bash

if [ ! $VENDOR_BT_SET_ENVIRONMENT ]; then
    export VENDOR_BT_SET_ENVIRONMENT="yes"
    source ./vendor_set_environment.sh
fi

rm -rf ${Bluetooth_Vendor_Prebuilts_Dir}/lib/libbt-vendor.so

cd ${Bluetooth_Vendor_Lib_Dir}/
rm -rf BUILD.gn
rm -rf .gn
rm -rf build
rm -rf external
rm -rf out
cd ${Script_Dir}