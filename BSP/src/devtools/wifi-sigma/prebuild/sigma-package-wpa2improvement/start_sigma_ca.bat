@echo off
if not exist scripts (msg %username% no scripts folder found! && goto :eof)

ping 127.0.0.1 -n 4 > nul

:start_dut_and_ca
title Control-Agent
rem forward local pc port 9997 to remount port 6666 in device
adb forward tcp:9997 tcp:6666
rem 9000:port to ucc; 127.0.0.1:ip of PC which wfa_dut connected; 9997: port of PC which wfa_dut connected
wfa_ca lo 9000 127.0.0.1 9997 log/sta_ca_log.txt




