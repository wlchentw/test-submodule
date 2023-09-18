@echo off
if "%1" == "start_dut" goto start_dut
if not exist scripts (msg %username% no scripts folder found! && goto :eof)
adb get-state|findstr "unknown">nul && (msg %username% no dut connected! && goto :eof)
adb root | findstr /c:"production builds">nul && (msg %username% sigma test can only run on eng or userdebug load && goto :eof)

for /r %%i in (scripts\*) do (adb shell ls -l /system/bin/%%~nxi | findstr r-x>nul || msg %username% %%~nxi is not exist or can't be executed! && goto :eof)
adb shell ps|findstr wpa_supplicant>nul || goto turn_on_wifi_in_sigma_mode
adb shell ls /data/misc/wifi/sockets/wpa_ctrl* | findstr /C:"No such file">nul && goto start_dut_and_ca
echo wlan is in normal mode, turn off first
adb shell svc wifi disable > nul
ping 127.0.0.1 -n 4 > nul

:turn_on_wifi_in_sigma_mode
rem echo turning wifi on, wait 2 seconds
rem adb shell "echo 1>/dev/wmtWifi6630"
rem ping 127.0.0.1 -n 2 > nul
rem adb shell /system/bin/wpa_supplicant -Dnl80211 -iwlan0 -c/data/misc/wifi/wpa_supplicant.conf &
rem echo wait 2s to check if wifi enabled
rem ping 172.0.0.1 -n 2 > nul

rem echo "(1)WLAN Modules Install"
rem set driver_path=/system/lib/modules
rem set drivers=compat cfg80211 mac80211 wlan_mt
rem for %%i in (%drivers%) do (	
rem 	@echo installing... %%i
rem 	adb shell lsmod|findstr %%i>nul || adb shell insmod %driver_path%/%%i_bp.ko
rem )

echo "(2)WLAN Interface Up"
adb shell "echo 1 > /dev/wmtWifi6630" 
ping 127.0.0.1 -n 2 > nul


echo "(3) wlan_service_up"
adb shell ps|findstr wpa_supplicant>nul || adb shell /system/bin/wpa_supplicant -Dnl80211 -iwlan0 -c/data/wifi/wpa_supplicant.conf & 
ping 127.0.0.1 -n 2 > nul
rem adb shell wpa_cli -iwlan0 -p/data/wifi/sockets status
adb shell wpa_cli -iwlan0 -p/data/wifi/sockets status
ping 127.0.0.1 -n 2 > nul
rem adb shell wpa_cli -i wlan0 -p/data/misc/wifi/sockets DISCONNECT
echo "disconnect wlan0"
adb shell wpa_cli -iwlan0 -p/data/wifi/sockets DISCONNECT
ping 127.0.0.1 -n 2 > nul
adb shell wpa_cli -iwlan0 -p/data/wifi/sockets status|findstr DISCONNECT>nul || adb shell wpa_cli -iwlan0 -p/data/wifi/sockets DISCONNECT
ping 127.0.0.1 -n 2 > nul
adb shell wpa_cli -iwlan0 -p/data/wifi/sockets status
ping 127.0.0.1 -n 2 > nul
rem adb shell ps|findstr dhcpcd>nul || adb shell start dhcpcd
adb shell ps|findstr wpa_supplicant>nul || goto start_wlan_fail
echo "start wpa_supplicant success"
ping 127.0.0.1 -n 2 > nul

rem adb shell getprop | findstr combo.chipid | findstr 6630 > nul && (adb shell iwpriv wlan0 set_sw_ctrl 0xa0400000 0x1)

:start_dut_and_ca
start "" "%~f0" start_dut %1
cls
rem %1(start /min cmd.exe /c %0 :&exit)
ping 127.0.0.1 -n 2 > nul
title Control-Agent
rem forward local pc port 9997 to remount port 6666 in phone
adb forward tcp:9997 tcp:6666
rem 9000:port to ucc; 127.0.0.1:ip of PC which wfa_dut connected; 9997: port of PC which wfa_dut connected
echo "begin to start wfa_ca"
wfa_ca lo 9000 127.0.0.1 9997
msg %username% error happens on wfa_ca!
goto :eof

:start_dut
title WFA-DUT
rem if %2 is D, then wfa_dut will print more traces
mode con cols=35 lines=5
echo "begin to start wfa_dut"
adb shell wfa_dut lo 6666 %2 > dut_log.txt
msg %username% error happens on wfa_dut!
goto :eof

:start_wlan_fail
msg %username% enable wlan in sigma mode fail, please call help!
goto :eof

:check_test_env

