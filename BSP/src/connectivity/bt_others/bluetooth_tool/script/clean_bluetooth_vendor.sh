#!/bin/bash

if [ ! $VENDOR_BT_SET_ENVIRONMENT ]; then
    export VENDOR_BT_SET_ENVIRONMENT="yes"
    source ./vendor_set_environment.sh
fi

echo "Bluetooth_Tool_Dir = ${Bluetooth_Tool_Dir}"
echo "VENDOR_LIBRARY = $VENDOR_LIBRARY"
echo "MTK_BT_CHIP_ID = $MTK_BT_CHIP_ID"

cd ${Bluetooth_Tool_Dir}/script

echo "start clean vendor lib"
/bin/bash clean_${VENDOR_LIBRARY}

echo "start clean boots"
/bin/bash clean_boots.sh
if [ "$MTK_BT_CHIP_ID" = "mt7668" ]; then
    /bin/bash clean_picus.sh
fi

if [ -d ${Bluetooth_Vendor_Prebuilts_Dir} ]; then
    rm -rf ${Bluetooth_Vendor_Prebuilts_Dir}
fi
