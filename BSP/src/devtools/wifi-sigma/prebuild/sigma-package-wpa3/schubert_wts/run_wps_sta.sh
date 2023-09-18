#!/bin/sh
export ETH0_IP=192.168.251.70
export WLAN0_DEFAULT_IP=0.0.0.0
#export WPA_CTRL_IFACE_PATH=/data/misc/wifi/sockets
export WPA_CTRL_IFACE_PATH=/tmp/wpa_supplicant


#LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/3rd/lib:/basic/lib
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
#setenforce 0

echo "============================   Bring down wlan0    =================================="
#busybox ifconfig wlan0 $WLAN0_DEFAULT_IP down
sleep 1

#Rebuilding the /bin/sh for "system" call
#rm /bin/sh
#ln -s /sbin/sh /bin/sh
#sync;

echo "[Sigma] ============================   Setting eth0 IP to $ETH0_IP    ===================================="
ifconfig eth0 $ETH0_IP



echo "[Sigma] Starting wpa_supplicant and driver"
#/vendor/bin/hw/wpa_supplicant -d -O$WPA_CTRL_IFACE_PATH -Dnl80211 -iwlan0 -c /3rd_rw/wpa_supplicant.conf &
#/system/bin/wpa_supplicant -d -O/data/misc/wifi/sockets -Dnl80211 -iwlan0 -c /data/misc/wifi/wpa_supplicant.conf &
wpa_supplicant -iwlan0 -Dnl80211 -c/data/wts/wpa_supplicant.conf &
sleep 2
wpa_cli -i wlan0 -p$WPA_CTRL_IFACE_PATH disconnect
#wpa_cli -i wlan0 -p$WPA_CTRL_IFACE_PATH add_n


#iwpriv wlan0 set Debug=3
iwpriv wlan0 driver VER
#setup Tgn sigma mode for disable fast TX mode
iwpriv wlan0 driver "set_chip sigma 6"
echo "[Sigma] ======================== Ready for STATION Certification ==========================================="



