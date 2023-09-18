#!/bin/bash

if [ ! $BT_SET_ENVIRONMENT ]; then
    export BT_SET_ENVIRONMENT="yes"
    source ./set_environment.sh
fi

Mw_Include_Path=${Bluetooth_Mw_Dir}/inc
Mw_Config_Path=${Bluetooth_Mw_Dir}/inc/config
Libbt_Mw_Path=${Bluetooth_Prebuilts_Dir}/lib
Libz_Path=${External_Libs_Path}

cd ${Bluetooth_Mw_Dir}/rpc

rm -rf out


if [ $ENABLE_A2DP_SRC -eq 1 -a $ENABLE_A2DP_ADEV -eq 0 -a -f ${External_Libs_Path}/libasound.so ]; then
	if [ $ENABLE_FILE_UPL -eq 0 ]; then
		UPLOADER_LINK="-lbt-alsa-uploader"
		Mw_ALSA_Uploader_Inc_Path=${Bluetooth_Mw_Dir}/uploader/ALSA
		echo link ALSA UPLOADER
	else
		UPLOADER_LINK="-lbt-file-uploader"
		Mw_ALSA_Uploader_Inc_Path=${Bluetooth_Mw_Dir}/uploader/FILE
		echo link FILE UPLOADER
	fi
else
    UPLOADER_LINK=""
    Mw_ALSA_Uploader_Inc_Path=$Mw_Include_Path
fi

echo "MTK_BT_C4A = ${MTK_BT_C4A}"
if [ "$MTK_BT_C4A" = "yes" ]; then
   export BT_C4A_BLE_SETUP_flag=-DMTK_LINUX_C4A_BLE_SETUP
   echo " set DMTK_LINUX_C4A_BLE_SETUP"
else
   echo "no set DMTK_LINUX_C4A_BLE_SETUP"
   export BT_C4A_BLE_SETUP_flag=""
fi

gn gen out/Default/ --args="mw_include_path=\"${Mw_Include_Path}\" mw_config_path=\"${Mw_Config_Path}\" mw_alsa_uploader_inc_path=\"${Mw_ALSA_Uploader_Inc_Path}\" libbt_mw_path=\"-L${Libbt_Mw_Path}\" libbt_uploader_link = \"${UPLOADER_LINK}\" libz_path=\"-L${Libz_Path}\" bt_sys_log_flag=\"${BT_SYS_LOG_FLAG}\" bt_tmp_path=\"${BT_Tmp_Path}\" bt_misc_path=\"${BT_Misc_Path}\" bt_etc_path=\"${BT_Etc_Path}\" rpc_dbg_flag=\"${Rpc_Dbg_Flag}\" bt_c4a_ble_setup_flag=\"${BT_C4A_BLE_SETUP_flag}\" cc=\"${CC}\" cxx=\"${CXX}\""
ninja -C out/Default all


cd ${Script_Dir}

if [ ! -d ${Bluetooth_Prebuilts_Dir}/lib ]; then
    mkdir -p ${Bluetooth_Prebuilts_Dir}/lib
fi
if [ ! -d ${Bluetooth_Prebuilts_Dir}/bin ]; then
    mkdir -p ${Bluetooth_Prebuilts_Dir}/bin
fi
if [ -f ${Bluetooth_Mw_Dir}/rpc/out/Default/libipcrpc.so ]; then
    cp ${Bluetooth_Mw_Dir}/rpc/out/Default/libipcrpc.so ${Bluetooth_Prebuilts_Dir}/lib/
else
    exit 1
fi
if [ -f ${Bluetooth_Mw_Dir}/rpc/out/Default/libmtk_bt_ipcrpc_struct.so ]; then
    cp ${Bluetooth_Mw_Dir}/rpc/out/Default/libmtk_bt_ipcrpc_struct.so ${Bluetooth_Prebuilts_Dir}/lib/
else
    exit 1
fi
if [ -f ${Bluetooth_Mw_Dir}/rpc/out/Default/libmtk_bt_service_client.so ]; then
    cp ${Bluetooth_Mw_Dir}/rpc/out/Default/libmtk_bt_service_client.so ${Bluetooth_Prebuilts_Dir}/lib/
else
    exit 1
fi
if [ -f ${Bluetooth_Mw_Dir}/rpc/out/Default/libmtk_bt_service_server.so ]; then
    cp ${Bluetooth_Mw_Dir}/rpc/out/Default/libmtk_bt_service_server.so ${Bluetooth_Prebuilts_Dir}/lib/
else
    exit 1
fi
if [ -f ${Bluetooth_Mw_Dir}/rpc/out/Default/btservice ]; then
    cp ${Bluetooth_Mw_Dir}/rpc/out/Default/btservice ${Bluetooth_Prebuilts_Dir}/bin/
else
    exit 1
fi
