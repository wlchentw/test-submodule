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
export SIGMA_BIN=/sigma_p2p
export PATH=./$SIGMA_BIN:./scripts_busybox:$PATH

clear
echo "============================  Terminating Android framework    =============================="
svc wifi disable
stop;

echo "@@@@@@   Terminating framework    @@@@@"
busybox killall wfa_ca
busybox killall wfa_dut
setenforce 0
sleep 2

echo "@@@@@@   Setting eth0 IP to $ETH0_IP    @@@@@"
ifconfig eth0 $ETH0_IP
sleep 2

echo 1 > /sys/power/wake_lock

echo "@@@@@@   Starting P2P_SIGMA   @@@@@@@"
echo "[Sigma] ------------------Starting wpa_supplicant--------------------"
sleep 3
wpa_cli -i wlan0 -p$WPA_CTRL_IFACE_PATH disconnect
echo "[Sigma] wpa_supplicant started"

chmod -R 777 ./sigma_p2p
chmod -R 777 ./scripts_busybox

echo "[Sigma] ------------------ Starting wfa_dut...------------------"
./sigma_p2p/wfa_dut lo $WFA_DUT_PORT &
sleep 3

echo "[Sigma] ------------------Starting wfa_ca...------------------";
./sigma_p2p/wfa_ca eth0 $WFA_CA_PORT &
sleep 1
echo " ";
echo "[Sigma] ********************************************************"
echo "[Sigma] ------------- Ready to test SIGMA, let's roll-----------"
echo "[Sigma] ********************************************************"

#clear logcat buffer
#logcat -c

#logcat &

am start -n com.mediatek.wificert/.BoxActivity
sleep 2
#wpa_cli -iwlan0 -p/data/misc/wifi/sockets set block_scan 1 &
