#!/bin/bash

if [ ! $BT_SET_ENVIRONMENT ]; then
    export BT_SET_ENVIRONMENT="yes"
    source ./set_environment.sh
fi

echo "start build stack"
/bin/bash build_stack.sh
if [ ! -f ${Bluetooth_Prebuilts_Dir}/lib/libbluetooth.default.so ]; then
    echo build libbluetooth.default.so failed, EXIT!
    exit 1
fi
if [ ! -f ${Bluetooth_Prebuilts_Dir}/lib/libaudio.a2dp.default.so ]; then
    echo build libaudio.a2dp.default.so failed, EXIT!
    exit 1
fi

if [ ! -f ${Bluetooth_Prebuilts_Dir}/conf/bt_stack.conf ]; then
    echo copy bt_stack.conf failed, EXIT!
    exit 1
fi
if [ ! -f ${Bluetooth_Prebuilts_Dir}/conf/bt_did.conf ]; then
    echo copy bt_did.conf failed, EXIT!
    exit 1
fi

echo "start build btut"
/bin/bash build_btut.sh
if [ ! -f ${Bluetooth_Prebuilts_Dir}/bin/btut ]; then
    echo build btut failed, EXIT!
    exit 1
fi

echo "start build bt_common"
/bin/bash build_bt_common.sh
if [ ! -f ${Bluetooth_Prebuilts_Dir}/lib/libbt-common.so ]; then
    echo build libbt-common.so failed, EXIT!
    exit 1
fi

echo "start build mw"
/bin/bash build_mw.sh
if [ ! -f ${Bluetooth_Prebuilts_Dir}/lib/libbt-mw.so ]; then
    echo build libbt-mw.so failed, EXIT!
    exit 1
fi

#external_lib have ALSA library so should build playback module
if [ $ENABLE_A2DP_SINK -eq 1 -a -f ${External_Libs_Path}/libasound.so ]; then
    echo "start build playback"
    /bin/bash build_playback.sh
    if [ ! -f ${Bluetooth_Prebuilts_Dir}/lib/libbt-alsa-playback.so ]; then
        echo build libbt-alsa-playback.so failed, EXIT!
        exit 1
    fi
else
    echo "no need build playback"
fi

#external_lib have ALSA library so should build uploader module
if [ $ENABLE_A2DP_SRC -eq 1 -a $ENABLE_A2DP_ADEV -eq 0 -a -f ${External_Libs_Path}/libasound.so ]; then
    echo "start build uploader"
    /bin/bash build_uploader.sh
	if [ $ENABLE_FILE_UPL -eq 0 ]; then
		if [ ! -f ${Bluetooth_Prebuilts_Dir}/lib/libbt-alsa-uploader.so ]; then
			echo build libbt-alsa-uploader.so failed, EXIT!
			exit 1
		fi
	else
		if [ ! -f ${Bluetooth_Prebuilts_Dir}/lib/libbt-file-uploader.so ]; then
			echo build libbt-file-uploader.so failed, EXIT!
			exit 1
		fi
	fi
else
    echo "no need build uploader"
fi

#######################################--Begin RPC--########################################
echo "start build rpc"
/bin/bash build_rpc.sh
if [ $? -ne 0 ]; then
    echo rpc compile fail!!
    exit 1
fi
#######################################--End RPC--########################################

echo "start build non rpc"
/bin/bash build_non_rpc_test.sh
if [ $? -ne 0 ]; then
    echo non-rpc-test compile fail!!
    exit 1
fi

echo "start build dbg"
/bin/bash build_rpc_dbg.sh
if [ $? -ne 0 ]; then
    echo rpc-dbg compile fail!!
    exit 1
fi

echo "start build demo"
/bin/bash build_mw_rpc_test.sh
if [ $? -ne 0 ]; then
    echo mw-rpc-test compile fail!!
    exit 1
fi

cd ${Script_Dir}
