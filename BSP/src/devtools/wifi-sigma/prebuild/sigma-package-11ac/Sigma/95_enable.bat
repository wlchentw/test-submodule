rem disable soc dvfs  

adb shell "echo 0 > /proc/cpufreq/socfreq_state"

rem GPU set to 700Mhz
rem adb shell "echo 0 4 > /proc/freqhopping/status"
rem adb shell "echo 6B 40 > /sys/devices/platform/mt6333-user/mt6333_access"
rem adb shell "echo 6C 40 > /sys/devices/platform/mt6333-user/mt6333_access"
rem adb shell "echo 4 0x801b4000 > /proc/clkmgr/pll_fsel"
rem adb shell "cat /proc/pm_init/mfgclk_speed_dump"

rem DVFS/Hotplug off:
adb shell "echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
adb shell "echo 0 > /sys/module/mt_hotplug_mechanism/parameters/g_enable"
adb shell "echo 1 > /sys/devices/system/cpu/cpu1/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu2/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu3/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu4/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu5/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu6/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu7/online"

rem ATM off
adb shell "/system/bin/thermal_manager /etc/.tp/.ht120.mtc"

rem mTH
adb shell "echo 110000 120000 > /proc/cpufreq/cpufreq_ptpod_temperature_limit"
adb shell cat /proc/cpufreq/cpufreq_downgrade_freq_in

rem adb shell "echo 45000 65000 > /proc/cpufreq/cpufreq_ptpod_temperature_limit"
rem adb shell cat /proc/cpufreq/cpufreq_downgrade_freq_info
pause
