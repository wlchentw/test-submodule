@echo off
set sigma_dir=/system/bin
set command=cut tr sed awk find less more ipconfig ipaddr arp
rem cut tr sed awk find less more ipconfig ipaddr arp grep
if not exist scripts (msg %username% no scripts folder found! && goto :eof)
if not exist log (mkdir log)
color 00
echo wait for device ready
adb wait-for-device
adb root
adb wait-for-device
adb shell "mount|grep /system"|findstr "dm-"||goto disable_verity_done
color 0c
echo disable-verity
adb disable-verity
echo need to reboot device for setttings to take effect
adb reboot
echo wait for device ready
adb wait-for-device

:disable_verity_done
echo root device
adb root
echo wait for device ready
adb wait-for-device
adb remount
echo off
color 00

if exist scripts\callback_for_install.bat (call scripts\callback_for_install.bat)
for /f %%f in (scripts/install_list.txt) do (adb push "scripts/%%f" %sigma_dir%
adb shell chmod 755 %sigma_dir%/%%~nxf)
for %%i in (%command%) do (adb shell ln -ns busybox-full %sigma_dir%/%%i)

:end
adb push scripts\wfa_dut %sigma_dir%
adb shell chmod 755 %sigma_dir%/wfa_dut
echo "Software installation complete!!"
pause
