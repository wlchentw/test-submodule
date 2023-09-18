#!/bin/sh
export WFD_SIGMA_AGENT_PORT=2412
export WFD_SIGMA_AGENT_IP="127.0.0.1"
export WLAN_IF_NAME=wlan0
export P2P_IF_NAME=p2p0
export WPA_CTRL_IFACE_PATH=/var/run/wpa_supplicant
export SIGMA_BIN=./
export CHECK_TOOL=/3rd_rw/Sigma_Auto_Check_Tool
export DHCPC_BINARY=/sbin/udhcpc
export DHCPD_LEASE_FILE=/tmp/dhcpd.leases
export DHCPD_CONFIG_FILE=/3rd/bin/wpa_supplicant/dhcpd/dhcpd.conf
export DHCPD_BINARY=/3rd/bin/wpa_supplicant/dhcpd/dhcpd
export DHCP_SERVER_IP_ADDRESS=192.168.5.1
export WPA_CLI_BIN_PATH=/3rd/bin/wpa_supplicant/common
export IWPRIV_BIN_PATH=/3rd/bin/wpa_supplicant/common
export BUSYBOX_BIN_PATH=/3rd_rw/Sigma_Auto_Check_Tool
export PATH=$SIGMA_BIN:$BUSYBOX_BIN_PATH:$WPA_CLI_BIN_PATH:$IWPRIV_BIN_PATH:$PATH

echo "*****************************************"
echo "**********Check Input Parameter**********"
echo "*****************************************"
echo "parameter1="$1
echo "parameter2="$2
echo "*****************************************"
echo "******************End********************"
echo "*****************************************"

echo "*****************************************"
echo "**********Check BusyBox Tool*************"
echo "*****************************************"
echo "busybox ifconfig"
busybox ifconfig
echo "*****************************************"
echo "******************End********************"
echo "*****************************************"

echo "*****************************************"
echo "**********Check WPA_CLI Tool*************"
echo "*****************************************"
echo "wpa_cli -i $WLAN_IF_NAME status"
wpa_cli -i $WLAN_IF_NAME status
echo "*****************************************"
echo "******************End********************"
echo "*****************************************"

echo "*****************************************"
echo "**********Check IWPRIV Tool**************"
echo "*****************************************"
echo "iwpriv $WLAN_IF_NAME connStatus"
iwpriv $WLAN_IF_NAME connStatus
echo "*****************************************"
echo "******************End********************"
echo "*****************************************"

echo "*****************************************"
echo "**********SIGMA BIN Path Test************"
echo "*****************************************"
echo "busybox ls -ll $SIGMA_BIN"
busybox ls -ll $SIGMA_BIN
echo "*****************************************"
echo "******************End********************"
echo "*****************************************"

echo "*****************************************"
echo "********WPA_CTRL_IFACE Path Test*********"
echo "*****************************************"
echo "busybox ls -ll $WPA_CTRL_IFACE_PATH"
busybox ls -ll $WPA_CTRL_IFACE_PATH
echo "*****************************************"
echo "******************End********************"
echo "*****************************************"

echo "*****************************************"
echo "*******/TMP Path Write/Read Test*********"
echo "*****************************************"
echo "busybox touch /tmp/tmp_path_test_file"
busybox touch /tmp/tmp_path_test_file
echo "busybox echo hello world > /tmp/tmp_path_test_file"
busybox echo "hello world" > /tmp/tmp_path_test_file
echo "*****************************************"
echo "******************End********************"
echo "*****************************************"

echo "*****************************************"
echo "**********Linux cat CLI Check************"
echo "*****************************************"
echo "busybox cat /tmp/tmp_path_test_file"
busybox cat /tmp/tmp_path_test_file
echo "*****************************************"
echo "******************End********************"
echo "*****************************************"

echo "*****************************************"
echo "***********Linux RM CLI Check************"
echo "*****************************************"
echo "busybox rm /tmp/tmp_path_test_file"
busybox rm /tmp/tmp_path_test_file
echo "busybox ls /tmp/tmp_path_test_file"
busybox ls /tmp/tmp_path_test_file
echo "*****************************************"
echo "******************End********************"
echo "*****************************************"

echo "*****************************************"
echo "*********Linux Route CLI Check***********"
echo "*****************************************"
echo "busybox route -n"
busybox route -n
echo "busybox route add default gw 127.0.0.1"
busybox route add default gw 127.0.0.1
echo "busybox route -n"
busybox route -n
echo "busybox route del default gw 127.0.0.1"
busybox route del default gw 127.0.0.1
echo "busybox route -n"
busybox route -n
echo "*****************************************"
echo "******************End********************"
echo "*****************************************"

echo "*****************************************"
echo "**********Linux cp CLI Check*************"
echo "*****************************************"
echo "busybox ls /tmp/check.log"
busybox ls /tmp/check.log
echo "busybox cp $CHECK_TOOL/check.log /tmp/check.log"
busybox cp $CHECK_TOOL/check.log /tmp/check.log
echo "busybox ls /tmp/check.log"
busybox ls /tmp/check.log
echo "busybox rm /tmp/check.log"
busybox rm /tmp/check.log
echo "busybox ls /tmp/check.log"
busybox ls /tmp/check.log
echo "*****************************************"
echo "******************End********************"
echo "*****************************************"

