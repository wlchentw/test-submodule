#!/bin/sh

/usr/bin/android-gadget-setup none
/usr/bin/android-gadget-setup rndis_adb
/usr/bin/adbd &
/usr/bin/android-gadget-setup post
