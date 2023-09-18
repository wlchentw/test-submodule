#---------------------------------------------------
# For MTK Android DTV platform
#---------------------------------------------------
#!/sbin/sh

export WFA_DUT_PORT=8000
export WFA_CA_PORT=9000
export WFA_ENV_AGENT_IPADDR=127.0.0.1
export WFA_ENV_AGENT_PORT=8000
export ETH0_IP=192.168.250.70
export WLAN0_DEFAULT_IP=0.0.0.0
export WPA_CTRL_IFACE_PATH=/data/misc/wifi/sockets
export SIGMA_BIN=/sigma_wfd
export PATH=./$SIGMA_BIN:./scripts_busybox:$PATH

clear
echo "============================  Terminating Android framework    =============================="
svc wifi disable
stop;

busybox cp /system/bin/sh /bin/sh
chmod 0666 /dev/mmcblk0p26

busybox killall dhcpd
busybox killall dhclient
busybox killall wfa_ca
busybox killall wfa_dut
busybox killall wfd_cert

busybox ifconfig eth0 $ETH0_IP up
busybox ifconfig wlan0 0.0.0.0 up
busybox ifconfig p2p0 0.0.0.0 up
setenforce 0

wpa_cli -i wlan0 -p$WPA_CTRL_IFACE_PATH set wfd_devType 1
wpa_cli -i wlan0 -p$WPA_CTRL_IFACE_PATH set wfd_sessionAvail 1
wpa_cli -i wlan0 -p$WPA_CTRL_IFACE_PATH set wfd_maxThroughput 300
wpa_cli -i wlan0 -p$WPA_CTRL_IFACE_PATH set wfd_rtspPort 554
wpa_cli -i p2p0 -p$WPA_CTRL_IFACE_PATH set device_name MTK-DTV-Sink

wfd_cert &
sleep 2
echo "strat wfd_cert!!"

am start -n com.mediatek.wificert/.BoxActivity
sleep 2

chmod -R 777 ./sigma_wfd
chmod -R 777 ./scripts_busybox

echo "[Sigma] ------------------ Starting wfa_dut...------------------"
./sigma_wfd/wfa_dut lo $WFA_DUT_PORT &
sleep 3

echo "[Sigma] ------------------Starting wfa_ca...------------------";
./sigma_wfd/wfa_ca eth0 $WFA_CA_PORT &
sleep 1

echo " ";
echo "[Sigma] ********************************************************"
echo "[Sigma] ------------- Ready to test SIGMA, let's roll-----------"
echo "[Sigma] ********************************************************"

