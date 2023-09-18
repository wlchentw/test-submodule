#!/bin/bash

if [ $1 ]; then
    export Bluetooth_Tool_Dir=$1/../src/connectivity/bt_others/bluetooth_tool
else
    echo "should input TOPDIR"
    exit 1
fi
if [ $2 ]; then
    typeset -l MTK_BT_CHIP_ID
    export MTK_BT_CHIP_ID=$2
else
    echo "should input COMBO_CHIP_ID"
    exit 1
fi
if [ $3 ]; then
    export MTK_BT_PROJECT=$3
    echo MTK_BT_PROJECT=${MTK_BT_PROJECT}
else
    echo "should input MTK_PROJECT"
    exit 1
fi
if [ $4 ]; then
    echo "RECIPE_SYSROOT:$4"
else
    echo "should input RECIPE_SYSROOT"
    exit 1
fi

#default nvram is enabled for mt66xx, for special project, nvram may not supported
if [ $5 = "no" ]; then
    export BT_VENDOR_NVRAM_ENABLE="no"
else
    export BT_VENDOR_NVRAM_ENABLE="yes"
fi
echo BT_VENDOR_NVRAM_ENABLE = ${BT_VENDOR_NVRAM_ENABLE}

export MTK_BT_C4A="no"
export CC=$CC
export CXX=$CXX
export BT_GET_CROSS_COMPILE="yes"


if [ ! $VENDOR_BT_SET_ENVIRONMENT ]; then
    export VENDOR_BT_SET_ENVIRONMENT="yes"
    source ${Bluetooth_Tool_Dir}/script/vendor_set_environment.sh
fi

echo "Bluetooth_Tool_Dir = ${Bluetooth_Tool_Dir}"
echo "VENDOR_LIBRARY = $VENDOR_LIBRARY"
echo "MTK_BT_CHIP_ID = $MTK_BT_CHIP_ID"
echo "MTK_BT_PROJECT = $MTK_BT_PROJECT"

if [ "$ENABLE_SYS_LOG" = "yes" ]; then
    export BT_SYS_LOG_FLAG=-DMTK_BT_SYS_LOG
    echo "enable sys log:$ENABLE_SYS_LOG"
else
    echo "not disable sys log:$ENABLE_SYS_LOG"
fi
if [ "${MTK_BT_CHIP_ID:0:4}" = "mt76" ]; then
    export BT_PERFORMANCE_ANALYSIS_FLAG=-DMTK_BT_PERFORMANCE_ANALYSIS
fi

cd ${Bluetooth_Tool_Dir}/script

echo "start clean bluetooth vendor"
/bin/bash clean_bluetooth_vendor.sh
if [ "$MTK_BT_CHIP_ID" = "mt8167" -o "$MTK_BT_CHIP_ID" = "mt6630"  -o "$MTK_BT_CHIP_ID" = "mt6631" ]; then
    #copy platform library and header files for build vendor
    if [ -f $1/../src/support/libnvram_custom/CFG/${MTK_BT_PROJECT}/linux-4.4.22/common/cgen/cfgdefault/CFG_BT_Default.h ]; then
        cp $1/../src/support/libnvram_custom/CFG/${MTK_BT_PROJECT}/linux-4.4.22/common/cgen/cfgdefault/CFG_BT_Default.h ${Bluetooth_Vendor_Lib_Dir}/mtk/bluedroid/external/platform/
    elif [ -f $1/../src/support/libnvram_custom/CFG/${MTK_BT_PROJECT}/cgen/cfgdefault/CFG_BT_Default.h ]; then
        cp $1/../src/support/libnvram_custom/CFG/${MTK_BT_PROJECT}/cgen/cfgdefault/CFG_BT_Default.h ${Bluetooth_Vendor_Lib_Dir}/mtk/bluedroid/external/platform/
    fi
    cp $1/../src/support/libnvram_custom/CFG/${MTK_BT_PROJECT}/linux-4.4.22/common/cgen/cfgfileinc/CFG_BT_File.h ${Bluetooth_Vendor_Lib_Dir}/mtk/bluedroid/external/platform/
    cp $1/../src/support/libnvram_custom/CFG/${MTK_BT_PROJECT}/linux-4.4.22/common/cgen/inc/CFG_file_lid.h ${Bluetooth_Vendor_Lib_Dir}/mtk/bluedroid/external/platform/
    if [ -d $1/tmp/sysroots/${MTK_BT_PROJECT}/usr/include/alsa ]; then
        cp $1/tmp/sysroots/${MTK_BT_PROJECT}/${base_libdir}/libnvram_custom.so ${Bluetooth_Vendor_Lib_Dir}/mtk/bluedroid/external/platform/
        cp $1/tmp/sysroots/${MTK_BT_PROJECT}/${base_libdir}/libfile_op.so ${Bluetooth_Vendor_Lib_Dir}/mtk/bluedroid/external/platform/
        cp $1/tmp/sysroots/${MTK_BT_PROJECT}/${base_libdir}/libnvram.so ${Bluetooth_Vendor_Lib_Dir}/mtk/bluedroid/external/platform/
    else
        cp $4/${base_libdir}/libnvram_custom.so ${Bluetooth_Vendor_Lib_Dir}/mtk/bluedroid/external/platform/
        cp $4/${base_libdir}/libfile_op.so ${Bluetooth_Vendor_Lib_Dir}/mtk/bluedroid/external/platform/
        cp $4/${base_libdir}/libnvram.so ${Bluetooth_Vendor_Lib_Dir}/mtk/bluedroid/external/platform/
    fi
fi

echo "start build bluetooth vendor"
/bin/bash build_bluetooth_vendor.sh
if [ $? -ne 0 ]; then
    echo vendor_lib compile fail!!
    exit 1
fi
