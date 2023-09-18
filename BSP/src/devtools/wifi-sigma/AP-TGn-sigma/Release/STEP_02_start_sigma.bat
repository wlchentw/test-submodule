@echo off

call .\programs\PARA_SETUP.bat

adb wait-for-device
adb root
adb wait-for-device
adb remount
adb wait-for-device
if not exist log mkdir log

echo "Set permissive mode"
adb shell setenforce permissive
adb shell getenforce

if "%1" == "start_performance_mode" goto :start_performance_mode
if "%1" == "start_dut" goto :start_dut

echo turning wifi off
call .\STEP_03_stop_sigma.bat
adb shell settings --user 0 put global wifi_scan_always_enabled 0
adb shell svc wifi disable > nul

:start_dut_and_ca
start "" "%~f0" start_dut %1
start "" "%~f0" start_performance_mode %1

cls
rem %1(start /min cmd.exe /c %0 :&exit)
ping 127.0.0.1 -n 5 > nul
title Control-Agent
mode con cols=35 lines=5
rem forward local pc port 9997 to remount port 6666 in phone
adb forward tcp:9997 tcp:6000
rem 9000:port to ucc; 127.0.0.1:ip of PC which wfa_dut connected; 9997: port of PC which wfa_dut connected
rem ca_log.log, redirect log to log/ca_log.log
.\programs\wfa_ca.exe lo %CA_PORT% 127.0.0.1 9997 log/ca_log.log
msg %username% E006: error happens on wfa_ca!
goto :end

:start_dut
title WFA-DUT
rem if %2 is D, then wfa_dut will print more traces
cls
mode con cols=35 lines=5
adb shell wfa_dut lo 6000 %DEBUG% %PCENDPOINT_IP_ADDRESS% %PCENDPOINT_IP_NETMASK% %HOSTAPD_BIN_PATH% %INTERFACE_NAME_AP% %CTRL_INTERFACE_HOSTAPD% %CONFIG_PATH_HOSTAPD% %CONFIG_PATH_HOSTAPD_BK% %CONFIG_PATH_WLAN_DRIVER%
adb get-state|findstr "unknown">nul && msg %username% E007: adb connection lost! || msg %username% E008: error happens on wfa_dut!
goto :end

:start_performance_mode
title performance_mode
if exist .\programs\callback_for_start.bat (start .\programs\callback_for_start.bat)
goto :end_without_message

:start_wlan_fail
msg %username% E009: enable wlan in sigma mode fail!

:end
start notepad.exe FAQ.txt

:end_without_message
exit