@echo off
if exist scripts\wifi.cfg (adb shell getprop | findstr combo.chipid | findstr 663 > nul && (adb push scripts\wifi.cfg data/misc/wifi/))
if exist scripts\wifi_fw.cfg (adb shell getprop | findstr combo.chipid | findstr 663 > nul && (adb push scripts\wifi_fw.cfg data/misc/wifi/))

@echo on

adb shell cat /data/misc/wifi/wifi.cfg
adb shell cat /data/misc/wifi/wifi_fw.cfg

@echo off
