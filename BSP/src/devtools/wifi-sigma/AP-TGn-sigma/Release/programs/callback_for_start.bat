@echo off
REM Always keep screen on when enabling performance mode
adb shell "svc power stayon usb"
adb shell getprop | findstr ro.hardware | findstr 6755 > nul && goto jade_performance_mode
adb shell getprop | findstr ro.hardware | findstr 6757 > nul && goto olympus_performance_mode
adb shell getprop | findstr ro.hardware | findstr 6797 > nul && goto everest_performance_mode
adb shell getprop | findstr ro.hardware | findstr 6758 > nul && goto vinson_performance_mode
adb shell getprop | findstr ro.hardware | findstr 6771 > nul && goto vinson_performance_mode
adb shell getprop | findstr ro.hardware | findstr 6775 > nul && goto vinson_performance_mode
adb shell getprop | findstr ro.hardware | findstr 6735 > nul && goto denali_performance_mode
adb shell getprop | findstr ro.hardware | findstr 6580 > nul && goto rainier_performance_mode
adb shell getprop | findstr ro.hardware | findstr 6763 > nul && goto bianco_performance_mode
adb shell getprop | findstr ro.hardware | findstr 6739 > nul && goto zion_performance_mode
adb shell getprop | findstr ro.hardware | findstr 6765 > nul && goto cervino_performance_mode
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

:vinson_performance_mode
adb shell "echo 4 4 > /proc/ppm/policy/ut_fix_core_num"
adb shell "echo 0 0 > /proc/ppm/policy/ut_fix_freq_idx"
goto end

:rainier_performance_mode
REM Rainer platform needs this command to boost frequency
adb shell "echo 0 > /proc/cpufreq/cpufreq_oppidx" 
:denali_performance_mode
adb shell "echo 0 > /proc/hps/enabled"
adb shell "echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
adb shell "echo 1 > /sys/devices/system/cpu/cpu0/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu1/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu2/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu3/online"
goto end

:bianco_performance_mode
adb shell "echo enable 0 > /sys/kernel/debug/mcdi/mcdi_state"
adb shell "echo 4 4 > /proc/ppm/policy/ut_fix_core_num"
adb shell "echo 0 0 > /proc/ppm/policy/ut_fix_freq_idx"
adb shell "echo soidle3 0 > /sys/kernel/debug/cpuidle/soidle3_state"
adb shell "echo soidle 0 > /sys/kernel/debug/cpuidle/soidle_state"
adb shell "echo dpidle 0 > /sys/kernel/debug/cpuidle/dpidle_state"
goto end

:zion_performance_mode
adb shell "echo 10:3000000000 > /proc/net/wlan/autoPerfCfg"
goto end

:cervino_performance_mode
adb shell "echo 0 0 > /proc/ppm/policy/ut_fix_freq_idx"
adb shell "echo 100 > proc/perfmgr/eas/debug_ta_boost"
goto end

:end
