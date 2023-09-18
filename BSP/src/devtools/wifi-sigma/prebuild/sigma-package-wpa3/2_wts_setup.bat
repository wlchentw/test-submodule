adb wait-for-device
::adb shell "setprop sys.usb.config rndis,adb;"

@ping 127.0.0.1 -n 3 -w 1000 > null
adb wait-for-device


adb shell "ifconfig -a | grep -e usb0 -e rndis0 | busybox cut -f1 -d \" \"" > iface.txt
set /p ETH_IF=<iface.txt
echo RNDIS = %ETH_IF%
adb shell ifconfig %ETH_IF% 192.168.250.29 netmask 255.255.255.0
adb shell "iptables -F"
adb shell "iptables -P INPUT ACCEPT"
adb shell "iptables -P OUTPUT ACCEPT"
adb shell "iptables -P FORWARD ACCEPT"
pause



@echo off
echo Please manually execute the command in "/data/wts/run_sigma_sta.sh"
echo Please manually execute the command in "/data/wts/run_sigma_p2p.sh"
echo Please manually execute the command in "/data/wts/run_sigma_wmmps.sh"
echo Please manually execute the command in "/data/wts/run_sigma_wpa3.sh"
echo Please manually execute the command in "/data/wts/run_sigma_voe.sh"
del iface.txt
pause
pause