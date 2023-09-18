adb wait-for-device
::adb shell "setprop sys.usb.config rndis,adb;"

@ping 127.0.0.1 -n 3 -w 1000 > null
adb wait-for-device

@echo off
adb push rndis_adb.sh /data
adb shell chmod 777 /data/rndis_adb.sh
adb shell cd  /data
adb shell nohup ./rndis_adb.sh &
pause