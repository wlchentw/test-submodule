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

#clear
#echo "============================  Terminating Android framework    =============================="
#svc wifi disable
#stop;

#busybox killall wpa_supplicant
killall wfa_ca
killall wfa_dut
killall wfa_ca_sta
killall wfa_dut_sta
killall wfa_ca_p2p
killall wfa_dut_p2p
killall wfa_ca_wfd
killall wfa_dut_wfd
killall dhcpd
killall dhcpc

#echo "============================   Bring down wlan0    =================================="
#busybox ifconfig wlan0 $WLAN0_DEFAULT_IP down
#sleep 1

#Rebuilding the /bin/sh for "system" call
#rm /bin/sh
#ln -s /sbin/sh /bin/sh
#sync;

echo "[Sigma] ============================   Setting eth0 IP to $ETH0_IP    ===================================="
ifconfig eth0 $ETH0_IP


#echo "[Sigma] ============================   Setting TDLS Sigma Environment   ===================================="
#chmod 777 ./sigma_sta/*


#echo "[Sigma] Starting wpa_supplicant and driver"
#/system/bin/wpa_supplicant -d -O/data/misc/wifi/sockets -Dnl80211 -iwlan0 -c /data/misc/wifi/wpa_supplicant.conf &
#sleep 2
wpa_cli -i ra0 -p$WPA_CTRL_IFACE_PATH disconnect
iwpriv ra0 set enable_dynamic_vga=0

echo "[Sigma] ------------------ Enable the Cert flow in driver...------------------"
iwpriv ra0 set SigmaEnable=1

echo "[Sigma] Starting wfa_dut and wfa_ca..."
wfa_dut_sta ra0 $WFA_DUT_PORT &
sleep 1
wfa_ca_sta eth0 $WFA_CA_PORT &

#iwpriv ra0 set Debug=3
iwpriv ra0 driverVer
sleep 1

# TDLS require U-APSD enabled
iwpriv ra0 set APSDCapable=1

echo "[Sigma] ========================   Sigma is ready for TDLS Certification  ==========================================="