echo "*****************************************"
echo "**********Linux ping CLI Check***********"
echo "*****************************************"
echo "busybox ping -c 4 127.0.0.1"
busybox ping -c 4 127.0.0.1
echo "*****************************************"
echo "******************End********************"
echo "*****************************************"

echo "*****************************************"
echo "*********Linux sleep CLI Check***********"
echo "*****************************************"
echo "busybox sleep 1"
busybox sleep 1
echo "*****************************************"
echo "******************End********************"
echo "*****************************************"

echo "*****************************************"
echo "***********Linux ps CLI Check************"
echo "*****************************************"
echo "busybox ps"
busybox ps
echo "*****************************************"
echo "******************End********************"
echo "*****************************************"

if [ "$2" = "WFD" -o "$2" = "P2P" ];then
echo "*****************************************"
echo "********DHCP Server Start Check**********"
echo "*****************************************"
if [ "$1" = "andriod" ];then
echo "ndc tether stop"
ndc tether stop
echo "busybox ps | grep dhcpd"
busybox ps | grep dhcpd
echo "busybox ifconfig $P2P_IF_NAME $DHCP_SERVER_IP_ADDRESS"
busybox ifconfig $P2P_IF_NAME 192.168.5.1
echo "ndc tether start 192.168.5.100 192.168.5.200"
ndc tether start 192.168.5.100 192.168.5.200
echo "busybox ps | grep dhcpd"
busybox ps | grep dhcpd
else
echo "busybox killall dhcpd"
busybox killall dhcpd
echo "busybox ps | grep dhcpd"
busybox ps | grep dhcpd
echo "busybox ifconfig $P2P_IF_NAME $DHCP_SERVER_IP_ADDRESS"
busybox ifconfig $P2P_IF_NAME $DHCP_SERVER_IP_ADDRESS
echo "echo \"\"> $DHCPD_LEASE_FILE ;sync;sync;$DHCPD_BINARY -cf $DHCPD_CONFIG_FILE -lf $DHCPD_LEASE_FILE $P2P_IF_NAME &"
echo \"\"> $DHCPD_LEASE_FILE ;sync;sync;$DHCPD_BINARY -cf $DHCPD_CONFIG_FILE -lf $DHCPD_LEASE_FILE $P2P_IF_NAME &
echo "busybox ps | grep dhcpd"
busybox ps | grep dhcpd
fi
echo "*****************************************"
echo "******************End********************"
echo "*****************************************"

echo "*****************************************"
echo "********DHCP Server Stop Check***********"
echo "*****************************************"
sleep 5
if [ "$1" = "andriod" ];then
echo "ndc tether stop"
ndc tether stop
echo "busybox ps | grep dhcpd"
busybox ps | grep dhcpd
else
echo "busybox killall dhcpd"
busybox killall dhcpd
echo "busybox ps | grep dhcpd"
busybox ps | grep dhcpd
fi
echo "*****************************************"
echo "******************End********************"
echo "*****************************************"

echo "*****************************************"
echo "********DHCP Client Start Check**********"
echo "*****************************************"
echo "busybox killall udhcpc"
busybox killall udhcpc
echo "busybox ps | grep udhcpc"
busybox ps | grep udhcpc
echo "$DHCPC_BINARY --timeout=3 --retries=10 -b -i $WLAN_IF_NAME -s /usr/share/udhcpc/default.script -p /tmp/udhcpc-$WLAN_IF_NAME.pid &"
$DHCPC_BINARY --timeout=3 --retries=10 -b -i $WLAN_IF_NAME -s /usr/share/udhcpc/default.script -p /tmp/udhcpc-$WLAN_IF_NAME.pid &
echo "busybox ps | grep udhcpc"
busybox ps | grep udhcpc
echo "*****************************************"
echo "******************End********************"
echo "*****************************************"

echo "*****************************************"
echo "********DHCP Client Stop Check***********"
echo "*****************************************"
sleep 5
echo "busybox killall udhcpc"
busybox killall udhcpc
echo "busybox ps | grep udhcpc"
busybox ps | grep udhcpc
echo "*****************************************"
echo "******************End********************"
echo "*****************************************"
fi

if [ "$2" = "WFD" ];then
echo "*****************************************"
echo "*****WFD Sigma Socket Connect Test*******"
echo "*****************************************"
echo "ls -l /proc/self/fd/"
busybox ls -l /proc/self/fd/
echo "exec 8<>/dev/tcp/127.0.0.1/2472"
exec 8<>/dev/tcp/127.0.0.1/2472
echo "ls -l /proc/self/fd/"
busybox ls -l /proc/self/fd/
echo "*****************************************"
echo "******************End********************"
echo "*****************************************"
fi


