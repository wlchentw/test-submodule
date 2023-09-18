@echo off

call .\programs\PARA_SETUP.bat
rem sigma_dir is used to store sigma binary and others sigma related programes. So this directory must can been read and write. You can change it according to your platform
set sigma_dir=/data
@color 00
@echo wait for device ready
adb wait-for-device
adb root
adb wait-for-device
adb shell "mount | grep \"on /vendor\"" | findstr "\/dev\/block\/dm-" || goto disable_verity_done
echo "Disable verity.."
@color 0c
@echo disable-verity
adb disable-verity
@echo need to reboot device for setttings to take effect
adb reboot
@echo wait for device ready
adb wait-for-device

:disable_verity_done
@echo root device
adb root
@echo wait for device ready
adb wait-for-device
adb remount
@color 00

adb push programs/wfa_dut %sigma_dir%
adb shell "chmod 777 %sigma_dir%/wfa_dut"
rem adb push programs/busybox %sigma_dir%
rem adb shell "chmod 777 %sigma_dir%/busybox"
rem adb push programs/busybox-full %sigma_dir%
rem adb shell "chmod 777 %sigma_dir%/busybox-full"
adb push programs/wfa_con %sigma_dir%
adb shell "chmod 777 %sigma_dir%/wfa_con"
for /r %%f in (scripts\*.sh) do (
adb push "%%f" %sigma_dir%
adb shell "chmod 777 %sigma_dir%/%%~nxf"
)
rem adb push programs/iwpriv %sigma_dir%
rem adb shell "chmod 755 %sigma_dir%/iwpriv"
adb push programs/tos.txt /tmp/asec/
rem adb shell "chmod 755 %sigma_dir%/iwpriv"

echo "Push default hostapd.conf for Android"
adb push programs/hostapd_ap0.conf %CONFIG_PATH_HOSTAPD%
adb shell "cp %CONFIG_PATH_HOSTAPD% %CONFIG_PATH_HOSTAPD_BK%"

adb shell sync

:end
@echo "Software installation complete!!"
pause

