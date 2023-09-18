
FOR /F "tokens=1 delims=" %%A in ('adb shell wpa_cli add_network') do SET idx=%%A

adb shell wpa_cli -g@android:wpa_wlan0 IFNAME=wlan0 disable_network %idx%
adb shell wpa_cli -g@android:wpa_wlan0 IFNAME=wlan0 set_network %idx% ssid '\"ssidhome-network\"'
adb shell wpa_cli -g@android:wpa_wlan0 IFNAME=wlan0 set_network %idx% key_mgmt 'WPA-PSK'
adb shell wpa_cli -g@android:wpa_wlan0 IFNAME=wlan0 set_network %idx% psk '\"9876543210\"'
adb shell wpa_cli -g@android:wpa_wlan0 IFNAME=wlan0 set_network %idx% priority 255
adb shell wpa_cli -g@android:wpa_wlan0 IFNAME=wlan0 set_network %idx% imsi '\"none\"'
adb shell wpa_cli -g@android:wpa_wlan0 IFNAME=wlan0 set_network %idx% sim_slot '\"-1\"'
adb shell wpa_cli -g@android:wpa_wlan0 IFNAME=wlan0 set_network %idx% pcsc '\"none\"'
adb shell wpa_cli -g@android:wpa_wlan0 IFNAME=wlan0 save_config

echo Complete adding profile for test plan 5_4_D 1st profile...
PAUSE