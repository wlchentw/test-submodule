@echo off

call .\programs\PARA_SETUP.bat
set sigma_dir=/data
adb wait-for-device
adb root
adb wait-for-device
adb remount
adb wait-for-device
if not exist log mkdir log

echo "Enable Performance Mode.."
adb shell "echo 0 > /proc/hps/enabled"
adb shell "echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
adb shell "echo 1 > /sys/devices/system/cpu/cpu0/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu1/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu2/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu3/online"

echo "Create tmp asec directory"
adb shell mkdir /tmp/asec
adb push programs/tos.txt /tmp/asec/

rem start wfa_dut
title WFA-DUT
mode con cols=35 lines=5
adb shell %sigma_dir%/wfa_dut lo 6000 %DEBUG% %PCENDPOINT_IP_ADDRESS% %PCENDPOINT_IP_NETMASK% %HOSTAPD_BIN_PATH% %INTERFACE_NAME_AP% %CTRL_INTERFACE_HOSTAPD% %CONFIG_PATH_HOSTAPD% %CONFIG_PATH_HOSTAPD_BK% %CONFIG_PATH_WLAN_DRIVER% %2 > log/dut_log.log
@goto end
:end
pause