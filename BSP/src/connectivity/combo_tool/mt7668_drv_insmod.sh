#!/bin/sh
# insmod mt7668 driver before appmainprog starts
echo insmod mt7668 wlan driver
insmod /lib/modules/mt7668/wlan_mt7668_sdio.ko

echo insmod mt7668 bt driver
insmod /lib/modules/mt7668/btmtksdio.ko
