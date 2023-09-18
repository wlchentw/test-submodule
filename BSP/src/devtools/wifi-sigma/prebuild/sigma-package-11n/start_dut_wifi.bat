@echo on



:start_wifi
title WIFI-ON
rem if %2 is D, then wfa_dut will print more traces
mode con cols=55 lines=35

rem turn off dhcpcd$
adb shell stop dhcpcd

rem turn off bt
adb shell killall btservice
adb shell killall bluetoothtbd



rem turn off appmain
adb shell killall appmainprog
adb shell killall appmainprog

rem down ap0 for c4a/gva product
adb shell ifconfig ap0 down

rem kill cast_shell for c4a/gva product
adb shell ./chrome/cast_cli stop cast

rem open performance mode
adb shell "echo 0 > /proc/hps/enabled"
adb shell "echo 1 > /sys/devices/system/cpu/cpu0/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu1/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu2/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu3/online"
adb shell "echo "performance" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
adb shell "echo "1300000" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"
adb shell "echo "1300000" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq"

rem configure TpTestMode: n_ac_pmf(2) for 7668
adb shell iwpriv wlan0 driver "set_cfg TpTestMode 2"

rem maybe you need to insert cmds here to enable wifi and start wpa_supplicant program. Device usually auto enable wifi and supplicant after power on.

rem change wifi to disconnect any APs.
adb shell wpa_cli -iwlan0 -p/tmp/wpa_supplicant DISCONNECT



