#!/bin/sh
export WFA_DUT_PORT=8000
export WFA_CA_PORT=9000
export WFA_ENV_AGENT_IPADDR=127.0.0.1
export WFA_ENV_AGENT_PORT=8000
export ETH0_IP=192.168.250.82
export RA0_DEFAULT_IP=0.0.0.0
export WPA_CTRL_IFACE_PATH=/var/run/wpa_supplicant
export SIGMA_BIN=/tmp/bdpwts
export PATH=$SIGMA_BIN:$PATH

if [ ! -e $SIGMA_BIN ]; then
	mkdir -p $SIGMA_BIN
	cp -f /usr/local/bin/WTS_BDP/* $SIGMA_BIN
fi

echo "============================  Terminating Process: wfa_ca, wfd_dut, dhcpcd, dhcpc    =============================="

#killall wpa_supplicant
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

#echo "============================   Bring down ra0    =================================="
#ifconfig ra0 $RA0_DEFAULT_IP down
#sleep 1

#echo "============================   Setting ra0 IP to $RA0_DEFAULT_IP   =================================="
ifconfig ra0 $RA0_DEFAULT_IP

#Rebuilding the /bin/sh for "system" call
#rm /bin/sh
#ln -s /sbin/sh /bin/sh
#sync;

echo "[Sigma] ============================   Setting eth0 IP to $ETH0_IP    ===================================="
ifconfig eth0 $ETH0_IP


#echo "[Sigma] ============================   Setting STATION DUT Environment   ===================================="
#chmod 777 ./sigma_sta/*


#echo "[Sigma] Starting wpa_supplicant and driver"
#sleep 2
wpa_cli -i ra0 -p$WPA_CTRL_IFACE_PATH disconnect

echo "[Sigma] ------------------ Enable the Cert flow in driver...------------------"
iwpriv ra0 set SigmaEnable=1

echo "[Sigma] Starting wfa_dut and wfa_ca..."
wfa_dut_sta ra0 $WFA_DUT_PORT &
sleep 1
wfa_ca_sta eth0 $WFA_CA_PORT &

#iwpriv ra0 set Debug=3
iwpriv ra0 driverVer
echo "[Sigma] ======================== Ready for STATION Certification ==========================================="



