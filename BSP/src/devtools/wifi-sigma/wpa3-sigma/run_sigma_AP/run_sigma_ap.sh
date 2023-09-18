#!/bin/sh

export WFA_DUT_PORT=8000
export WFA_CA_PORT=9000
export WFA_ENV_AGENT_IPADDR=127.0.0.1
export WFA_ENV_AGENT_PORT=8000
export ETH0_IP=192.168.250.70
export WLAN0_DEFAULT_IP=0.0.0.0
#export WPA_CTRL_IFACE_PATH=/data/misc/wifi/sockets
export SIGMA_BIN=/sigma_ap
export PATH=./$SIGMA_BIN:./scripts_busybox:$PATH

echo "@@@@@@   Terminating framework    @@@@@"
busybox killall wfa_ca
busybox killall wfa_dut
sleep 2

#Rebuilding the /bin/sh for "system" call
sync;
sleep 1;


echo "@@@@@@   Setting eth1 IP to $ETH0_IP    @@@@@"
ifconfig eth1 $ETH0_IP
sleep 2

echo "@@@@@@   Starting AP_SIGMA   @@@@@@@"

echo "[Sigma] ------------------ Starting wfa_dut...------------------"
PATH=./$SIGMA_BIN:./scripts_busybox:$PATH wfa_dut wlan1 $WFA_DUT_PORT &
sleep 3

echo "[Sigma] ------------------Starting wfa_ca...------------------";
PATH=./$SIGMA_BIN:./scripts_busybox:$PATH wfa_ca eth1 $WFA_CA_PORT &
sleep 1

echo " ";
echo "[Sigma] ********************************************************"
echo "[Sigma] ------------- Ready to test SIGMA, let's roll-----------"
echo "[Sigma] ********************************************************"
