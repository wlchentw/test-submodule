#!/bin/sh
# insmod wmt driver before wmt_loader starts
echo insmod wmt driver
insmod /lib/modules/mt66xx/wmt_drv.ko

#wmt_chrdev_wifi.ko and wlan_drv_gen3.ko are loaded from wmt_loader

# insmod bt driver
echo insmod bt driver
insmod /lib/modules/mt66xx/bt_drv.ko

# insmod gps driver
echo insmod gps driver
insmod /lib/modules/mt66xx/gps_drv.ko
