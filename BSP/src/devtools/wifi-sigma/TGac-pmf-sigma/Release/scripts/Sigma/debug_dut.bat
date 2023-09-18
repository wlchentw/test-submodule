@echo off
@if \%1\==\usage\ goto usage


@echo Running test-bed side control agent
start "" adb shell "logcat /system/bin/wpa_supplicant:I wpa_supplicant:I *:S > /mnt/asec/mtk_hs20_wpa_supplicant.log"
start "" adb shell "cat /proc/kmsg > /mnt/asec/mtk_hs20_kmsg.log"
adb shell "wfa_dut lo 6666 > /mnt/asec/mtk_hs20_wfa_dut.log"
@goto end

:usage
@echo Run this batch to start test-bed side control agent.
@echo Please specify # start_dut.bat

:end
pause
