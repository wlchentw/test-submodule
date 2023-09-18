adb wait-for-device
adb root
@ping 127.0.0.1 -n 3 -w 1000 > null
::adb remount
cd schubert_wts
adb push scripts /data/wts/scripts/
adb push scripts_busybox /data/wts/scripts_busybox/
adb push Sigma_Auto_Check_Tool /data/wts/Sigma_Auto_Check_Tool/
adb push busybox /data/wts/
adb push mtk_ini.ini /data/wts/
adb push PMF_DUTInfo.txt /data/wts/
adb push ping /data/wts/
cd ..
adb push p2p_supplicant.conf data/wts/
adb push wpa_supplicant.conf data/wts/
adb push wps_hostapd.conf data/wts/

set /p type="Test: (1)sta (2)p2p (3)wmmps (4)wpa (5)voe (6)wps_ap/wps_sta "

if "%type%"=="1" (goto update_STA)
if "%type%"=="2" (goto update_P2P)
if "%type%"=="3" (goto update_WMMPS)
if "%type%"=="4" (goto update_WPA) 
if "%type%"=="5" (goto update_VOE) 
if "%type%"=="6" (goto update_WPS_AP_STA) else (goto update_end)
:update_STA
adb push schubert_wts\sigma_sta /data/wts/sigma_sta/
adb push schubert_wts\run_sigma_sta.sh /data/wts/
goto uptate_cont

:update_P2P
adb push schubert_wts\sigma_p2p /data/wts/sigma_p2p/
adb push schubert_wts\run_sigma_p2p.sh /data/wts/
goto uptate_cont

:update_WMMPS
adb push schubert_wts\sigma_wmmps /data/wts/sigma_wmmps/
adb push schubert_wts\run_sigma_wmmps.sh /data/wts/
goto uptate_cont

:update_WPA
adb push schubert_wts\sigma_wpa /data/wts/sigma_wpa/
adb push schubert_wts\run_sigma_wpa.sh /data/wts/
goto uptate_cont

:update_VOE
adb push schubert_wts\sigma_voe /data/wts/sigma_voe/
adb push schubert_wts\run_sigma_voe.sh /data/wts/
goto uptate_cont

:update_WPS_AP_STA
adb push schubert_wts\run_wps_sta.sh /data/wts/
adb push schubert_wts\run_wps_ap.sh /data/wts/
goto uptate_cont

:uptate_cont
adb shell chmod 777 /data/wts/* -R
goto uptate_end

:update_end
pause
pause