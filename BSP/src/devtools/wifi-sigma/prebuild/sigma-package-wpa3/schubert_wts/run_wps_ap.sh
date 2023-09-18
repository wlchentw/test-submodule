#!/bin/sh
echo "Enable ap0 interface for WPS AP"
ifconfig ap0 up
ifconfig ap0 192.168.1.1 netmask 255.255.255.0
echo "Enable udhcpd..."
mkdir -p /var/lib/misc
touch /var/lib/misc/udhcpd.leases
udhcpd /etc/udhcpd.conf&
echo "Set FwReplyProbResp=0"
iwpriv ap0 driver "set_cfg FwReplyProbResp 0"
echo "Enable HOSTAPD..."
hostapd -dd /data/wts/wps_hostapd.conf &