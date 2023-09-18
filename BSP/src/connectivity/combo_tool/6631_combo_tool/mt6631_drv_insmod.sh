#!/bin/sh
# insmod wmt driver before wmt_loader starts
echo insmod wmt driver
insmod /lib/modules/mt66xx/wmt_drv.ko
#echo insmod wifi cdev driver
#insmod /lib/modules/mt66xx/wmt_chrdev_wifi.ko
#echo insmod wifi driver
#insmod /lib/modules/mt66xx/wlan_drv_gen4m.ko
#echo insmod bt driver
#insmod /lib/modules/mt66xx/wmt_cdev_bt.ko

#wpa_supplicant -Dnl80211 -iwlan0 -c/etc/wpa_supplicant.conf
