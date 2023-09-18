#!/bin/sh
export WFA_DUT_PORT=8000
export WFA_CA_PORT=9000
export WFA_ENV_AGENT_IPADDR=127.0.0.1
export WFA_ENV_AGENT_PORT=8000
export ETH0_IP=192.168.251.70
export WLAN0_DEFAULT_IP=0.0.0.0
#export WPA_CTRL_IFACE_PATH=/data/misc/wifi/sockets
#export WPA_CTRL_IFACE_PATH=/var/run/wpa_supplicant
export WPA_CTRL_IFACE_PATH=/tmp/wpa_supplicant
export SIGMA_BIN=sigma_p2p
export PATH=./$SIGMA_BIN:./scripts_busybox:$PATH
#export PATH=./$SIGMA_BIN:./scripts_busybox:$PATH

#LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/3rd/lib:/basic/lib:/system/lib
#export LD_LIBRARY_PATH

#clear
#echo "============================  Terminating Android framework    =============================="
#svc wifi disable
#stop;

busybox killall wpa_supplicant
busybox killall wfa_ca
busybox killall wfa_dut
busybox killall dhcpcd
busybox killall dhcpc
#workaround for P2P-5.1.2 DHCP server (20190305 Danny)
busybox killall dnsmasq
mkdir /var/lib/misc
#setenforce 0

echo "============================   Bring down wlan0    =================================="
#busybox ifconfig wlan0 $WLAN0_DEFAULT_IP down
echo ap_start 0 > /proc/net/wlan/driver   

sleep 1

#Rebuilding the /bin/sh for "system" call
#rm /bin/sh
#ln -s /sbin/sh /bin/sh
#sync;

echo "[Sigma] ============================   Setting eth0 IP to $ETH0_IP    ===================================="
ifconfig eth0 $ETH0_IP


echo "[Sigma] ============================   Setting STATION DUT Environment   ===================================="
chmod 777 ./sigma_p2p/*
chmod 777 ./scripts_busybox/*
#workaround for P2P-5.1.1 udhcpc IP request from GCUT to GO (20190304 Danny)
mkdir /etc/udhcpc
cp /etc/udhcpc.d/50default  /etc/udhcpc/default.script

echo "[Sigma] Starting wpa_supplicant and driver"
wpa_supplicant -d -O$WPA_CTRL_IFACE_PATH -Dnl80211 -iwlan0 -c /data/wts/wpa_supplicant.conf -N -O$WPA_CTRL_IFACE_PATH -Dnl80211 -ip2p0 -c /data/wts/p2p_supplicant.conf &
#/system/bin/wpa_supplicant -d -O/data/misc/wifi/sockets -Dnl80211 -iwlan0 -c /data/misc/wifi/wpa_supplicant.conf &
sleep 2
wpa_cli -i wlan0 -p$WPA_CTRL_IFACE_PATH disconnect
#wpa_cli -i wlan0 -p$WPA_CTRL_IFACE_PATH add_n


echo "[Sigma] Starting wfa_dut and wfa_ca..."
#PATH=./$SIGMA_BIN:./scripts_busybox:$PATH wfa_dut wlan0 $WFA_DUT_PORT &
wfa_dut wlan0 $WFA_DUT_PORT &
sleep 1
#PATH=./$SIGMA_BIN:./scripts_busybox:$PATH wfa_ca eth0 $WFA_CA_PORT &
wfa_ca eth0 $WFA_CA_PORT &

#iwpriv wlan0 set Debug=3
iwpriv wlan0 driver VER
#setup Tgn sigma mode for disable fast TX mode
iwpriv wlan0 driver "set_chip sigma 6"
echo "[Sigma] ======================== Ready for STATION Certification ==========================================="



