#---------------------------------------------------
# For MTK BDP platform
#---------------------------------------------------
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

#busybox cp /system/bin/sh /bin/sh
#chmod 0666 /dev/mmcblk0p26

killall dhcpd
killall dhclient
killall wfa_ca
killall wfa_dut
killall wfa_ca_sta
killall wfa_dut_sta
killall wfa_ca_p2p
killall wfa_dut_p2p
killall wfa_ca_wfd
killall wfa_dut_wfd

ifconfig eth0 $ETH0_IP
ifconfig ra0 0.0.0.0
ifconfig p2p0 0.0.0.0


#wpa_cli -i ra0 -p$WPA_CTRL_IFACE_PATH set wfd_devType 1
#wpa_cli -i ra0 -p$WPA_CTRL_IFACE_PATH set wfd_sessionAvail 1
#wpa_cli -i ra0 -p$WPA_CTRL_IFACE_PATH set wfd_maxThroughput 300
#wpa_cli -i ra0 -p$WPA_CTRL_IFACE_PATH set wfd_rtspPort 554
#wpa_cli -i p2p0 -p$WPA_CTRL_IFACE_PATH set device_name MTK-BDP-Sink


#am start -n com.mediatek.wificert/.BoxActivity
#sleep 2

echo "[Sigma] ------------------ Enable the Cert flow in driver...------------------"
iwpriv ra0 set SigmaEnable=1

#chmod 777 wfa_dut
echo "[Sigma] ------------------ Starting wfa_dut...------------------"
wfa_dut_wfd lo $WFA_DUT_PORT &
sleep 3

#chmod 777 wfa_ca
echo "[Sigma] ------------------Starting wfa_ca...------------------";
wfa_ca_wfd eth0 $WFA_CA_PORT &
sleep 1

echo " ";
echo "[Sigma] ********************************************************"
echo "[Sigma] ------------- Ready to test SIGMA, let's roll-----------"
echo "[Sigma] ********************************************************"
