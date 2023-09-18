REM FORFILES /p Put_Your_Certificate_Here /C "cmd /c adb push @file /data/misc/wifi/"

adb root
adb shell mkdir /data/misc/wifi
adb push .\Put_Your_Certificate_Here\cas.pem /data/misc/wifi/
adb push .\Put_Your_Certificate_Here\wifiuser.key /data/misc/wifi/
adb push .\Put_Your_Certificate_Here\wifiuser.pem /data/misc/wifi/

@echo "Upload Certificate Successfully!"
pause