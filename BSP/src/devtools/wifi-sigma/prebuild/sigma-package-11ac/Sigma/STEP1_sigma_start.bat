if not exist log (mkdir log)

:turn_on_wifi_in_sigma_mode
echo turning wifi on, wait 2 seconds
ping 127.0.0.1 -n 2 > nul
rem adb shell setprop ctl.start wpa_supplicant
echo wait 2s to check if wifi enabled
ping 127.0.0.1 -n 2 > nul
adb shell ps|findstr wpa_supplicant>null || goto wpa_start_fail
rem the -p parameters can been changed according to your platform
adb shell wpa_cli -iwlan0 -p/tmp/wpa_supplicant status

rem turn off dhcpcd$
adb shell stop dhcpcd

rem kill bt and appmainprog
adb shell killall btservice
adb shell killall bluetoothtbd
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

rem configure TpTestMode: n_ac_pmf(2)
adb shell iwpriv wlan0 driver "set_cfg TpTestMode 2"

rem here we create a new sub-directory in /tmp directory. So you should confirm your platform has the /tmp directory with read and write access rights
adb shell mkdir /tmp/asec

REM add routing rule
adb shell ip rule add pref 9999 from all fwmark 0x10063/0x1ffff table main
adb shell ip rule add pref 9999 from all fwmark 0x0/0xffff table main

if exist callback_for_start.bat (call callback_for_start.bat)
goto end

:wpa_start_fail
msg %username% E008: enable wlan in sigma mode fail!
goto end

:add_wlan0_if_fail
msg %username% E009: enable wlan in sigma mode fail!
goto end

:end
pause