wmmps sigma tool release notes:

if you do this test firstly on this device, please:
1. install_sigma.bat, you should run this bat file to install all sigma running environments

if sigma tool has been installed on your device, you can:
1. start_sigma.bat, run this bat file to start sigma test
2. stop_sigma.bat, run this bat file to stop sigma test
if you meet some errors when run sigma tool, or you need to stop sigma test some time, then
run this test again, you can run stop_sigma.bat, and input N to leave the phone in sigma test
mode, this will save your time to run simga tool again.

For 5.2.14, tester need to use this cmd to add route:
adb shell busybox-full route add -net 224.0.0.0 netmask 240.0.0.0 dev wlan0

Note: the log for wfa_dut and wfa_ca will be saved into ca_log.txt and dut_log.txt, if you meet any issue
when run sigma tool, please provide the two logs, sniffer log, UCC log, and any other logs you collected.
