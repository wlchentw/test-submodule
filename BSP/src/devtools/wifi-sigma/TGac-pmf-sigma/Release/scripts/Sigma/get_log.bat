@echo off

adb pull /mnt/asec/mtk_hs20_wfa_dut.log
adb pull /mnt/asec/mtk_hs20_wpa_supplicant.log
adb pull /mnt/asec/mtk_hs20_kmsg.log

echo "Get Log files successfully"
pause