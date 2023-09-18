@echo off
adb shell ps -A | findstr hostapd > nul || goto :stop_gen2_driver
echo stop hostapd
adb shell "echo $(ps -A | grep hostapd) | cut -d' ' -f2 | xargs kill"
:stop_gen2_driver
adb shell ps -A | findstr tx_thread > nul || goto :stop_gen4m_driver
echo stop wlan driver
adb shell "echo 0 > /dev/wmtWifi"
:stop_gen4m_driver
adb shell ps -A | findstr main_thread > nul || goto :stop_wfa_dut
echo stop wlan driver
adb shell "echo 0 > /dev/wmtWifi"
:stop_wfa_dut
adb shell ps -A | findstr wfa_dut > nul || goto :stop_wfa_con
echo stop wfa_dut
adb shell "echo $(ps -A | grep wfa_dut) | cut -d' ' -f2 | xargs kill"
:stop_wfa_con
adb shell ps -A | findstr wfa_con > nul || goto :stop_wfa_ca
echo stop wfa_con
adb shell "echo $(ps -A | grep wfa_con) | cut -d' ' -f2 | xargs kill"
:stop_wfa_ca
for /f "tokens=2" %%i in ('tasklist /V ^| findstr wfa_ca') do (taskkill /F /FI "pid eq %%i")
