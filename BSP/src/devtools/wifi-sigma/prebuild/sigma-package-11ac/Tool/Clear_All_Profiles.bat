@echo off

adb shell svc wifi enable
adb shell ping -c 10 -q 127.0.0.1
adb shell wpa_cli -g@android:wpa_wlan0 IFNAME=wlan0 remove_network all
adb shell wpa_cli -g@android:wpa_wlan0 IFNAME=wlan0 save_config

echo Complete removing all profiles...
pause