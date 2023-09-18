@echo "Insmod ko and supplicant"
adb root
adb remount
adb shell ifconfig wlan0 up
adb shell "wpa_supplicant -Dnl80211 -iwlan0 -c/etc/wpa_supplicant.conf"