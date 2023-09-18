@echo on
@color 00
@echo wait for device ready
adb wait-for-device
adb root
adb wait-for-device
adb shell "mount|grep /system"|findstr "dm-"||goto disable_verity_done
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
@echo off
@color 00

REM FORFILES /p sw /C "cmd /c adb push @file /system/bin/"
REM for /r %%f in (.\sw\*) do (adb push %%f /system/bin/)

REM FORFILES /p sw /C "cmd /c adb shell chmod 700 /system/bin/@file"
REM for /r %%f in (.\sw\*) do (adb shell chmod 700 /system/bin/%%~nxf)

adb push .\sw\busybox-full /data/
adb push .\sw\getpid.sh /data/
adb push .\sw\getpstats.sh /data/
adb push .\sw\ipconfig.txt /data/
adb push .\sw\mtk_p2p_getipconfig.sh /data/
adb push .\sw\ndsend /data/
adb push .\sw\stoping.sh /data/
adb push .\sw\updatepid.sh /data/
adb push .\sw\wfaping.sh /data/
adb push .\sw\wfav6ping.sh /data/
adb push .\sw\wfa_dut /data/

adb shell chmod 700 /data/busybox-full
adb shell chmod 700 /data/getpid.sh
adb shell chmod 700 /data/getpstats.sh
adb shell chmod 700 /data/ipconfig.txt
adb shell chmod 700 /data/mtk_p2p_getipconfig.sh
adb shell chmod 700 /data/ndsend
adb shell chmod 700 /data/stoping.sh
adb shell chmod 700 /data/updatepid.sh
adb shell chmod 700 /data/wfaping.sh
adb shell chmod 700 /data/wfav6ping.sh
adb shell chmod 700 /data/wfa_dut

adb shell /data/busybox-full ln -fns /data/busybox-full /data/cut
adb shell /data/busybox-full ln -fns /data/busybox-full /data/tr
adb shell /data/busybox-full ln -fns /data/busybox-full /data/sed
adb shell /data/busybox-full ln -fns /data/busybox-full /data/awk
adb shell /data/busybox-full ln -fns /data/busybox-full /data/find
adb shell /data/busybox-full ln -fns /data/busybox-full /data/less
adb shell /data/busybox-full ln -fns /data/busybox-full /data/more
adb shell /data/busybox-full ln -fns /data/busybox-full /data/ipconfig
adb shell /data/busybox-full ln -fns /data/busybox-full /data/ipaddr
adb shell /data/busybox-full ln -fns /data/busybox-full /data/arp
adb shell /data/busybox-full ln -fns /data/busybox-full /data/grep

@echo "Software installation complete!!"
pause
