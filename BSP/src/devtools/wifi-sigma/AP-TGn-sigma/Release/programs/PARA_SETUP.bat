call .\ENV_SETUP.bat

SET INTERFACE_NAME_AP=ap0
SET DEBUG=D
SET CONFIG_PATH_WLAN_DRIVER=/data/misc/wifi/wifi.cfg




SET HOSTAPD_BIN_PATH=hostapd
SET CTRL_INTERFACE_HOSTAPD=/tmp/hostapd
rem CONFIG_PATH_HOSTAPD should be a path which has access rights to read and write
SET CONFIG_PATH_HOSTAPD=/data/hostapd_ap0.conf
SET CONFIG_PATH_HOSTAPD_BK=/data/hostapd_ap0_bk.conf

:end