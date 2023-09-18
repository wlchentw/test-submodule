#!/system/bin/sh
clear
echo "===== SELinux ====="
setenforce 0

echo "===== Export global parameters ====="
export WFA_DUT_PORT=8000
export WFA_CA_PORT=9999
export WFA_ENV_AGENT_IPADDR=127.0.0.1
export WFA_ENV_AGENT_PORT=8000
export ETH0_IP=192.168.250.70
export WLAN0_DEFAULT_IP=0.0.0.0
export WPA_CTRL_IFACE_PATH=/data/misc/wifi/sockets
export SIGMA_BIN=/sigma_sta
export PATH=./$SIGMA_BIN:$PATH

echo "===== Terminating Android framework ====="
svc wifi disable
stop
sleep 3

busybox killall wpa_supplicant
busybox killall wfa_ca
busybox killall wfa_dut
busybox killall dhcpcd
busybox killall dhcpc

echo "===== Remove wlan.ko ====="
ifconfig wlan0 down
rmmod mt7662u_sta
sleep 3

echo "===== Insert wlan.ko ====="
insmod /basic/modules/wlan.ko
sleep 3

echo "[Sigma] ===== Setting eth0 IP to $ETH0_IP ====="
ifconfig eth0 $ETH0_IP

echo "[Sigma] Starting wpa_supplicant and driver"
/system/bin/wpa_supplicant -d -O/data/misc/wifi/sockets -Dnl80211 -iwlan0 -c /data/misc/wifi/wpa_supplicant.conf &
sleep 3
wpa_cli -i wlan0 -p$WPA_CTRL_IFACE_PATH disconnect

echo "[Sigma] Starting wfa_dut and wfa_ca..."
./sigma_sta/wfa_dut wlan0 $WFA_DUT_PORT &
sleep 1
./sigma_sta/wfa_ca eth0 $WFA_CA_PORT &

#iwpriv wlan0 set Debug=3
iwpriv wlan0 driverVer
sleep 1

iwpriv wlan0 set SigmaEnable=1
iwpriv wlan0 set DynamicVgaEnable=0

echo "[Sigma] ===== Sigma is ready for STA Certification ====="
