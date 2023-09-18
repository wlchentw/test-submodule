@echo off
adb shell getprop | findstr combo.chipid | findstr 6630 > nul && (adb shell iwpriv wlan0 set_sw_ctrl 0xa0400000 0x1)
adb shell wpa_cli -iwlan0 -p/data/misc/wifi/sockets remove_network all
adb shell wpa_cli -iwlan0 -p/data/misc/wifi/scokets save_config

adb shell getprop | findstr ro.hardware | findstr 6755 > nul && goto jade_performance_mode
adb shell getprop | findstr ro.hardware | findstr 6757 > nul && goto olympus_performance_mode
adb shell getprop | findstr ro.hardware | findstr 6797 > nul && goto everest_performance_mode
adb shell getprop | findstr ro.hardware | findstr 6735 > nul && goto denali_performance_mode
adb shell getprop | findstr ro.hardware | findstr 6580 > nul && goto rainier_performance_mode
goto end

:olympus_performance_mode
:jade_performance_mode
adb shell "echo 4 4 > /proc/ppm/policy/ut_fix_core_num"
adb shell "echo 0 0 > /proc/ppm/policy/ut_fix_freq_idx"
goto end

:everest_performance_mode
adb shell "echo 4 4 2 > /proc/ppm/policy/ut_fix_core_num"
adb shell "echo 0 0 0 > /proc/ppm/policy/ut_fix_freq_idx"
goto end

:rainier_performance_mode
:denali_performance_mode
adb shell "echo 0 > /proc/hps/enabled"
adb shell "echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
adb shell "echo 1 > /sys/devices/system/cpu/cpu0/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu1/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu2/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu3/online"


goto end

:end

