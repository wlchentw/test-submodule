@echo off
for /f "tokens=2" %%i in ('tasklist /V ^| findstr Control-Agent') do (taskkill /FI "pid eq %%i")
for /f "tokens=2" %%i in ('tasklist /V ^| findstr WFA-DUT') do (taskkill /FI "pid eq %%i")
choice /C YN /M "press Y to turn off wlan, press N to leave wlan in sigma mode"
if ERRORLEVEL 2 goto :eof
adb shell ps|findstr wpa_supplicant>nul || echo wlan already off && goto :eof
adb shell ls /data/misc/wifi/sockets/wpa_ctrl* | findstr /C:"No such file">nul || echo wlan is in normal mode && goto :eof
echo stop wifi
adb shell setprop wlan.driver.status unloaded
adb shell wpa_cli -iwlan0 -g@android:wpa_wlan0 TERMINATE
