@echo off
if "%1" == "start_dut" goto start_dut
set bin_dir=/system/bin
set version=20151230
tasklist | findstr wfa_ca>nul && (adb shell ps | findstr wfa_dut > nul && msg %username% E001: sigma tool is running now! && goto :end)
rem check adb connection
adb get-state|findstr "unknown">nul && (msg %username% E002: no dut connected! && goto :end)
rem check build type, only eng or user debug load can are allowed to run sigma tool
adb root | findstr /c:"production builds">nul && (msg %username% E003: sigma test can only run on eng or userdebug load && goto :end)

adb wait-for-device
rem check necessary scripts can be executed or not
for /f %%i in (scripts\install_list.txt) do (adb shell ls -l %bin_dir%/%%~nxi | findstr [-r][-w]x>nul || msg %username% E004: %%~nxi is not exist or can't be executed! && goto :end)
adb shell ps|findstr wpa_supplicant>nul || goto turn_on_wifi_in_sigma_mode
rem check if wlan normal mode is enabled
adb shell ls /data/misc/wifi/sockets/wpa_ctrl* | findstr /C:"No such">nul && goto start_dut_and_ca
echo wlan is in normal mode, turn off first
adb shell svc wifi disable > nul
ping 127.0.0.1 -n 4 > nul

:turn_on_wifi_in_sigma_mode
echo turning wifi on, wait 2 seconds
adb shell "echo S > /dev/wmtWifi"
ping 127.0.0.1 -n 2 > nul
adb shell setprop ctl.start p2p_supplicant
echo wait 2s to check if wifi enabled
ping 172.0.0.1 -n 2 > nul
adb shell ps|findstr wpa_supplicant>nul || goto start_wlan_fail
if exist scripts\callback_for_start.bat (call scripts\callback_for_start.bat)

:start_dut_and_ca
start "" "%~f0" start_dut %1
cls
rem %1(start /min cmd.exe /c %0 :&exit)
ping 127.0.0.1 -n 2 > nul
title Control-Agent
mode con cols=35 lines=5
rem forward local pc port 9997 to remount port 6666 in phone
adb forward tcp:9997 tcp:6669
rem 9000:port to ucc; 127.0.0.1:ip of PC which wfa_dut connected; 9997: port of PC which wfa_dut connected
rem ca_log.log, redirect log to ca_log.log
echo start_sigma.bat build:%version% > log/ca_log.log
scripts\wfa_ca lo 9000 127.0.0.1 9997 log/ca_log.log
msg %username% E005: error happens on wfa_ca!
goto :end

:start_dut
title WFA-DUT
rem if %2 is D, then wfa_dut will print more traces
mode con cols=35 lines=5
adb shell %bin_dir%/wfa_dut lo 6669 %2 > log\dut_log.log
adb get-state|findstr "unknown">nul && msg %username% E006: adb connection lost! || msg %username% E007: error happens on wfa_dut!
goto :end

:start_wlan_fail
msg %username% E008: enable wlan in sigma mode fail!

:end
start notepad.exe scripts\FAQ.txt

