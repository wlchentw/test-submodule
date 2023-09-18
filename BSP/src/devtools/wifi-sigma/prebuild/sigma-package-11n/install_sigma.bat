@echo off
rem sigma_dir is used to store sigma binary and others sigma related programes. So this directory must can been read and write. You can change it according to your platform
set sigma_dir=/data
rem fw_dir is used to store the wifi firmware. You can change it according to your platform. Currently no need to use fw_dir.
set fw_dir=/lib/firmware
set command=cut tr sed awk find less more ipconfig ipaddr arp
rem if not exist scripts (msg %username% no scripts folder found! && goto :eof)

ping 127.0.0.1 -n 1> nul
adb root
adb wait-for-device
adb remount
adb push wfa_dut %sigma_dir%
adb shell chmod 777 %sigma_dir%/wfa_dut
rem push ping_sigma to device. the ping_sigma support "-i" parameter. If the device's ping cmd already support "-i" then no need to push ping_sigma
adb push ping_sigma %sigma_dir%
adb shell chmod 777 %sigma_dir%/ping_sigma
adb shell sync

rem if exist scripts\wifi.cfg (adb shell getprop | findstr combo.chipid | findstr 8516 > nul && (adb push scripts\wifi.cfg data/))
rem adb shell ls %sigma_dir%/busybox-full | findstr "No such">nul || goto end
rem for /r %%f in (scripts\*) do (echo upload %%f && adb push "%%f" %sigma_dir% && adb shell chmod 755 %sigma_dir%/%%~nxf)
rem for %%i in (%command%) do (adb shell busybox-full ln -fns %sigma_dir%/busybox-full %sigma_dir%/%%i)
rem :end
@echo "Software installation complete!!"
pause
