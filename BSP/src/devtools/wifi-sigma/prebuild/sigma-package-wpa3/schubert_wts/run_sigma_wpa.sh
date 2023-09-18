#!/bin/sh
#export WLAN0_DEFAULT_IP=0.0.0.0
#export WPA_CTRL_IFACE_PATH=/tmp/wpa_supplicant
# copy to adb shell 0-0
cd /data/wts/
export SIGMA_BIN=sigma_wpa
export PATH=./$SIGMA_BIN:./scripts_busybox:$PATH

echo "[Sigma] ============================   Setting eth0 IP to $ETH0_IP    ===================================="
# copy to adb shell 0
export ETH0_IP=192.168.250.29
ifconfig rndis0 $ETH0_IP

echo "[Sigma] ============================   Setting STATION DUT Environment   ===================================="
chmod 777 ./sigma_wpa/*
chmod 777 ./scripts_busybox/*

# copy to adb shell 1
busybox killall wpa_supplicant
busybox killall wfa_ca
busybox killall wfa_dut
busybox killall dhcpcd
busybox killall dhcpc
busybox killall appmainprog
busybox killall btservice

echo "[Sigma] Starting wpa_supplicant and driver"
# copy to adb shell 2
wpa_supplicant -iwlan0 -Dnl80211 -c/data/wts/wpa_supplicant.conf &
wpa_cli -i wlan0 -p/tmp/wpa_supplicant disconnect

echo "[Sigma] Starting wfa_dut and wfa_ca..."
# copy to adb shell 3
cd /data/wts/
export WFA_DUT_PORT=8000
export WFA_CA_PORT=9000
export WFA_ENV_AGENT_IPADDR=127.0.0.1
export WFA_ENV_AGENT_PORT=8000
./sigma_wpa/wfa_dut wlan0 $WFA_DUT_PORT &
# copy to adb shell 5
./sigma_wpa/wfa_ca rndis0 $WFA_CA_PORT &

# ps and check wpa_supplicant & wfa_dut & wfa_ca are runing

echo "[Sigma] ======================== Ready for STATION Certification ==========================================="