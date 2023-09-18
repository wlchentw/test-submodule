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
export SIGMA_BIN=/sigma_sta
export PATH=./$SIGMA_BIN:./scripts_busybox:$PATH

clear
echo "============================  Terminating Android framework    =============================="
svc wifi disable
stop;

busybox killall wpa_supplicant
busybox killall wfa_ca
busybox killall wfa_dut
busybox killall dhcpcd
busybox killall dhcpc
setenforce 0

echo "============================   Bring down wlan0    =================================="
busybox ifconfig wlan0 $WLAN0_DEFAULT_IP down
sleep 1

echo "[Sigma] ============================   Setting eth0 IP to $ETH0_IP    ===================================="
ifconfig eth0 $ETH0_IP


echo "[Sigma] ============================   Setting STATION DUT Environment   ===================================="
chmod 777 ./sigma_sta/*


echo "[Sigma] Starting wpa_supplicant and driver"
/system/bin/wpa_supplicant -d -O/data/misc/wifi/sockets -Dnl80211 -iwlan0 -c /data/misc/wifi/wpa_supplicant.conf &
sleep 2
wpa_cli -i wlan0 -p$WPA_CTRL_IFACE_PATH disconnect


echo "[Sigma] Starting wfa_dut and wfa_ca..."
./sigma_sta/wfa_dut wlan0 $WFA_DUT_PORT &
sleep 1
./sigma_sta/wfa_ca eth0 $WFA_CA_PORT &

#iwpriv wlan0 set Debug=3
iwpriv wlan0 driver VER
#setup Tgn sigma mode for disable fast TX mode
iwpriv wlan0 driver "set_chip sigma 6"
echo "[Sigma] ======================== Ready for STATION Certification ==========================================="



