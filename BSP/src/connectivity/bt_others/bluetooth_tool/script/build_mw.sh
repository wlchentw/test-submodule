#!/bin/bash

if [ ! $BT_SET_ENVIRONMENT ]; then
    export BT_SET_ENVIRONMENT="yes"
    source ./set_environment.sh
fi

echo "MTK_BT_C4A = ${MTK_BT_C4A}"
if [ "$SUPPORT_IPDC" = "yes" ]; then
    IPCD_LINK="-lipcd"
    ENABLE_IPCD_FLAG="-DENABLE_IPCD"
else
    IPCD_LINK=""
    ENABLE_IPCD_FLAG=""
fi

if [ "$MTK_BT_C4A" = "yes" ]; then
   export BT_C4A_BLE_SETUP_flag=-DMTK_LINUX_C4A_BLE_SETUP
   echo " set DMTK_LINUX_C4A_BLE_SETUP"
else
   echo "no set DMTK_LINUX_C4A_BLE_SETUP"
fi

Stack_Include_Path=${Bluetooth_Stack_Dir}/mediatek/include
Stack_Root_Path=${Bluetooth_Stack_Dir}

cd ${Bluetooth_Mw_Dir}/sdk

rm -rf out

gn gen out/Default/ --args="stack_include_path=\"${Stack_Include_Path}\" stack_root_path=\"${Stack_Root_Path}\" bluedroid_libs_path=\"-L${Bluedroid_Libs_Path}\" storage_path=\"${Storage_Path}\" platform_libs_path=\"${Platform_Libs_Path}\" bt_sys_log_flag=\"${BT_SYS_LOG_FLAG}\" bt_tmp_path=\"${BT_Tmp_Path}\" enable_ipcd=\"${ENABLE_IPCD_FLAG}\" ipcd_link=\"${IPCD_LINK}\" bt_misc_path=\"${BT_Misc_Path}\" bt_etc_path=\"${BT_Etc_Path}\" conf_path=\"${Conf_Path}\" rpc_dbg_flag=\"${Rpc_Dbg_Flag}\" enable_ble_mesh=\"${ENABLE_BLE_MESH}\" bt_c4a_ble_setup_flag=\"${BT_C4A_BLE_SETUP_flag}\" cc=\"${CC}\" cxx=\"${CXX}\""
ninja -C out/Default all

if [ ! -d ${Bluetooth_Prebuilts_Dir}/lib ]; then
    mkdir -p ${Bluetooth_Prebuilts_Dir}/lib
fi

cd ${Script_Dir}

if [ -f ${Bluetooth_Mw_Dir}/sdk/out/Default/libbt-mw.so ]; then
    cp ${Bluetooth_Mw_Dir}/sdk/out/Default/libbt-mw.so ${Bluetooth_Prebuilts_Dir}/lib/
else
    exit 1
fi