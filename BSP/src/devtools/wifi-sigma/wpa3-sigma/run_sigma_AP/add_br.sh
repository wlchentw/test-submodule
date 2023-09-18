#!/bin/sh
export BR0_IP=192.165.100.162
echo "[Enable SoftAP on MT7668]"
echo ap_start 1 > /proc/net/wlan/driver
echo "[Create br0 between eth0 and ap0]"
brctl addbr br0
brctl addif br0 eth0
brctl addif br0 ap0
echo "[Change br0's Forward Delay from Default to 0 sec]"
brctl setfd br0 0
ifconfig eth0 down
ifconfig ap0 down
ifconfig eth0 0.0.0.0 up
ifconfig ap0 0.0.0.0 up
echo "[Set eth0 to promisc mode]"
ifconfig eth0 promisc
echo "[Assogn br0's IP]"
ifconfig br0 $BR0_IP netmask 255.255.0.0 up
echo "[Enable traffic forward functionality]"
echo "1" > /proc/sys/net/ipv4/ip_forward
