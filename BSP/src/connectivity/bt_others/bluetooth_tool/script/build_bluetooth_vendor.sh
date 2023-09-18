#!/bin/bash

if [ ! $VENDOR_BT_SET_ENVIRONMENT ]; then
    export VENDOR_BT_SET_ENVIRONMENT="yes"
    source ./vendor_set_environment.sh
fi

if [ "${MTK_BT_CHIP_ID:0:4}" = "mt76" ]; then
    export BT_PERFORMANCE_ANALYSIS_FLAG=-DMTK_BT_PERFORMANCE_ANALYSIS
fi

if [ "${MTK_BT_VCOM_OPENED}" = "yes" ]; then
    export BT_Vcom_Opened=-DVCOM_OPENED
fi

cd ${Bluetooth_Tool_Dir}/script

echo "start build vendor lib"
/bin/bash build_${VENDOR_LIBRARY} $MTK_BT_CHIP_ID
if [ $? -ne 0 ]; then
    echo vendor_lib compile fail!!
    exit 1
fi

if [ "${MTK_BT_CHIP_ID:0:4}" = "mt76" ]; then
    echo "start build boots"
    /bin/bash build_boots.sh
    if [ ! -f ${Bluetooth_Vendor_Prebuilts_Dir}/bin/boots_srv ]; then
        echo build boots_srv failed, EXIT!
        exit 1
    fi
    if [ ! -f ${Bluetooth_Vendor_Prebuilts_Dir}/bin/boots ]; then
        echo build boots failed, EXIT!
        exit 1
    fi
    echo "start build picus"
    /bin/bash build_picus.sh
    if [ ! -f ${Bluetooth_Vendor_Prebuilts_Dir}/bin/picus ]; then
        echo build picus failed, EXIT!
        exit 1
    fi
fi
