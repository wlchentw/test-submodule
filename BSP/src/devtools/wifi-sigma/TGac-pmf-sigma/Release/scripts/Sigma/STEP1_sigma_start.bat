if not exist log (mkdir log)

::Enable Wi-Fi
adb shell "svc wifi enable"

::Wait 5 second
@ECHO Wait 5s...
@ping 127.0.0.1 -n 5 > NUL

::Disable Wi-Fi
adb shell "svc wifi disable"

::Wait 5 second
@ECHO Wait 5s...
@ping 127.0.0.1 -n 5 > NUL

:turn_on_wifi_in_sigma_mode
echo turning wifi on, wait 2 seconds
adb shell "echo 1 > /dev/wmtWifi"
ping 127.0.0.1 -n 2 > nul
adb shell "echo S>/dev/wmtWifi"
ping 127.0.0.1 -n 2 > nul
adb shell setprop ctl.start wpa_supplicant
echo wait 2s to check if wifi enabled
ping 127.0.0.1 -n 2 > nul
adb shell ps -A|findstr wpa_supplicant>nul || goto wpa_start_fail

echo "add wlan0 interface.."
adb shell wpa_cli -g@android:wpa_wlan0 interface_add wlan0
echo wait 2s to check if wlan0 added
ping 172.0.0.1 -n 2 > nul
adb shell ifconfig|findstr wlan0>nul || goto add_wlan0_if_fail

echo "Set permissive mode"
adb shell setenforce permissive
adb shell getenforce

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