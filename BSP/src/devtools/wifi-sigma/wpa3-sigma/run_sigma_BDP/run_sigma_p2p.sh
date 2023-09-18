#!/bin/sh

export WFA_DUT_PORT=8000
export WFA_CA_PORT=9000
export WFA_ENV_AGENT_IPADDR=127.0.0.1
export WFA_ENV_AGENT_PORT=8000
export ETH0_IP=192.168.250.82
export WLAN0_DEFAULT_IP=0.0.0.0
export WPA_CTRL_IFACE_PATH=/var/run/wpa_supplicant
export SIGMA_BIN=/tmp/bdpwts
export PATH=$SIGMA_BIN:$PATH

if [ ! -e $SIGMA_BIN ]; then
	mkdir -p $SIGMA_BIN
	cp -f /usr/local/bin/WTS_BDP/* $SIGMA_BIN
fi

echo "@@@@@@   Terminating framework    @@@@@"
killall wfa_ca
killall wfa_dut
killall wfa_ca_sta
killall wfa_dut_sta
killall wfa_ca_p2p
killall wfa_dut_p2p
killall wfa_ca_wfd
killall wfa_dut_wfd
sleep 2

#Rebuilding the /bin/sh for "system" call
#rm /bin/sh
#ln -s /sbin/sh /bin/sh
#sync;
#sleep 1;


echo "@@@@@@   Setting eth0 IP to $ETH0_IP    @@@@@"
ifconfig eth0 $ETH0_IP
sleep 2

#echo 1 > /sys/power/wake_lock

echo "@@@@@@   Starting P2P_SIGMA   @@@@@@@"
#echo "[Sigma] ------------------Starting wpa_supplicant--------------------"
#/usr/bin/wpa_supplicant -d -O/var/run/wpa_supplicant -Dnl80211 -ip2p0 -c /tmp/P2P_DEV_CONF &
#sleep 3
wpa_cli -i ra0 -p$WPA_CTRL_IFACE_PATH disconnect
#echo "[Sigma] wpa_supplicant started"

echo "[Sigma] ------------------ Enable the Cert flow in driver...------------------"
iwpriv ra0 set SigmaEnable=1

echo "[Sigma] ------------------ Starting wfa_dut...------------------"
wfa_dut_p2p lo $WFA_DUT_PORT &
sleep 3

echo "[Sigma] ------------------Starting wfa_ca...------------------";
wfa_ca_p2p eth0 $WFA_CA_PORT &
sleep 1
#iwpriv ra0 set Debug=3
#sleep 1;
echo " ";
echo "[Sigma] ********************************************************"
echo "[Sigma] ------------- Ready to test SIGMA, let's roll-----------"
echo "[Sigma] ********************************************************"

#clear logcat buffer
#logcat -c

#logcat &

#am start -n com.mediatek.wificert/.BoxActivity
sleep 2
#wpa_cli -ira0 -p/data/misc/wifi/sockets set block_scan 1 &
