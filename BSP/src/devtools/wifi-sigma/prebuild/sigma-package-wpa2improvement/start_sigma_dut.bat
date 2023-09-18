@echo on
rem sigma_dir is same the sigma_dir defined by install_sigma.bat
set sigma_dir=/data
:start_dut
title WFA-DUT
rem if %2 is D, then wfa_dut will print more traces
mode con cols=55 lines=35
adb shell wpa_cli -iwlan0 -p/tmp/wpa_supplicant disconnect
adb shell %sigma_dir%/wfa_dut lo 6666 %2 > log/sta_dut_log.txt
msg %username% error happens on wfa_dut!





