@echo off

call .\programs\PARA_SETUP.bat

adb wait-for-device
adb root
adb wait-for-device
adb remount
adb wait-for-device



rem %1(start /min cmd.exe /c %0 :&exit)
ping 127.0.0.1 -n 5 > nul
title Control-Agent
mode con cols=35 lines=5
rem forward local pc port 9997 to remount port 6666 in phone
adb forward tcp:9997 tcp:6000
rem 9000:port to ucc; 127.0.0.1:ip of PC which wfa_dut connected; 9997: port of PC which wfa_dut connected
rem ca_log.log, redirect log to log/ca_log.log
.\programs\wfa_ca.exe lo %CA_PORT% 127.0.0.1 9997 log/ca_log.log
goto :end
:end
pause
