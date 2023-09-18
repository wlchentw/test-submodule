echo ====== DO NOT RELEASE DOWNLOAD KEY UNTIL FOLLOWING STEP SAY IT! ======

python fbtool.py
::Wait for reboot finish
ping 123.45.67.89 -n 1 -w 50 > nul

fastboot erase mmc0boot0
fastboot erase mmc0

echo ====== YOU CAN RELEASE DOWNLOAD KEY NOW. ======

fastboot flash mmc0 MBR_EMMC
fastboot flash mmc0boot0 bl2.img
fastboot flash UBOOT u-boot-mtk-fit.bin
fastboot flash tee_a tee.img
fastboot flash boot_a boot.img
fastboot flash system_a rootfs.img
fastboot flash userdata userdata.ext4

fastboot reboot

echo Finish downloading images!
pause
