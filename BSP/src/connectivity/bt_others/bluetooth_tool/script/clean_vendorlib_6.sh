#!/bin/bash

if [ ! $VENDOR_BT_SET_ENVIRONMENT ]; then
    export VENDOR_BT_SET_ENVIRONMENT="yes"
    source ./vendor_set_environment.sh
fi

rm -rf ${Bluetooth_Vendor_Prebuilts_Dir}/lib/libbt-vendor.so
rm -rf ${Bluetooth_Vendor_Prebuilts_Dir}/lib/libbluetooth_hw_test.so
rm -rf ${Bluetooth_Vendor_Prebuilts_Dir}/lib/libbluetooth_mtk_pure.so
rm -rf ${Bluetooth_Vendor_Prebuilts_Dir}/lib/libbluetooth_relayer.so
rm -rf ${Bluetooth_Vendor_Prebuilts_Dir}/lib/libbluetoothem_mtk.so
rm -rf ${Bluetooth_Vendor_Prebuilts_Dir}/bin/autobt

cd ${Bluetooth_Tool_Dir}
rm -rf libhardware

cd ${Bluetooth_Vendor_Lib_Dir}
rm -rf out
rm -rf mtk/bluedroid/external/platform/CFG_BT_Default.h
rm -rf mtk/bluedroid/external/platform/CFG_BT_File.h
rm -rf mtk/bluedroid/external/platform/CFG_file_lid.h
rm -rf mtk/bluedroid/external/platform/libfile_op.so
rm -rf mtk/bluedroid/external/platform/libnvram.so
rm -rf mtk/bluedroid/external/platform/libnvram_custom.so

cd ${Script_Dir}

